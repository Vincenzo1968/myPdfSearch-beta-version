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

#ifndef MYTIME_H_INCLUDED
#define MYTIME_H_INCLUDED

#ifdef __cplusplus
    #define BEGIN_C_DECLS extern "C" {
    #define END_C_DECLS   }
#else
    #define BEGIN_C_DECLS
    #define END_C_DECLS
#endif

// Keep the platform headers previously exposed by this header, for source
// compatibility with existing project files that may rely on them.
#if defined(_WIN64) || defined(_WIN32)
    #include <windows.h>
#elif defined(__unix__) || defined(__unix) || defined(unix) || \
      (defined(__APPLE__) && defined(__MACH__))
    #include <unistd.h>
    #include <sys/times.h>   // fallback when CLOCK_MONOTONIC is unavailable
    #include <sys/resource.h>
    #include <sys/time.h>
    #include <time.h>
#else
    #error "Unable to define getTimes(myTimes *pTimes) for an unknown OS."
#endif

#include <stdio.h>
#include <stdlib.h>

typedef struct _myTimes
{
    double realTime;
    double userTime;
    double systemTime;
} myTimes;

BEGIN_C_DECLS

int getTimes(myTimes *pTimes);

END_C_DECLS

#endif // MYTIME_H_INCLUDED
