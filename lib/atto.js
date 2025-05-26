var wasm = null;
var exports = null;
var contexts = [];
var currentLogMessage = [];

function internString(string) {
    var buffer = new TextEncoder().encode(string);
    var address = exports.internString(buffer.byteLength);
    var memoryView = new DataView(exports.memory.buffer, 0, exports.memory.byteLength);

    for (var i = 0; i < buffer.byteLength; i++) {
        memoryView.setUint8(address + i, buffer[i]);
    }

    memoryView.setUint8(address + string.length, 0);

    return address;
}

function freeString(address) {
    exports.freeString(address);
}

function readString(address) {
    if (address == 0) {
        return null;
    }

    var memoryView = new DataView(exports.memory.buffer, 0, exports.memory.byteLength);
    var chars = [];

    while (memoryView.getUint8(address)) {
        chars.push(String.fromCharCode(memoryView.getUint8(address++)));
    }

    return chars.join("");
}

function yieldThread() {
    return new Promise(function(resolve, reject) {
        requestAnimationFrame(resolve);
    });
}

export class Argument {
    #context;
    #address = 0;

    constructor(context, address) {
        this.#context = context;
        this.#address = address;
    }
    
    #getContext() {
        return this.#context._context;
    }

    get type() {
        return exports.getArgType(this.#getContext(), this.#address);
    }

    get asNumber() {
        return exports.evalArgAsNumber(this.#getContext(), this.#address);
    }

    get asString() {
        var stringAddress = exports.evalArgAsString(this.#getContext(), this.#address);
        var string = readString(stringAddress);

        freeString(string);

        return string;
    }

    get asBool() {
        return exports.evalArgAsBool(this.#getContext(), this.#address);
    }

    set asNumber(value) {
        exports.assignArgAsNumber(this.#getContext(), this.#address, value);
    }

    set asString(value) {
        var stringAddress = internString(value);

        exports.assignArgAsString(this.#getContext(), this.#address, stringAddress);

        freeString(stringAddress);
    }

    set asBool(value) {
        exports.assignArgAsBool(this.#getContext(), this.#address, value);
    }
}

export class Context {
    #context = null;
    #steps = 0;
    #running = false;
    #stopCompleted = false;
    #commands = {};

    constructor({
        addStandardCommands = true,
        yieldFrequencyNs = 10
    } = {}) {
        if (!wasm) {
            throw new ReferenceError("atto.js hasn't been initialised yet — please call `init`");
        }

        this.#context = exports.newContext();

        this.yieldFrequencyNs = yieldFrequencyNs;

        if (addStandardCommands) {
            exports.addContextStandardCommands(this.#context);
        }

        contexts.push(this);
    }

    #getContext() {
        if (!this.#context) {
            throw new ReferenceError("This context has been destroyed");
        }

        return this.#context;
    }

    get _context() {
        return this.#context;
    }

    _handleCommand(commandAddress) {
        var command = readString(commandAddress);
        var args = [];
        var currentArg;

        while (currentArg = exports.getNextArg(this.#getContext())) {
            args.push(new Argument(this, currentArg));
        }
        
        this.#commands[command](...args);
    }

    load(code) {
        var codeString = internString(code);

        try {
            exports.load(this.#getContext(), codeString);
        } finally {
            freeString(codeString);
        }
    }

    step() {
        var running = !!exports.step(this.#getContext());

        this.#steps++;

        return running;
    }

    async run() {
        if (this.#running) {
            throw new Error("This context is currently running");
        }

        var lastYield = performance.now();

        this.#running = true;

        while (this.#running) {
            if (!this.step()) {
                break;
            }

            var currentTime = performance.now();

            if (currentTime - lastYield >= this.yieldFrequencyNs) {
                await yieldThread();

                lastYield = currentTime;
            }
        }

        this.#running = false;
        this.#stopCompleted = true;
    }

    async stop() {
        this.#stopCompleted = false;
        this.#running = false;

        while (!this.#stopCompleted) {
            await yieldThread();
        }
    }

    addCommand(name, handler) {
        if (!this.#commands[name]) {
            exports.addCommand(this.#getContext(), internString(name));
        }

        this.#commands[name] = handler;
    }

    destroy() {
        this.stop();

        exports.freeContext(this.#getContext());

        this.#context = null;

        contexts = contexts.filter((context) => context != this);
    }
}

function log(address) {
    var memoryView = new DataView(exports.memory.buffer, 0, exports.memory.byteLength);

    while (memoryView.getUint8(address)) {
        var char = String.fromCharCode(memoryView.getUint8(address++));

        if (char == "\n") {
            console.log(currentLogMessage.join(""));

            globalThis.dispatchEvent(new CustomEvent("atto-log", {detail: {
                message: currentLogMessage.join("")
            }}));

            currentLogMessage = [];
        } else {
            logChar(char);
        }
    }
}

function logChar(char) {
    currentLogMessage.push(char);

    globalThis.dispatchEvent(new CustomEvent("atto-logchar", {detail: {
        char
    }}));
}

function handleCommand(context, commandAddress) {
    contexts.find((instance) => instance._context == context)._handleCommand(commandAddress);
}

export async function init() {
    var response = await fetch(new URL("bin/atto.wasm", import.meta.url));
    var code = await response.arrayBuffer();

    wasm = await WebAssembly.instantiate(code, {
        main: {
            log,
            logChar,
            handleCommand
        }
    });

    exports = wasm.instance.exports;

    exports.init(exports.__heap_base);
}