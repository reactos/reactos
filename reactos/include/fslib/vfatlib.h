/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        include/fslib/vfatlib.h
 * PURPOSE:     Public definitions for vfat filesystem library
 */
#ifndef __VFATLIB_H
#define __VFATLIB_H

NTSTATUS
VfatInitialize();

NTSTATUS
VfatCleanup();

#endif /*__VFATLIB_H */
