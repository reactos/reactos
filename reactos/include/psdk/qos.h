/*
 * qos.h
 *
 * Structures and definitions for QoS data types.
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Alex Ionescu <alex@relsoft.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __QOS_H
#define __QOS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef ULONG SERVICETYPE;

typedef struct _flowspec
{
    ULONG TokenRate;
    ULONG TokenBucketSize;
    ULONG PeakBandwidth;
    ULONG Latency;
    ULONG DelayVariation;
    SERVICETYPE	ServiceType;
    ULONG MaxSduSize;
    ULONG MinimumPolicedSize;
} FLOWSPEC, *PFLOWSPEC, *LPFLOWSPEC;

#ifdef __cplusplus
}
#endif

#endif
