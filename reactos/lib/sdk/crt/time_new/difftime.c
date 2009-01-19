/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/difftime.c
 * PURPOSE:     Implementation of difftime
 * PROGRAMERS:  Timo Kreuzer
 */
#include <time.h>
#include "bitsfixup.h"

/**
 * \name difftime
 * \brief Retrurns the difference between two time_t values in seconds.
 * \param time1 Beginning time.
 * \param time2 Ending time.
 */
double
difftime(
    time_t time1, /**< Beginning time */
    time_t time2) /**< Ending time */
{
    return (double)(time1 - time2);
}
