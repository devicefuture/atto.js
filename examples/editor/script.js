import * as atto from "../../lib/atto.js";

window.addEventListener("load", async function() {
    await atto.init();

    var context = new atto.Context({yieldFrequencyNs: 50});

    var demoInput = this.document.querySelector("#demo");
    var runButton = document.querySelector("#run");
    var stopButton = document.querySelector("#stop");
    var editor = document.querySelector("#editor");
    var jsEditor = document.querySelector("#jsEditor");
    var output = document.querySelector("#output");

    async function loadCode() {
        var response = await fetch(`demos/${demoInput.value}.atto`);
        var code = await response.text();

        editor.value = code;

        try {
            var response = await fetch(`demos/${demoInput.value}.js`);

            if (!response.ok) {
                throw null;
            }

            var code = await response.text();

            jsEditor.value = code;
        } catch (error) {
            jsEditor.value = "// This demo doesn't have any custom JavaScript code";
        }
    }

    var consoleLines = [""];

    demoInput.addEventListener("change", function() {
        loadCode();
    });

    runButton.addEventListener("click", function() {
        consoleLines = [""];

        eval(jsEditor.value);

        context.load(editor.value);
        context.run();
    });

    stopButton.addEventListener("click", function() {
        context.stop();
    });

    window.addEventListener("atto-log", function(event) {
        consoleLines[consoleLines.length - 1] = event.detail.message;

        consoleLines.push("");

        output.value = consoleLines.join("\n");

        while (consoleLines.length > 1000) {
            consoleLines.shift();
        }

        output.scrollTop = output.scrollHeight;
    });

    window.addEventListener("atto-logchar", function(event) {
        consoleLines[consoleLines.length - 1] += event.detail.char;

        output.value = consoleLines.join("\n");
        output.scrollTop = output.scrollHeight;
    });

    loadCode();
});