#include "global.h"
#include "malloc.h"
#include "mgba.h"

#include <stdio.h>

static void *sHeapStart;
static u32 sHeapSize;
static u32 malloc_c_unused_0300000c; // needed to align dma3_manager.o(.bss)

#define MALLOC_SYSTEM_ID 0xA3A3

// sizeof(MemBlock) is 16
struct MemBlock {
    // Whether this block is currently allocated.
    bool16 flag;

    // Magic number used for error checking. Should equal MALLOC_SYSTEM_ID.
    u16 magic;

    // Size of the block (not including this header struct).
    u32 size;

    // Previous block pointer. Equals sHeapStart if this is the first block.
    struct MemBlock *prev;

    // Next block pointer. Equals sHeapStart if this is the last block.
    struct MemBlock *next;

    // Data in the memory block. (Arrays of length 0 are a GNU extension.)
    u8 data[0];
};

void PutMemBlockHeader(void *block, struct MemBlock *prev, struct MemBlock *next, u32 size)
{
    struct MemBlock *header = (struct MemBlock *)block;

    header->flag = FALSE;
    header->magic = MALLOC_SYSTEM_ID;
    header->size = size;
    header->prev = prev;
    header->next = next;
}

void PutFirstMemBlockHeader(void *block, u32 size)
{
    struct MemBlock *header = (struct MemBlock *)block;
    PutMemBlockHeader(block, (struct MemBlock *)block, (struct MemBlock *)block, size - sizeof(struct MemBlock));
    // mgba_printf(MGBA_LOG_INFO, "First block size: %d", header->size);
}

void *AllocInternal(void *heapStart, u32 size)
{
    struct MemBlock *pos = (struct MemBlock *)heapStart;
    struct MemBlock *head = pos;
    struct MemBlock *splitBlock;
    u32 foundBlockSize;
    u32 allocSpace = 0;

    // Alignment
    if (size & 3)
        size = 4 * ((size / 4) + 1);
    // mgba_printf(MGBA_LOG_INFO, "Requested size: %d", size);

    for (;;) {
        // Loop through the blocks looking for unused block that's big enough.

        if (!pos->flag) {
            foundBlockSize = pos->size;

            if (foundBlockSize >= size) {
                if (foundBlockSize - size < 2 * sizeof(struct MemBlock)) {
                    // The block isn't much bigger than the requested size,
                    // so just use it.
                    pos->flag = TRUE;
                    allocSpace += foundBlockSize;
                } else {
                    // The block is significantly bigger than the requested
                    // size, so split the rest into a separate block.
                    foundBlockSize -= sizeof(struct MemBlock);
                    foundBlockSize -= size;
                    allocSpace += sizeof(struct MemBlock) + size;

                    splitBlock = (struct MemBlock *)(pos->data + size);

                    pos->flag = TRUE;
                    pos->size = size;

                    PutMemBlockHeader(splitBlock, pos, pos->next, foundBlockSize);

                    pos->next = splitBlock;

                    if (splitBlock->next != head)
                        splitBlock->next->prev = splitBlock;
                }

                // mgba_printf(MGBA_LOG_INFO, "Actual alloc: %d", allocSpace);

                return pos->data;
            }
        }

        if (pos->next == head)
            return NULL;

        pos = pos->next;
    }
}

void FreeInternal(void *heapStart, void *pointer)
{
    if (pointer) {
        struct MemBlock *head = (struct MemBlock *)heapStart;
        struct MemBlock *block = (struct MemBlock *)((u8 *)pointer - sizeof(struct MemBlock));
        block->flag = FALSE;
        u32 freedSpace = 0;
        u32 blockSize = block->size;

        // mgba_printf(MGBA_LOG_INFO, "Request free: %d", block->size);
        // mgba_printf(MGBA_LOG_INFO, "block->next: %X", block->next);
        // mgba_printf(MGBA_LOG_INFO, "block->next->flag: %X", block->next->flag);
        // mgba_printf(MGBA_LOG_INFO, "block->prev->flag: %X", block->prev->flag);

        // If the freed block isn't the last one, merge with the next block
        // if it's not in use.
        if (block->next != head) {
            if (!block->next->flag) {
                freedSpace += sizeof(struct MemBlock);
                block->size += sizeof(struct MemBlock) + block->next->size;
                block->next->magic = 0;
                block->next = block->next->next;
                if (block->next != head)
                    block->next->prev = block;
            }
        }

        // If the freed block isn't the first one, merge with the previous block
        // if it's not in use.
        if (block != head) {
            if (!block->prev->flag) {
                freedSpace += sizeof(struct MemBlock);
                block->prev->next = block->next;

                if (block->next != head)
                    block->next->prev = block->prev;

                block->magic = 0;
                block->prev->size += sizeof(struct MemBlock) + block->size;
            }
        }

        if (freedSpace > 0) {
            freedSpace += blockSize;
        }
        // mgba_printf(MGBA_LOG_INFO, "Actual freed: %d", freedSpace);
    }
}

