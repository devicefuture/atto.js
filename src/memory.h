#ifndef MEMORY_H_
#define MEMORY_H_

typedef unsigned long size_t;
typedef size_t Block;
typedef char bool;

extern Block* base;
extern Block* firstFreeBlock;

void* memset(void* destination, int value, size_t size);
void* memcpy(void* destination, const void* source, size_t size);
void* malloc(size_t size);
void free(void* ptr);
void* calloc(size_t count, size_t size);
void* realloc(void* ptr, size_t size);

#endif