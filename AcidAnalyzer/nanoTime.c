/*
    __linux__       Defined on Linux
    __sun           Defined on Solaris
    __FreeBSD__     Defined on FreeBSD
    __NetBSD__      Defined on NetBSD
    __OpenBSD__     Defined on OpenBSD
    __APPLE__       Defined on Mac OS X
    __hpux          Defined on HP-UX
    __osf__         Defined on Tru64 UNIX (formerly DEC OSF1)
    __sgi           Defined on Irix
    _AIX            Defined on AIX
    _WIN32          Defined on Windows
*/


#include <nanoTime.h>

#if defined(__linux__) || defined(__APPLE__)
#include <time.h>
#endif

#ifdef _WIN32
#include <sys\timeb.h>
// #include <windows.h>
#endif

#ifdef __APPLE__

#endif


int64_t nanoTime()
{
    #if defined(__linux__) || defined(__APPLE__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (int64_t)ts.tv_sec * 1000000000 + (int64_t)ts.tv_nsec;
    #endif
    
    #ifdef _WIN32
    return GetTickCount() * 1000000;
    #endif
}

int64_t nanoSleepUniversal(int64_t nanoSeconds)
{
    #if defined(__linux__) || defined(__APPLE__)
    struct timespec requested = {nanoSeconds / 1000000000, nanoSeconds % 1000000000};
    struct timespec remaining;
    nanosleep(&requested, &remaining);
    return (int64_t)remaining.tv_sec * 1000000000 + (int64_t)remaining.tv_nsec;
    #endif
    
    #ifdef _WIN32
    // Windows can't sleep for less than 16 milliseconds unless a program changes the granulatiry of the timer
    Sleep(nanoSeconds / 1000000);
    return 0;
    #endif
}

