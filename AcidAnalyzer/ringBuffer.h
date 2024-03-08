#pragma once

#include <stdbool.h>
#include <stdint.h>

struct RingBuffer
{
    int64_t size;
    int64_t elementSize;
    int64_t chunkSize;
    int64_t numberOfBuffers;
    int64_t readIndex;
    int64_t writeIndex;
    uint8_t **buffers;
    bool partialWrite;
    bool partialRead;
    int64_t tooMuch;
    int64_t tooLittle;
};

void initializeBufferWithChunksSize(struct RingBuffer *rb, int64_t numberOfBuffers, int64_t size, int64_t elementSize, int64_t chunkSize);

void initializeBuffer(struct RingBuffer *rb, int64_t numberOfBuffers, int64_t size, int64_t elementSize);

bool isBufferEmpty(struct RingBuffer* rb);

bool isBufferFull(struct RingBuffer* rb);

int64_t getBufferWriteSpace(struct RingBuffer* rb);

int64_t increaseBufferWriteIndex(struct RingBuffer* rb, int64_t count);

int64_t writeBuffer(struct RingBuffer* rb, uint8_t *data, int64_t bufferNumber, int64_t count);

int64_t getBufferReadSpace(struct RingBuffer* rb);

int64_t increaseBufferReadIndex(struct RingBuffer *rb, int64_t count);

int64_t readBuffer(struct RingBuffer *rb, uint8_t *destination, int64_t bufferNumber, int64_t count);

int64_t readChunkFromBuffer(struct RingBuffer *rb, uint8_t *destination, int64_t bufferNumber);

void freeBuffer(struct RingBuffer* rb);

