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

WASM_EXPORT_AS("newContext") catto_Context* catto_newContext();
WASM_EXPORT_AS("freeContext") void catto_freeContext(catto_Context* context);
WASM_EXPORT_AS("step") catto_Bool catto_step(catto_Context* context);
WASM_EXPORT_AS("load") void catto_load(catto_Context* context, const catto_Char* code);

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