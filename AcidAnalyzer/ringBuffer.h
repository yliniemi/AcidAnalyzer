#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h> // memcpy

struct RingBuffer
{
    int size;
    int elementSize;
    int chunkSize;
    int numberOfBuffers;
    int readIndex;
    int writeIndex;
    uint8_t **buffers;
    bool partialWrite;
    bool partialRead;
    int tooMuch;
    int tooLittle;
};

void initializeBufferWithChunksSize(struct RingBuffer *rb, int numberOfBuffers, int size, int elementSize, int chunkSize);

void initializeBuffer(struct RingBuffer *rb, int numberOfBuffers, int size, int elementSize);

bool isBufferEmpty(struct RingBuffer* rb);

bool isBufferFull(struct RingBuffer* rb);

int getBufferWriteSpace(struct RingBuffer* rb);

int increaseBufferWriteIndex(struct RingBuffer* rb, int count);

int writeBuffer(struct RingBuffer* rb, uint8_t *data, int bufferNumber, int count);

int getBufferReadSpace(struct RingBuffer* rb);

int increaseBufferReadIndex(struct RingBuffer *rb, int count);

int readBuffer(struct RingBuffer *rb, uint8_t *destination, int bufferNumber, int count);

int readChunkFromBuffer(struct RingBuffer *rb, uint8_t *destination, int bufferNumber);

void freeBuffer(struct RingBuffer* rb);

