/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file ichreg.h was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

#ifndef _ICHREG_H_
#define _ICHREG_H_

// We define the offsets like PI_BDBAR as ULONG (instead of UCHAR) for run
// time efficiency.

// CoDec AC97 register space offsets
const ULONG PRIMARY_CODEC   = 0x00;
const ULONG SECONDARY_CODEC = 0x80;

// Native audio bus master control registers (offsets)
const ULONG PI_BDBAR        = 0x00; // PCM In Buffer Descriptor Base Address Register
const ULONG PI_CIV          = 0x04; // PCM In Current Index Value
const ULONG PI_LVI          = 0x05; // PCM In Last Valid Index
const ULONG PI_SR           = 0x06; // PCM In Status Register
const ULONG PI_PICB         = 0x08; // PCM In Position In Current Buffer
const ULONG PI_PIV          = 0x0A; // PCM In Prefetch Index Value
const ULONG PI_CR           = 0x0B; // PCM In Control Register
const ULONG PO_BDBAR        = 0x10; // PCM Out Buffer Descriptor Base Address Register
const ULONG PO_CIV          = 0x14; // PCM Out Current Index Value
const ULONG PO_LVI          = 0x15; // PCM Out Last Valid Index
const ULONG PO_SR           = 0x16; // PCM Out Status Register
const ULONG PO_PICB         = 0x18; // PCM Out Position In Current Buffer
const ULONG PO_PIV          = 0x1A; // PCM Out Prefetch Index Value
const ULONG PO_CR           = 0x1B; // PCM Out Control Register
const ULONG MC_BDBAR        = 0x20; // Mic In Buffer Descriptor Base Address Register
const ULONG MC_CIV          = 0x24; // Mic In Current Index Value
const ULONG MC_LVI          = 0x25; // Mic In Last Valid Index
const ULONG MC_SR           = 0x26; // Mic In Status Register
const ULONG MC_PICB         = 0x28; // Mic In Position In Current Buffer
const ULONG MC_PIV          = 0x2A; // Mic In Prefetch Index Value
const ULONG MC_CR           = 0x2B; // Mic In Control Register
const ULONG GLOB_CNT        = 0x2C; // Global Control
const ULONG GLOB_STA        = 0x30; // Global Status
const ULONG CAS             = 0x34; // Codec Access Semiphore

// Defines for relative accesses (offsets)
const ULONG X_PI_BASE       = 0x00; // PCM In Base
const ULONG X_PO_BASE       = 0x10; // PCM Out Base
const ULONG X_MC_BASE       = 0x20; // Mic In Base
const ULONG X_BDBAR         = 0x00; // Buffer Descriptor Base Address Register
const ULONG X_CIV           = 0x04; // Current Index Value
const ULONG X_LVI           = 0x05; // Last Valid Index
const ULONG X_SR            = 0x06; // Status Register
const ULONG X_PICB          = 0x08; // Position In Current Buffer
const ULONG X_PIV           = 0x0A; // Prefetch Index Value
const ULONG X_CR            = 0x0B; // Control Register

// Bits defined in satatus register (*_SR)
const USHORT SR_FIFOE       = 0x0010;   // FIFO error
const USHORT SR_BCIS        = 0x0008;   // Buffer Completeion Interrupt Status
const USHORT SR_LVBCI       = 0x0004;   // Last Valid Buffer Completion Interrupt
const USHORT SR_CELV        = 0x0002;   // Last Valid Buffer Completion Interrupt

// Global Control bit defines (GLOB_CNT)
const ULONG GLOB_CNT_PCM6   = 0x00200000;   // 6 Channel Mode bit
const ULONG GLOB_CNT_PCM4   = 0x00100000;   // 4 Channel Mode bit
const ULONG GLOB_CNT_SRIE   = 0x00000020;   // Secondary Resume Interrupt Enable
const ULONG GLOB_CNT_PRIE   = 0x00000010;   // Primary Resume Interrupt Enable
const ULONG GLOB_CNT_ACLOFF = 0x00000008;   // ACLINK Off
const ULONG GLOB_CNT_WARM   = 0x00000004;   // AC97 Warm Reset
const ULONG GLOB_CNT_COLD   = 0x00000002;   // AC97 Cold Reset
const ULONG GLOB_CNT_GIE    = 0x00000001;   // GPI Interrupt Enable

// Global Status bit defines (GLOB_STA)
const ULONG GLOB_STA_MC6    = 0x00200000;   // Multichannel Capability 6 channel
const ULONG GLOB_STA_MC4    = 0x00100000;   // Multichannel Capability 4 channel
const ULONG GLOB_STA_MD3    = 0x00020000;   // Modem Power Down Semiphore
const ULONG GLOB_STA_AD3    = 0x00010000;   // Audio Power Down Semiphore
const ULONG GLOB_STA_RCS    = 0x00008000;   // Read Completion Status
const ULONG GLOB_STA_B3S12  = 0x00004000;   // Bit 3 Slot 12
const ULONG GLOB_STA_B2S12  = 0x00002000;   // Bit 2 Slot 12
const ULONG GLOB_STA_B1S12  = 0x00001000;   // Bit 1 Slot 12
const ULONG GLOB_STA_SRI    = 0x00000800;   // Secondary Resume Interrupt
const ULONG GLOB_STA_PRI    = 0x00000400;   // Primary Resume Interrupt
const ULONG GLOB_STA_SCR    = 0x00000200;   // Secondary Codec Ready
const ULONG GLOB_STA_PCR    = 0x00000100;   // Primary Codec Ready
const ULONG GLOB_STA_MINT   = 0x00000080;   // Mic In Interrupt
const ULONG GLOB_STA_POINT  = 0x00000040;   // PCM Out Interrupt
const ULONG GLOB_STA_PIINT  = 0x00000020;   // PCM In Interrupt
const ULONG GLOB_STA_MOINT  = 0x00000004;   // Modem Out Interrupt

// CoDec Access Semiphore bit defines (CAS)
const UCHAR CAS_CAS         = 0x01; // Codec Access Semiphore Bit

// DMA Engine Control Register (*_CR) bit defines
const UCHAR CR_IOCE         = 0x10; // Interrupt On Completion Enable
const UCHAR CR_FEIE         = 0x08; // FIFO Error Interrupt Enable
const UCHAR CR_LVBIE        = 0x04; // Last Valid Buffer Interrupt Enable
const UCHAR CR_RPBM         = 0x01; // Run/Pause Bus Master
const UCHAR CR_RR           = 0x02; // Reset Registers (RR)

// BDL policy bits
const USHORT IOC_ENABLE     = 0x8000;
const USHORT BUP_SET        = 0x4000;

#endif  //_ICHREG_H_

