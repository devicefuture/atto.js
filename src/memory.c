#include "memory.h"

#define NULL 0
#define true 1
#define false 0

#define ALIGN_SIZE 4
#define ALIGN(value) (((value) + ALIGN_SIZE - 1) & ~(ALIGN_SIZE - 1))

#define FLAG_USED 0x8000
#define BLOCK_IS_USED(address) (*(address) & FLAG_USED)
#define BLOCK_SIZE(address) (*(address) & (FLAG_USED - 1))
#define BLOCK_IS_LAST(address) (*(address) == 0)

#define DEBUG_HEAP_LOG(message)

Block* base;
Block* firstFreeBlock;

void (*heapLogger)(char*);

void* memset(void* destination, int value, size_t size) {
    for (size_t i = 0; i < size; i++) {
        *((char*)destination + size) = value;
    }

    return destination;
}

void* memcpy(void* destination, const void* source, size_t size) {
    for (size_t i = 0; i < size; i++) {
        *((char*)destination + i) = *((char*)source + i);
    }

    return destination;
}

void* malloc(size_t size) {
    DEBUG_HEAP_LOG("> malloc");

    if (size == 0) {
        size = ALIGN_SIZE;
    }

    if (size & FLAG_USED) {
        return NULL;
    }

    size = ALIGN(size);

    Block* currentBlock = firstFreeBlock;

    while (true) {
        if (BLOCK_IS_LAST(currentBlock)) {
            DEBUG_HEAP_LOG("Use last block");

            *currentBlock = FLAG_USED | size;
            *(currentBlock + sizeof(Block) + size) = 0; // Create a new last block after this one

            if (currentBlock == firstFreeBlock) {
                DEBUG_HEAP_LOG("  Set next free block");

                firstFreeBlock = currentBlock + sizeof(Block) + size;
            }

            return currentBlock + sizeof(Block);
        }

        size_t originalBlockSize = BLOCK_SIZE(currentBlock);

        if (BLOCK_IS_USED(currentBlock)) {
            // Skip over this block if it's used

            DEBUG_HEAP_LOG("Skip used");

            currentBlock += sizeof(Block) + originalBlockSize;

            continue;
        }

        if (originalBlockSize < size) {
            // Try and merge subsequent blocks to meet required size; if not, then skip

            Block* originalBlock = currentBlock;
            size_t totalUsableSize = originalBlockSize;
            bool encounteredLastBlock = false;

            DEBUG_HEAP_LOG("Attempt merge");

            while (true) {
                currentBlock += sizeof(Block) + BLOCK_SIZE(currentBlock);

                if (BLOCK_IS_USED(currentBlock)) {
                    DEBUG_HEAP_LOG("  Encountered used block");

                    break;
                }

                if (BLOCK_IS_LAST(currentBlock)) {
                    // Looks like we can use the rest of the available memory

                    DEBUG_HEAP_LOG("  Encountered last block");

                    encounteredLastBlock = true;

                    break;
                }

                totalUsableSize += *currentBlock + sizeof(Block);
            }

            if (encounteredLastBlock) {
                DEBUG_HEAP_LOG("  Using last block");

                currentBlock = originalBlock;

                *(currentBlock + sizeof(Block) + size) = 0; // Create a new last block after this one
                originalBlockSize = totalUsableSize;
            } else {
                *originalBlock = totalUsableSize;

                if (totalUsableSize < size + sizeof(Block)) {
                    continue;
                }

                DEBUG_HEAP_LOG("  Use merged block");

                currentBlock = originalBlock;
                originalBlockSize = totalUsableSize;
            }
        }

        if (originalBlockSize > size + sizeof(Block) + ALIGN_SIZE) {
            // Split this block so that the rest can be used for other purposes

            DEBUG_HEAP_LOG("Split block");

            *currentBlock = size;
            *(currentBlock + sizeof(Block) + size) = originalBlockSize - size - sizeof(Block);
        }

        DEBUG_HEAP_LOG("Mark as used");

        *currentBlock |= FLAG_USED;

        if (currentBlock == firstFreeBlock) {
            DEBUG_HEAP_LOG("Search for a new first free block");

            while (BLOCK_IS_USED(firstFreeBlock)) {
                DEBUG_HEAP_LOG("  Search next");

                firstFreeBlock += BLOCK_SIZE(firstFreeBlock) + sizeof(Block);
            }
        }

        return currentBlock + sizeof(Block);
    }
}

void free(void* ptr) {
    if (!ptr) {
        return;
    }

    Block* block = (Block*)ptr - sizeof(Block);

    DEBUG_HEAP_LOG("> free");

    *block &= ~FLAG_USED;

    if (block < firstFreeBlock) {
        DEBUG_HEAP_LOG("Set first free block");

        firstFreeBlock = block;
    }

    Block* currentBlock = block + sizeof(Block) + *block;

    // Merge together any subsequent free blocks
    while (!BLOCK_IS_USED(currentBlock)) {
        if (BLOCK_IS_LAST(currentBlock)) {
            DEBUG_HEAP_LOG("Convert to last block");

            *block = 0; // Make the penultimate block the last one instead

            return;
        }

        DEBUG_HEAP_LOG("Merge subsequent block");

        *block += BLOCK_SIZE(currentBlock) + sizeof(Block);
        currentBlock += BLOCK_SIZE(currentBlock) + sizeof(Block);
    }
}

void* calloc(size_t count, size_t size) {
    void* memory = malloc(count * size);

    if (memory) {
        memset(memory, '\0', count * size);
    }

    return memory;
}

void* realloc(void* ptr, size_t size) {
    DEBUG_HEAP_LOG("> realloc");

    if (size & FLAG_USED) {
        return NULL;
    }

    if (!ptr) {
        return malloc(size);
    }

    if (size == 0) {
        size = ALIGN_SIZE;
    }

    size = ALIGN(size);

    Block* blockPtr = (Block*)ptr;
    Block* block = blockPtr - sizeof(Block);

    if (size == BLOCK_SIZE(block)) {
        DEBUG_HEAP_LOG("Equal size");

        return ptr;
    }

    if (BLOCK_IS_LAST(blockPtr + BLOCK_SIZE(block))) {
        // Modify last block to match new size

        DEBUG_HEAP_LOG("Modify last block");

        if (blockPtr + BLOCK_SIZE(block) == firstFreeBlock) {
            // Push pointer to first free block further down if it currently lies in the extended space

            DEBUG_HEAP_LOG("  Increase first free block");

            firstFreeBlock = blockPtr + size;
        }

        *block = FLAG_USED | size;
        *(blockPtr + size) = 0; // Set new position for last block

        return ptr;
    }

    if (size + sizeof(Block) < BLOCK_SIZE(block)) {
        // Truncate current block by splitting truncated portion off into new block

        DEBUG_HEAP_LOG("Truncate block");

        *(blockPtr + size) = BLOCK_SIZE(block) - size - sizeof(Block);
        *block = FLAG_USED | size;

        return ptr;
    }

    // Perform full reallocation

    DEBUG_HEAP_LOG("Fully reallocate");

    void* newPtr = malloc(size);

    if (!newPtr) {
        return NULL;
    }

    memcpy(newPtr, ptr, BLOCK_SIZE(block));
    free(ptr);

    return newPtr;
}