/*
   Copyright (C) 2026 Vincenzo Lo Cicero

   Author: Vincenzo Lo Cicero.
   e-mail: vincenzolocicero68@gmail.com

   This file is part of mypdfsearch.

   mypdfsearch is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   mypdfsearch is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with mypdfsearch.  If not, see <http://www.gnu.org/licenses/>.
*/


// Feature-test macro: expose clock_gettime() and CLOCK_MONOTONIC on POSIX.
// It must be defined before including any system header.

#if !defined(_WIN64) && !defined(_WIN32)
    #ifndef _POSIX_C_SOURCE
        #define _POSIX_C_SOURCE 200809L
    #endif
#endif

#include "mytime.h"

#include <stddef.h>

#if defined(_WIN64) || defined(_WIN32)

    #include <windows.h>

    // FILETIME stores a 64-bit count of 100-nanosecond intervals.
    // GetProcessTimes() returns CPU times accumulated by all process threads.

    static double FileTimeToSeconds(const FILETIME *pFileTime)
    {
        ULARGE_INTEGER value;

        value.LowPart = pFileTime->dwLowDateTime;
        value.HighPart = pFileTime->dwHighDateTime;

        return (double)value.QuadPart / 10000000.0;
    }

#elif defined(__unix__) || defined(__unix) || defined(unix) || \
      (defined(__APPLE__) && defined(__MACH__))

    #include <sys/resource.h>
    #include <sys/time.h>
    #include <time.h>

    // Fallback headers for POSIX systems without CLOCK_MONOTONIC.
    #if !defined(CLOCK_MONOTONIC)
        #include <sys/times.h>
        #include <unistd.h>
    #endif

#else
    #error "Unable to define getTimes(myTimes *pTimes) for an unknown OS."
#endif

int getTimes(myTimes *pTimes)
{
    if (pTimes == NULL)
        return 0;

#if defined(_WIN64) || defined(_WIN32) // Windows: monotonic elapsed time + process-wide CPU times.
    {
        LARGE_INTEGER frequency;
        LARGE_INTEGER counter;
        FILETIME creationTime;
        FILETIME exitTime;
        FILETIME kernelTime;
        FILETIME userTime;

        if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0)
            return 0;

        if (!QueryPerformanceCounter(&counter))
            return 0;

        if (!GetProcessTimes(GetCurrentProcess(),
                             &creationTime,
                             &exitTime,
                             &kernelTime,
                             &userTime))
        {
            return 0;
        }

        pTimes->realTime = (double)counter.QuadPart / (double)frequency.QuadPart;
        pTimes->userTime = FileTimeToSeconds(&userTime);
        pTimes->systemTime = FileTimeToSeconds(&kernelTime);

        return 1;
    }

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__)) // POSIX: monotonic elapsed time + CPU times for the whole process.
    {
        double elapsedReal;
        double elapsedUser;
        double elapsedSystem;
        struct rusage usage;

    #if defined(CLOCK_MONOTONIC)
        struct timespec monotonicTime;

        if (clock_gettime(CLOCK_MONOTONIC, &monotonicTime) != 0)
            return 0;

        elapsedReal = (double)monotonicTime.tv_sec +
                      (double)monotonicTime.tv_nsec / 1000000000.0;
    #else
		{
			// Compatibility fallback for older POSIX systems. This has the
			// lower resolution of times(), unlike the CLOCK_MONOTONIC path.
            const long ticksPerSecond = sysconf(_SC_CLK_TCK);
            struct tms processTimes;
            clock_t realTicks;

            if (ticksPerSecond <= 0)
                return 0;

            realTicks = times(&processTimes);
            if (realTicks == (clock_t)-1)
                return 0;

            elapsedReal = (double)realTicks / (double)ticksPerSecond;
        }
    #endif

        if (getrusage(RUSAGE_SELF, &usage) != 0)
            return 0;

        elapsedUser = (double)usage.ru_utime.tv_sec +
                      (double)usage.ru_utime.tv_usec / 1000000.0;
        elapsedSystem = (double)usage.ru_stime.tv_sec +
                        (double)usage.ru_stime.tv_usec / 1000000.0;

        pTimes->realTime = elapsedReal;
        pTimes->userTime = elapsedUser;
        pTimes->systemTime = elapsedSystem;

        return 1;
    }
#endif

    return 0;
}
