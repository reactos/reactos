/*
 * Copyright (c) 2001
 *  Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _time_calls
#define _time_calls

#ifdef WIN_NT_DRIVER

#include "debug.h"

/*!
  \brief A microsecond precise timestamp.

  included in the sf_pkthdr or the bpf_hdr that NPF associates with every packet. 
*/

struct timeval {
        long    tv_sec;         ///< seconds
        long    tv_usec;        ///< microseconds
};

#endif /*WIN_NT_DRIVER*/

struct time_conv {
    ULONGLONG reference;
    struct timeval start;
};

#ifdef __GNUC__

void TIME_DESYNCHRONIZE(struct time_conv *data);
VOID TIME_SYNCHRONIZE(struct time_conv *data);
void FORCE_TIME(struct timeval *src, struct time_conv *dest);
void GET_TIME(struct timeval *dst, struct time_conv *data);

#else /* __GNUC__ */

#ifdef WIN_NT_DRIVER

__inline void TIME_DESYNCHRONIZE(struct time_conv *data)
{
    data->reference = 0;
    data->start.tv_sec = 0;
    data->start.tv_usec = 0;
}

#ifdef KQPC_TS

/* KeQueryPerformanceCounter TimeStamps */

__inline VOID TIME_SYNCHRONIZE(struct time_conv *data)
{
    struct timeval tmp;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER i;
    ULONG tmp2;
    LARGE_INTEGER TimeFreq,PTime;

    if (data->reference!=0)
        return;
    
    // get the absolute value of the system boot time.   
    PTime=KeQueryPerformanceCounter(&TimeFreq);
    KeQuerySystemTime(&SystemTime);
    tmp.tv_sec=(LONG)(SystemTime.QuadPart/10000000-11644473600);
    tmp.tv_usec=(LONG)((SystemTime.QuadPart%10000000)/10);
    tmp.tv_sec-=(ULONG)(PTime.QuadPart/TimeFreq.QuadPart);
    tmp.tv_usec-=(LONG)((PTime.QuadPart%TimeFreq.QuadPart)*1000000/TimeFreq.QuadPart);
    if (tmp.tv_usec<0) {
        tmp.tv_sec--;
        tmp.tv_usec+=1000000;
    }
    data->start=tmp;
    data->reference=1;
}

__inline void GET_TIME(struct timeval *dst, struct time_conv *data)
{
    LARGE_INTEGER PTime, TimeFreq;
    LONG tmp;

    PTime=KeQueryPerformanceCounter(&TimeFreq);
    tmp=(LONG)(PTime.QuadPart/TimeFreq.QuadPart);
    dst->tv_sec=data->start.tv_sec+tmp;
    dst->tv_usec=data->start.tv_usec+(LONG)((PTime.QuadPart%TimeFreq.QuadPart)*1000000/TimeFreq.QuadPart);
    if (dst->tv_usec>=1000000) {
        dst->tv_sec++;
        dst->tv_usec-=1000000;
    }
}

__inline void FORCE_TIME(struct timeval *src, struct time_conv *dest)
{
    dest->start=*src;
}

#else

/*RDTSC timestamps*/

/* callers must be at IRQL=PASSIVE_LEVEL */
__inline VOID TIME_SYNCHRONIZE(struct time_conv *data)
{
    struct timeval tmp;
    LARGE_INTEGER system_time;
    ULONGLONG curr_ticks;
    KIRQL old;
    LARGE_INTEGER start_kqpc,stop_kqpc,start_freq,stop_freq;
    ULONGLONG start_ticks,stop_ticks;
    ULONGLONG delta,delta2;
    KEVENT event;
    LARGE_INTEGER i;
    ULONGLONG reference;

    if (data->reference!=0)
        return;
    
    KeInitializeEvent(&event,NotificationEvent,FALSE);
    i.QuadPart=-3500000;
    KeRaiseIrql(HIGH_LEVEL,&old);
    start_kqpc=KeQueryPerformanceCounter(&start_freq);
#ifndef __GNUC__
    __asm
    {
        push eax
        push edx
        push ecx
        rdtsc
        lea ecx, start_ticks
        mov [ecx+4], edx
        mov [ecx], eax
        pop ecx
        pop edx
        pop eax
    }
#else
#endif
    KeLowerIrql(old);
    KeWaitForSingleObject(&event,UserRequest,KernelMode,TRUE ,&i);
    KeRaiseIrql(HIGH_LEVEL,&old);
    stop_kqpc=KeQueryPerformanceCounter(&stop_freq);
#ifndef __GNUC__
    __asm
    {
        push eax
        push edx
        push ecx
        rdtsc
        lea ecx, stop_ticks
        mov [ecx+4], edx
        mov [ecx], eax
        pop ecx
        pop edx
        pop eax
    }
#else
#endif
    KeLowerIrql(old);
    delta=stop_ticks-start_ticks;
    delta2=stop_kqpc.QuadPart-start_kqpc.QuadPart;
    if (delta>10000000000) {
        delta/=16;
        delta2/=16;
    }
    reference=delta*(start_freq.QuadPart)/delta2;
    data->reference=reference/1000;
    if (reference%1000>500) 
        data->reference++;
    data->reference*=1000;
    reference=data->reference;
    KeQuerySystemTime(&system_time);
#ifndef __GNUC__
    __asm
    {
        push eax
        push edx
        push ecx
        rdtsc
        lea ecx, curr_ticks
        mov [ecx+4], edx
        mov [ecx], eax
        pop ecx
        pop edx
        pop eax
    }
#else
#endif
    tmp.tv_sec=-(LONG)(curr_ticks/reference);
    tmp.tv_usec=-(LONG)((curr_ticks%reference)*1000000/reference);
    system_time.QuadPart-=116444736000000000;
    tmp.tv_sec+=(LONG)(system_time.QuadPart/10000000);
    tmp.tv_usec+=(LONG)((system_time.QuadPart%10000000)/10);
    if (tmp.tv_usec<0) {
        tmp.tv_sec--;
        tmp.tv_usec+=1000000;
    }
    data->start=tmp;
    IF_LOUD(DbgPrint("Frequency %I64u MHz\n",data->reference);)
}

__inline void FORCE_TIME(struct timeval *src, struct time_conv *dest)
{
    dest->start=*src;
}

__inline void GET_TIME(struct timeval *dst, struct time_conv *data)
{
    ULONGLONG tmp;
#ifndef __GNUC__
    __asm
    {
        push eax
        push edx
        push ecx
        rdtsc
        lea ecx, tmp
        mov [ecx+4], edx
        mov [ecx], eax
        pop ecx
        pop edx
        pop eax
    }
#else
#endif
    if (data->reference==0) {
        return;
    }
    dst->tv_sec=(LONG)(tmp/data->reference);
    dst->tv_usec=(LONG)((tmp-dst->tv_sec*data->reference)*1000000/data->reference);
    dst->tv_sec+=data->start.tv_sec;
    dst->tv_usec+=data->start.tv_usec;
    if (dst->tv_usec>=1000000) {
        dst->tv_sec++;
        dst->tv_usec-=1000000;
    }
}

#endif /*KQPC_TS*/

#endif /*WIN_NT_DRIVER*/

#endif /* __GNUC__ */

#endif /*_time_calls*/
