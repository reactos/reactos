/*
 * Implementation of memory & port access routines required by VirtioLib
 *
 * Copyright (c) 2016-2017 Red Hat, Inc.
 *
 * Author(s):
 *  Ladi Prosek <lprosek@redhat.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "osdep.h"
#include "virtio_pci.h"

/* The lower 64k of memory is never mapped so we can use the same routines
 * for both port I/O and memory access and use the address alone to decide
 * which space to use.
 */
#define PORT_MASK 0xFFFF

/* READ_REGISTER_* and WRITE_REGISTER_* take volatile ptr in newer DDKs */
#if NTDDI_VERSION >= NTDDI_WIN7
#define _VOLATILE volatile
#else
#define _VOLATILE
#endif

#define READ_PORT_OR_REGISTER(address, width)                       \
    (address & ~PORT_MASK) ?                                        \
        READ_REGISTER_##width((_VOLATILE width *)address) :         \
        READ_PORT_##width((width *)address)

#define WRITE_PORT_OR_REGISTER(address, width, value)               \
    (address & ~PORT_MASK) ?                                        \
        WRITE_REGISTER_##width((_VOLATILE width *)address, value) : \
        WRITE_PORT_##width((width *)address, value)

u8 ReadVirtIODeviceByte(ULONG_PTR ulRegister)
{
    return READ_PORT_OR_REGISTER(ulRegister, UCHAR);
}

void WriteVirtIODeviceByte(ULONG_PTR ulRegister, u8 bValue)
{
    WRITE_PORT_OR_REGISTER(ulRegister, UCHAR, bValue);
}

u16 ReadVirtIODeviceWord(ULONG_PTR ulRegister)
{
    return READ_PORT_OR_REGISTER(ulRegister, USHORT);
}

void WriteVirtIODeviceWord(ULONG_PTR ulRegister, u16 wValue)
{
    WRITE_PORT_OR_REGISTER(ulRegister, USHORT, wValue);
}

u32 ReadVirtIODeviceRegister(ULONG_PTR ulRegister)
{
    return READ_PORT_OR_REGISTER(ulRegister, ULONG);
}

void WriteVirtIODeviceRegister(ULONG_PTR ulRegister, u32 ulValue)
{
    WRITE_PORT_OR_REGISTER(ulRegister, ULONG, ulValue);
}
