#include "memory.h"

#define WASM_EXPORT __attribute__((used)) __attribute__((visibility ("default")))
#define WASM_EXPORT_AS(name) WASM_EXPORT __attribute__((export_name(name)))
#define WASM_IMPORT(module, name) __attribute__((import_module(module))) __attribute__((import_name(name)))
#define WASM_CONSTRUCTOR __attribute__((constructor))

WASM_IMPORT("main", "log") void main_log(const char* text);
WASM_IMPORT("main", "logChar") void main_logChar(char c);
WASM_IMPORT("main", "handleCommand") void main_handleCommand(void* context, const char* command);

#define CATTO_NOSTDLIB
#define CATTO_LOG main_log
#define CATTO_LOG_CHAR main_logChar
#define CATTO_MALLOC malloc
#define CATTO_REALLOC realloc
#define CATTO_FREE free

#include "../include/catto/dist/catto-config.h"

typedef CATTO_BOOL catto_Bool;
typedef CATTO_CHAR catto_Char;
typedef struct catto_Context catto_Context;
typedef struct catto_AstNode catto_AstNode;

WASM_EXPORT_AS("newContext") catto_Context* catto_newContext();
WASM_EXPORT_AS("freeContext") void catto_freeContext(catto_Context* context);
WASM_EXPORT_AS("step") catto_Bool catto_step(catto_Context* context);
WASM_EXPORT_AS("load") void catto_load(catto_Context* context, const catto_Char* code);
WASM_EXPORT_AS("getNextArg") catto_AstNode* catto_getNextArg(catto_Context* context);

WASM_EXPORT_AS("addContextStandardCommands") void catto_addContextStandardCommands(catto_Context* context);

#include "../include/catto/dist/catto.h"

WASM_EXPORT_AS("internString") char* internString(size_t length) {
    return malloc(length + 1);
}

WASM_EXPORT_AS("freeString") void freeString(char* string) {
    free(string);
}

WASM_EXPORT_AS("init") void init(Block* memoryBase) {
    firstFreeBlock = base = memoryBase;
}

void handleCommand(catto_Context* context) {
    main_handleCommand(context, context->currentParsedStatement->value.asStatement.attributes.asCommandHandler->name);
}

WASM_EXPORT_AS("addCommand") void addCommand(catto_Context* context, const char* name) {
    catto_addCommand(context, name, handleCommand);
}

WASM_EXPORT_AS("getArgType") char* getArgType(catto_Context* context, catto_AstNode* astNode) {
    if (!astNode) {
        return CATTO_NULL;
    }

    catto_TypedValue value = catto_evalExpression(context, astNode);

    switch (value.type) {
        case CATTO_DATA_TYPE_NULL: return "null";
        case CATTO_DATA_TYPE_NUMBER: return "number";
        case CATTO_DATA_TYPE_STRING: return "string";
        case CATTO_DATA_TYPE_FUNCTION: return "function";
        case CATTO_DATA_TYPE_LIST: return "list";
        default: return CATTO_NULL;
    }
}

WASM_EXPORT_AS("evalArgAsNumber") double evalArgAsNumber(catto_Context* context, catto_AstNode* astNode) {
    if (!astNode) {
        return 0;
    }

    return catto_asNumber(catto_evalExpression(context, astNode));
}

WASM_EXPORT_AS("evalArgAsString") char* evalArgAsString(catto_Context* context, catto_AstNode* astNode) {
    if (!astNode) {
        return catto_copyString("");
    }

    return catto_asString(catto_evalExpression(context, astNode));
}

WASM_EXPORT_AS("evalArgAsBool") bool evalArgAsBool(catto_Context* context, catto_AstNode* astNode) {
    if (!astNode) {
        return CATTO_FALSE;
    }

    return catto_asBool(catto_evalExpression(context, astNode));
}

WASM_EXPORT_AS("assignArgAsNumber") void assignArgAsNumber(catto_Context* context, catto_AstNode* astNode, double value) {
    if (!astNode) {
        return;
    }

    catto_TypedValue typedValue = {
        .type = CATTO_DATA_TYPE_NUMBER,
        .value.asNumber = value
    };

    catto_assignValue(context, astNode, typedValue);
}

WASM_EXPORT_AS("assignArgAsString") void assignArgAsString(catto_Context* context, catto_AstNode* astNode, char* value) {
    if (!astNode) {
        return;
    }

    catto_TypedValue typedValue = {
        .type = CATTO_DATA_TYPE_STRING,
        .value.asString = value
    };

    catto_assignValue(context, astNode, typedValue);
}

WASM_EXPORT_AS("assignArgAsBool") void assignArgAsBool(catto_Context* context, catto_AstNode* astNode, bool value) {
    if (!astNode) {
        return;
    }

    catto_TypedValue typedValue = {
        .type = CATTO_DATA_TYPE_NUMBER,
        .value.asNumber = value ? 1 : 0
    };

    catto_assignValue(context, astNode, typedValue);
}