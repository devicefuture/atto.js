context.addCommand("input", function(message, identifier) {
    if (!identifier) {
        identifier = message;
        message = "";
    }

    if (!identifier) {
        return;
    }

    identifier.asString = prompt(message?.asString);
});

context.addCommand("alert", function(message) {
    alert(message?.asString);
});