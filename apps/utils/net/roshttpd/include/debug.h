/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        include/debug.h
 */
#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>

#ifdef DBG
#define DPRINT(x...)    printf(x)
#else
#define DPRINT(x...)
#endif

#endif /* __DEBUG_H */
