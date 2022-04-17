/******************************************************************************
 *
 * Name: achware.h -- hardware specific interfaces
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2021, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef __ACHWARE_H__
#define __ACHWARE_H__


/* Values for the _SST predefined method */

#define ACPI_SST_INDICATOR_OFF  0
#define ACPI_SST_WORKING        1
#define ACPI_SST_WAKING         2
#define ACPI_SST_SLEEPING       3
#define ACPI_SST_SLEEP_CONTEXT  4


/*
 * hwacpi - high level functions
 */
ACPI_STATUS
AcpiHwSetMode (
    UINT32                  Mode);

UINT32
AcpiHwGetMode (
    void);


/*
 * hwregs - ACPI Register I/O
 */
ACPI_STATUS
AcpiHwValidateRegister (
    ACPI_GENERIC_ADDRESS    *Reg,
    UINT8                   MaxBitWidth,
    UINT64                  *Address);

ACPI_STATUS
AcpiHwRead (
    UINT64                  *Value,
    ACPI_GENERIC_ADDRESS    *Reg);

ACPI_STATUS
AcpiHwWrite (
    UINT64                  Value,
    ACPI_GENERIC_ADDRESS    *Reg);

ACPI_BIT_REGISTER_INFO *
AcpiHwGetBitRegisterInfo (
    UINT32                  RegisterId);

ACPI_STATUS
AcpiHwWritePm1Control (
    UINT32                  Pm1aControl,
    UINT32                  Pm1bControl);

ACPI_STATUS
AcpiHwRegisterRead (
    UINT32                  RegisterId,
    UINT32                  *ReturnValue);

ACPI_STATUS
AcpiHwRegisterWrite (
    UINT32                  RegisterId,
    UINT32                  Value);

ACPI_STATUS
AcpiHwClearAcpiStatus (
    void);


/*
 * hwsleep - sleep/wake support (Legacy sleep registers)
 */
ACPI_STATUS
AcpiHwLegacySleep (
    UINT8                   SleepState);

ACPI_STATUS
AcpiHwLegacyWakePrep (
    UINT8                   SleepState);

ACPI_STATUS
AcpiHwLegacyWake (
    UINT8                   SleepState);


/*
 * hwesleep - sleep/wake support (Extended FADT-V5 sleep registers)
 */
void
AcpiHwExecuteSleepMethod (
    char                    *MethodName,
    UINT32                  IntegerArgument);

ACPI_STATUS
AcpiHwExtendedSleep (
    UINT8                   SleepState);

ACPI_STATUS
AcpiHwExtendedWakePrep (
    UINT8                   SleepState);

ACPI_STATUS
AcpiHwExtendedWake (
    UINT8                   SleepState);


/*
 * hwvalid - Port I/O with validation
 */
ACPI_STATUS
AcpiHwReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width);

ACPI_STATUS
AcpiHwWritePort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width);


/*
 * hwgpe - GPE support
 */
UINT32
AcpiHwGetGpeRegisterBit (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo);

ACPI_STATUS
AcpiHwLowSetGpe (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo,
    UINT32                  Action);

ACPI_STATUS
AcpiHwDisableGpeBlock (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Context);

ACPI_STATUS
AcpiHwClearGpe (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo);

ACPI_STATUS
AcpiHwClearGpeBlock (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Context);

ACPI_STATUS
AcpiHwGetGpeStatus (
    ACPI_GPE_EVENT_INFO     *GpeEventInfo,
    ACPI_EVENT_STATUS       *EventStatus);

ACPI_STATUS
AcpiHwDisableAllGpes (
    void);

ACPI_STATUS
AcpiHwEnableAllRuntimeGpes (
    void);

ACPI_STATUS
AcpiHwEnableAllWakeupGpes (
    void);

UINT8
AcpiHwCheckAllGpes (
    void);

ACPI_STATUS
AcpiHwEnableRuntimeGpeBlock (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Context);


/*
 * hwpci - PCI configuration support
 */
ACPI_STATUS
AcpiHwDerivePciId (
    ACPI_PCI_ID             *PciId,
    ACPI_HANDLE             RootPciDevice,
    ACPI_HANDLE             PciRegion);


#endif /* __ACHWARE_H__ */
