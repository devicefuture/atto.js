var wasm = null;
var exports = null;
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

function yieldThread() {
    return new Promise(function(resolve, reject) {
        requestAnimationFrame(resolve);
    });
}

export class Context {
    #context = null;
    #steps = 0;
    #running = false;
    #stopCompleted = false;

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
    }

    #getContext() {
        if (!this.#context) {
            throw new ReferenceError("This context has been destroyed");
        }

        return this.#context;
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

    destroy() {
        this.stop();

        exports.freeContext(this.#getContext());

        this.#context = null;
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

export async function init() {
    var response = await fetch(new URL("bin/atto.wasm", import.meta.url));
    var code = await response.arrayBuffer();

    wasm = await WebAssembly.instantiate(code, {
        main: {
            log,
            logChar
        }
    });

    exports = wasm.instance.exports;

    exports.init(exports.__heap_base);
}