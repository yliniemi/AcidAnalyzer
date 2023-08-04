#include <ringBuffer.h>

void initializeBufferWithChunksSize(struct RingBuffer *rb, int64_t numberOfBuffers, int64_t size, int64_t elementSize, int64_t chunkSize)
{
    if (rb->buffers != NULL)
    {
        for (int64_t i = 0; i < rb->numberOfBuffers; i++)
        {
            if (rb->buffers[i] != NULL) free(rb->buffers[i]);
        }
        free(rb->buffers);
    }
    rb->numberOfBuffers = numberOfBuffers;
    rb->elementSize = elementSize;
    rb->chunkSize = chunkSize;
    rb->size = size + rb->chunkSize + 1;
    rb->readIndex = 0;
    rb->writeIndex = 0;
    rb->buffers = calloc(rb->numberOfBuffers, sizeof(uint8_t*));
    rb->partialWrite = false;
    rb->partialRead = false;
    rb->tooMuch = 0;
    rb->tooLittle = 0;
    for (int64_t i = 0; i < rb->numberOfBuffers; i++)
    {
        rb->buffers[i] = (uint8_t*)calloc(rb->size * rb->elementSize, sizeof(uint8_t));
    }
}

void initializeBuffer(struct RingBuffer *rb, int64_t numberOfBuffers, int64_t size, int64_t elementSize)
{
    initializeBufferWithChunksSize(rb, numberOfBuffers, size, elementSize, 0);
}

bool isBufferEmpty(struct RingBuffer* rb)
{
    return (rb->writeIndex == rb->readIndex);
}

bool isBufferFull(struct RingBuffer* rb)
{
    return !((rb->size - rb->writeIndex + rb->readIndex - 1) % rb->size);
}

int64_t getBufferWriteSpace(struct RingBuffer* rb)
{
    return (rb->size - rb->writeIndex + rb->readIndex - 1) % rb->size;
}

int64_t increaseBufferWriteIndex(struct RingBuffer* rb, int64_t count)
{
    int64_t space = (rb->size - rb->writeIndex + rb->readIndex - 1) % rb->size;
    int64_t toCopy = count > space ? space : count;

    if (count > space && !rb->partialWrite)
    {
        rb->tooMuch++;
        return -495232356;
    }
    
    if (toCopy <= 0)
    {
        rb->tooMuch++;
        return toCopy;
    }

    if (rb->writeIndex + toCopy <= rb->size)
    {
        rb->writeIndex += toCopy;
    }
    else
    {
        int64_t firstHalfSize = rb->size - rb->writeIndex;
        int64_t secondHalfSize = toCopy - firstHalfSize;
        rb->writeIndex = secondHalfSize;
    }
    return toCopy;
}

int64_t writeBuffer(struct RingBuffer* rb, uint8_t *data, int64_t bufferNumber, int64_t count)
{
    int64_t space = (rb->size - rb->writeIndex + rb->readIndex - 1) % rb->size;
    int64_t toCopy = count > space ? space : count;
    if (bufferNumber > rb->numberOfBuffers - 1)
    {
        return -123456789;
    }

    if (count > space && !rb->partialWrite)  // rb->partialWrite will cause problems. perhaps i whould deprecate it
    {
        rb->tooMuch++;
        return -495232356;
    }

    if (toCopy <= 0)
    {
        rb->tooMuch++;
        return toCopy;
    }
    static int64_t writeTimes = 0;
    if (rb->writeIndex + toCopy <= rb->size)
    {
        memcpy(&rb->buffers[bufferNumber][rb->writeIndex * rb->elementSize], &data[0], toCopy * rb->elementSize);
    }
    else
    {
        int64_t firstHalfSize = rb->size - rb->writeIndex;
        memcpy(&rb->buffers[bufferNumber][rb->writeIndex * rb->elementSize], &data[0], firstHalfSize * rb->elementSize);

        int64_t secondHalfSize = toCopy - firstHalfSize;
        memcpy(&rb->buffers[bufferNumber][0], &data[firstHalfSize * rb->elementSize], secondHalfSize * rb->elementSize);
    }
    return toCopy;
}

