/*
Copyright (c) 2006-2008 dogbert <dogber1@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _PROPERTY_H_
#define _PROPERTY_H_

//GUID for the private property
// {2B81CDBB-EE6C-4ECC-8AA5-9A188B023DFF}
#define STATIC_KSPROPSETID_CMI \
    0x2B81CDBB, 0xEE6C, 0x4ECC, {0x8A, 0xA5, 0x9A, 0x18, 0x8B, 0x02, 0x3D, 0xFF}
DEFINE_GUIDSTRUCT("2B81CDBB-EE6C-4ECC-8AA5-9A188B023DFF", KSPROPSETID_CMI);
#define KSPROPSETID_CMI DEFINE_GUIDNAMED(KSPROPSETID_CMI)

//methods
#define KSPROPERTY_CMI_GET  1
#define KSPROPERTY_CMI_SET  2

#define FMT_441_PCM       0x00000001
#define FMT_480_PCM       0x00000002
#define FMT_882_PCM       0x00000004
#define FMT_960_PCM       0x00000008
#define FMT_441_MULTI_PCM 0x00000010
#define FMT_480_MULTI_PCM 0x00000020
#define FMT_882_MULTI_PCM 0x00000040
#define FMT_960_MULTI_PCM 0x00000080
#define FMT_441_DOLBY     0x00000100
#define FMT_480_DOLBY     0x00000200
#define FMT_882_DOLBY     0x00000400
#define FMT_960_DOLBY     0x00000800

#ifndef UInt32
#define UInt32	ULONG
#define UInt16	USHORT
#define UInt8	BYTE
#define Int32   LONG
#endif

typedef struct
{
	char    driverVersion[32];
	int     hardwareRevision;
	UInt16  IOBase;
	UInt16  MPUBase;
	UInt32  maxChannels;
	UInt32  enableSPDIMonitor;
	UInt32  exchangeFrontBack;
	UInt32  enableBass2Line;
	UInt32  enableCenter2Line;
	UInt32  enableRear2Line;
	UInt32  enableCenter2Mic;
	UInt32  enableSPDO;
	UInt32  enableSPDO5V;
	UInt32  enableSPDOCopyright;
	UInt32  select2ndSPDI;
	UInt32  invertPhaseSPDI;
	UInt32  invertValidBitSPDI;
	UInt32  loopSPDI;
	UInt32  formatMask;
	UInt32  enableSPDI;
} CMIDATA;

#endif //_PROPERTY_H_
