
#define THIS_IS_A_TEST
#include "main.c"
#include <kaiser.h>


int64_t testKaiser()
{
    int64_t N = 4096;  // Window length
    double alpha = 6.815;  // Kaiser parameter

    double window[N];
    generateKaiserWindow(N, alpha * PI, window);
    fprintf(stdout, "Kaiser Window:\n");
    fprintf(stdout, "\ndouble kaiser_alpha_6.815_4096[] = \n{ ");
    for (int64_t i = 0; i < N; i++) {
        if (i % 4 == 0) fprintf(stdout, "\n");
        fprintf(stdout, "%.17e, ", window[i]);
    }
    fprintf(stdout, "\b\b  \n}\n\n");
    fflush(stdout);

    return 0;
}

int64_t testRatio()
{
    fprintf(stdout, "findRatio  = %.17f\n", findRatio(15, 0, 1, 0, 3, 2048, 128));
    fprintf(stdout, "findRatio  = %.17f\n", findRatio(60, 0, 1, 0, 110, 2 * 131072, 208));
}

struct RingBuffer rb;
int64_t testRingBuffer()
{
    fprintf(stdout, "about to initialize buffer\n");
    initializeBuffer(&rb, 1, 32, sizeof(int));
    fprintf(stdout, "initialized buffer\n");
    for (int64_t i = 0; i < 10; i++)
    {
        int64_t writeInts[] = {0, 1, 2, 3, 4, 5, 6, 7};
        writeBuffer(&rb, (uint8_t*)writeInts, 0, 8);
        increaseBufferWriteIndex(&rb, 8);
        fprintf(stdout, "wrote\n");
        int64_t readInts[8];
        fprintf(stdout, "\n");
        readBuffer(&rb, (uint8_t*)readInts, 0, 8);
        increaseBufferReadIndex(&rb, 8);
        fprintf(stdout, "read\n");
        for (int64_t j = 0; j < 8; j++)
        {
            fprintf(stdout, "%lld, ", readInts[j]);
        }
        fprintf(stdout, "\n");
    }
    fflush(stdout);
}

int64_t main(int64_t argc, char *argv[])
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