int64_t getBufferReadSpace(struct RingBuffer* rb)
{
    return (rb->size + rb->writeIndex - rb->readIndex) % rb->size - rb->chunkSize;
}

int64_t increaseBufferReadIndex(struct RingBuffer *rb, int64_t count)
{
    int64_t available = (rb->size + rb->writeIndex - rb->readIndex) % rb->size - rb->chunkSize;
    int64_t toRead = count > available ? available : count;

    if (count > available && !rb->partialRead)  // rb->partialRead will cause problems. perhaps i whould deprecate it
    {
        rb->tooLittle++;
        return -895232356;
    }
    
    if (toRead <= 0)
    {
        rb->tooLittle++;
        return toRead;
    }

    static int64_t readTimes = 0;
    if (rb->readIndex + toRead <= rb->size)
    {
        rb->readIndex += toRead;
    }
    else
    {
        int64_t firstHalfSize = rb->size - rb->readIndex;
        int64_t secondHalfSize = toRead - firstHalfSize;
        rb->readIndex = secondHalfSize;
    }
    return toRead;
}

int64_t readBuffer(struct RingBuffer *rb, uint8_t *destination, int64_t bufferNumber, int64_t count)
{
    int64_t available = (rb->size + rb->writeIndex - rb->readIndex) % rb->size - rb->chunkSize;
    int64_t toRead = count > available ? available : count;

    if (bufferNumber > rb->numberOfBuffers - 1)
    {
        return -987654321;
    }

    if (count > available && !rb->partialRead)  // rb->partialRead will cause problems. perhaps i whould deprecate it
    {
        rb->tooLittle++;
        return -895232356;
    }
    
    if (toRead <= 0)
    {
        rb->tooLittle++;
        return toRead;
    }

    static int64_t readTimes = 0;
    if (rb->readIndex + toRead <= rb->size)
    {
        memcpy(&destination[0], &rb->buffers[bufferNumber][rb->readIndex * rb->elementSize], toRead * rb->elementSize);
    }
    else
    {
        int64_t firstHalfSize = rb->size - rb->readIndex;
        memcpy(&destination[0], &rb->buffers[bufferNumber][rb->readIndex * rb->elementSize], firstHalfSize * rb->elementSize);

        int64_t secondHalfSize = toRead - firstHalfSize;
        memcpy(&destination[firstHalfSize * rb->elementSize], &rb->buffers[bufferNumber][0], secondHalfSize * rb->elementSize);

    }
    return toRead;
}

int64_t readChunkFromBuffer(struct RingBuffer *rb, uint8_t *destination, int64_t bufferNumber)
{
    int64_t available = (rb->size + rb->writeIndex - rb->readIndex) % rb->size;
    int64_t toRead = rb->chunkSize > available ? available : rb->chunkSize;

    if (bufferNumber > rb->numberOfBuffers - 1)
    {
        return -987654321;
    }

    if (rb->chunkSize > available && !rb->partialRead)
    {
        rb->tooLittle++;
        return -895232356;
    }
    
    if (toRead <= 0)
    {
        rb->tooLittle++;
        return toRead;
    }

    static int64_t readTimes = 0;
    if (rb->readIndex + toRead <= rb->size)
    {
        memcpy(&destination[0], &rb->buffers[bufferNumber][rb->readIndex * rb->elementSize], toRead * rb->elementSize);
    }
    else
    {
        int64_t firstHalfSize = rb->size - rb->readIndex;
        memcpy(&destination[0], &rb->buffers[bufferNumber][rb->readIndex * rb->elementSize], firstHalfSize * rb->elementSize);

        int64_t secondHalfSize = toRead - firstHalfSize;
        memcpy(&destination[firstHalfSize * rb->elementSize], &rb->buffers[bufferNumber][0], secondHalfSize * rb->elementSize);

    }
    return toRead;
}

void freeBuffer(struct RingBuffer* rb)
{
    if (rb->buffers != NULL)
    {    
        for (int64_t i = 0; i < rb->numberOfBuffers; i++)
        {
            if (rb->buffers[i] != NULL) free(rb->buffers[i]);
        }
        free(rb->buffers);
    }
    rb->numberOfBuffers = 0;
    rb->readIndex = 0;
    rb->writeIndex = 0;
    rb->chunkSize = 0;
}

