
#define THIS_IS_A_TEST
#include "main.c"


int testKaiser()
{
    int N = 4096;  // Window length
    double alpha = 3.0;  // Kaiser parameter

    double window[N];
    generateKaiserWindow(N, alpha * PI, window);
    fprintf(stdout, "Kaiser Window:\n");
    fprintf(stdout, "\ndouble kaiser_3_4096[] = \n{ ");
    for (int i = 0; i < N; i++) {
        if (i % 8 == 0) fprintf(stdout, "\n");
        fprintf(stdout, "%.17e, ", window[i]);
    }
    fprintf(stdout, "\b\b  \n}\n\n");
    fflush(stdout);

    return 0;
}

int testRatio()
{
    fprintf(stdout, "findRatio  = %.17f\n", findRatio(15, 0, 1, 0, 3, 2048, 128));
}

struct RingBuffer rb;
int testRingBuffer()
{
    fprintf(stdout, "about to initialize buffer\n");
    initializeBuffer(&rb, 1, 32, sizeof(int));
    fprintf(stdout, "initialized buffer\n");
    for (int i = 0; i < 10; i++)
    {
        int writeInts[] = {0, 1, 2, 3, 4, 5, 6, 7};
        writeBuffer(&rb, (uint8_t*)writeInts, 0, 8);
        increaseBufferWriteIndex(&rb, 8);
        fprintf(stdout, "wrote\n");
        int readInts[8];
        fprintf(stdout, "\n");
        readBuffer(&rb, (uint8_t*)readInts, 0, 8);
        increaseBufferReadIndex(&rb, 8);
        fprintf(stdout, "read\n");
        for (int j = 0; j < 8; j++)
        {
            fprintf(stdout, "%d, ", readInts[j]);
        }
        fprintf(stdout, "\n");
    }
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    fprintf(stdout, "sizeof(int*) = %zu\n\n", sizeof(int*));
    testKaiser();
    fprintf(stdout, "\n\n");
    testRingBuffer();
    fprintf(stdout, "\n\n");
    testRatio();
    fprintf(stdout, "\n\n");
    // struct timespec rem;
    // struct timespec req= {5, 0};
    // nanosleep(&req, &rem);

    return 0;
}