void *AllocZeroedInternal(void *heapStart, u32 size)
{
    void *mem = AllocInternal(heapStart, size);

    if (mem != NULL) {
        if (size & 3)
            size = 4 * ((size / 4) + 1);

        CpuFill32(0, mem, size);
    }

    return mem;
}

bool32 CheckMemBlockInternal(void *heapStart, void *pointer)
{
    struct MemBlock *head = (struct MemBlock *)heapStart;
    struct MemBlock *block = (struct MemBlock *)((u8 *)pointer - sizeof(struct MemBlock));

    if (block->magic != MALLOC_SYSTEM_ID)
        return FALSE;

    if (block->next->magic != MALLOC_SYSTEM_ID)
        return FALSE;

    if (block->next != head && block->next->prev != block)
        return FALSE;

    if (block->prev->magic != MALLOC_SYSTEM_ID)
        return FALSE;

    if (block->prev != head && block->prev->next != block)
        return FALSE;

    if (block->next != head && block->next != (struct MemBlock *)(block->data + block->size))
        return FALSE;

    return TRUE;
}

void InitHeap(void *heapStart, u32 heapSize)
{
    mgba_printf(MGBA_LOG_INFO, "Init heap. Addr: %X, size: %d", heapStart, heapSize);
    sHeapStart = heapStart;
    sHeapSize = heapSize;
    PutFirstMemBlockHeader(heapStart, heapSize);
}

void *Alloc(u32 size)
{
    return AllocInternal(sHeapStart, size);
}

void *AllocZeroed(u32 size)
{
    return AllocZeroedInternal(sHeapStart, size);
}

void *Realloc(void *pointer, u32 size) 
{
    struct MemBlock *block = (struct MemBlock *)((u8 *)pointer - sizeof(struct MemBlock));
    u32 originalSize = block->size;
    void *newPointer;

    if (size == 0) {
        Free(pointer);
    } else if (!pointer) {
        return Alloc(size);
    } else if (size <= originalSize) {
        return pointer;
    } else {
        newPointer = Alloc(size);
        if (newPointer) {
            memcpy(newPointer, pointer, originalSize);
            Free(pointer);
        }
        return newPointer;
    }
}

void Free(void *pointer)
{
    FreeInternal(sHeapStart, pointer);
}

bool32 CheckMemBlock(void *pointer)
{
    return CheckMemBlockInternal(sHeapStart, pointer);
}

bool32 CheckHeap()
{
    struct MemBlock *pos = (struct MemBlock *)sHeapStart;

    do {
        if (!CheckMemBlockInternal(sHeapStart, pos->data))
            return FALSE;
        pos = pos->next;
    } while (pos != (struct MemBlock *)sHeapStart);

    return TRUE;
}

void PrintHeap(void) {
    char buffer[500] = "";
    char address[16] = "";
    char *text = buffer;
    struct MemBlock *pos = (struct MemBlock *)sHeapStart;
    struct MemBlock *head = pos;

    do {
        strcat(buffer, "{");
        sprintf(address, "%X", pos);
        strncat(buffer, address, 10);
        strcat(buffer, " ");
        sprintf(address, "%d", pos->flag);
        strncat(buffer, address, 1);
        strcat(buffer, "} ");

        pos = pos->next;
    } while(pos != head);

    mgba_printf(MGBA_LOG_INFO, buffer);
}

u32 CalcHeapUsed(void)
{
    struct MemBlock *pos = (struct MemBlock *)sHeapStart;
    struct MemBlock *head = pos;
    u32 spaceUsed = 0;

    do
    {
        spaceUsed += sizeof(struct MemBlock);
        if (pos->flag) {
            spaceUsed += pos->size;
        }
        pos = pos->next;
    } while (pos != head);

    return spaceUsed;
}