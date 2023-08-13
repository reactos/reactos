/**
 *******************************************************************************
 ** Copyright (c) 2011-2012                                                   **
 **                                                                           **
 **   Integrated Device Technology, Inc.                                      **
 **   Intel Corporation                                                       **
 **   LSI Corporation                                                         **
 **                                                                           **
 ** All rights reserved.                                                      **
 **                                                                           **
 *******************************************************************************
 **                                                                           **
 ** Redistribution and use in source and binary forms, with or without        **
 ** modification, are permitted provided that the following conditions are    **
 ** met:                                                                      **
 **                                                                           **
 **   1. Redistributions of source code must retain the above copyright       **
 **      notice, this list of conditions and the following disclaimer.        **
 **                                                                           **
 **   2. Redistributions in binary form must reproduce the above copyright    **
 **      notice, this list of conditions and the following disclaimer in the  **
 **      documentation and/or other materials provided with the distribution. **
 **                                                                           **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS   **
 ** IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, **
 ** THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR    **
 ** PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR         **
 ** CONTRIBUTORS BE LIABLE FOR ANY DIRECT,INDIRECT, INCIDENTAL, SPECIAL,      **
 ** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       **
 ** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        **
 ** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
 ** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
 ** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
 ** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
 **                                                                           **
 ** The views and conclusions contained in the software and documentation     **
 ** are those of the authors and should not be interpreted as representing    **
 ** official policies, either expressed or implied, of Intel Corporation,     **
 ** Integrated Device Technology Inc., or Sandforce Corporation.              **
 **                                                                           **
 *******************************************************************************
**/

/*
 * File: nvmeReg.h
 */

#ifndef __NVME_REG_H__
#define __NVME_REG_H__

/* Various NVME register fields */
#define NVME_AQA_CQS_LSB            16
#define NVME_CC_EN_LSB              0
#define NVME_CC_MPS_LSB             7
#define NVME_CSTS_RDY_MSK           1
#define NVME_MEM_PAGE_SIZE_SHIFT    13 /* When MPS is 0, means 4KB */
#define NVME_DB_START               0x1000
#define NVME_CC_NVM_CMD (0)
#define NVME_CC_SHUTDOWN_NONE (0)
#define NVME_CC_ROUND_ROBIN (0)
#define NVME_CC_IOSQES (6)
#define NVME_CC_IOCQES (4)

/* NVMe CONTROLLER */

#pragma pack(push, regs, 1)

/* Table 3.1.1 */
typedef union _NVMe_CONTROLLER_CAPABILITIES
{
    struct
    {
        /* 
         * [Maximum Queue Entries Supported] This field indicates the maximum 
         * individual queue size that the controller supports. This value 
         * applies to each of the I/O Submission Queues and I/O Completion 
         * Queues that software may create. This is a 0’s based value. The 
         * minimum value is 1h, indicating two entries.
         */
        USHORT MQES; 

        /* 
         * [Contiguous Queues Required] This field is set to ‘1’ if the 
         * controller requires that I/O Submission and I/O Completion Queues are 
         * required to be physically contiguous. This field is cleared to ‘0’ 
         * if the controller supports I/O Submission and I/O Completion Queues 
         * that are not physically contiguous. If this field is set to ‘1’, then 
         * the Physically Contiguous bit (CDW11.PC) in the Create I/O Submission 
         * Queue and Create I/O Completion Queue commands shall be set to ‘1’.
         */
        UCHAR  CQR       :1; 

        /* 
         * [Arbitration Mechanism Supported] This field is bit significant and 
         * indicates the optional arbitration mechanisms supported by the 
         * controller. If a bit is set to ‘1’, then the corresponding 
         * arbitration mechanism is supported by the controller. The round robin 
         * arbitration mechanism is not listed since all controllers shall 
         * support this arbitration mechanism. Refer to section 4.7 for 
         * arbitration details.  Bit 17 == Weighted Round Robin + Urgent. Bit 
         * 18 == Vendor Specific.
         */
        UCHAR  AMS       :2; 

        /* Bits 19-23 */
        UCHAR  Reserved1 :5;

        /* 
         * [Timeout] This is the worst case time that host software shall wait 
         * for the controller to become ready (CSTS.RDY set to ‘1’) after a 
         * power-on or reset event (CC.EN is set to ‘1’ by software). This worst 
         * case time may be experienced after an unclean shutdown; typical times 
         * are expected to be much shorter. This field is in 500 millisecond 
         * units.
         */
        UCHAR TO;

        /* Bits 32-35 */
        USHORT DSTRD :4;

        /* Bit 36 */
        USHORT Reserved2 :1;

        /* 
         * [Command Sets Supported] This field indicates the command set(s) that 
         * the controller supports. A minimum of one command set shall be 
         * supported. The field is bit significant. If a bit is set to ‘1’, then 
         * the corresponding command set is supported. If a bit is cleared to 
         * ‘0’, then the corresponding command set is not supported. Bit 37 == 
         * NVM Command Set. Bits 38-44 Reserved.
         */
        USHORT CSS :8; // NVMe1.0E 

        /* Bits 45-47 */
        USHORT Reserved3 :3; // NVMe1.0E

        /* 
         * [Memory Page Size Minimum] This field indicates the minimum host 
         * memory page size that the controller supports. The minimum memory 
         * page size is (2 ^ (12 + MPSMIN)). The host shall not configure a 
         * memory page size in CC.MPS that is smaller than this value.
         */
        UCHAR  MPSMIN    :4; 

        /* 
         * [Memory Page Size Maximum] This field indicates the maximum host 
         * memory page size that the controller supports. The maximum memory 
         * page size is (2 ^ (12 + MPSMAX)). The host shall not configure a 
         * memory page size in CC.MPS that is larger than this value.
         */
        UCHAR  MPSMAX    :4; 
        UCHAR   Reserved4;
    };

    struct
    {
        ULONG LowPart;
        ULONG HighPart;
    };

    ULONGLONG AsUlonglong;
}  NVMe_CONTROLLER_CAPABILITIES, *PNVMe_CONTROLLER_CAPABILITIES;

/* Table 3.1.2 */
typedef union _NVMe_VERSION
{
	struct
	{
		/* [Minor Version Number] Indicates the minor version is “0”. */
		USHORT MNR;

		/* [Major Version Number] Indicates the major version is “1”. */
		USHORT MJR;
	};
	ULONG AsUlong;
} NVMe_VERSION, *PNVMe_VERSION;

/* Table 3.1.5 */
typedef union _NVMe_CONTROLLER_CONFIGURATION
{
    struct
    {
        /* 
         * [Enable] When set to ‘1’, then the controller shall process commands 
         * based on Submission Queue Tail doorbell writes. When cleared to ‘0’, 
         * then the controller shall not process commands nor submit completion 
         * entries to Completion Queues. When this field transitions from ‘1’ to 
         * ‘0’, the controller is reset (referred to as a Controller Reset). The 
         * reset deletes all I/O Submission Queues and I/O Completion Queues 
         * created, resets the Admin Submission and Completion Queues, and 
         * brings the hardware to an idle state. The reset does not affect PCI 
         * Express registers nor the Admin Queue registers (AQA, ASQ, or ACQ). 
         * All other controller registers defined in this section are reset. The 
         * controller shall ensure that there is no data loss for commands that 
         * have been completed to the host as part of the reset operation. Refer 
         * to section 7.3 for reset details. When this field is cleared to ‘0’, 
         * the CSTS.RDY bit is cleared to ‘0’ by the controller. When this field 
         * is set to ‘1’, the controller sets CSTS.RDY to ‘1’ when it is ready 
         * to process commands. The Admin Queue registers (AQA, ASQ, and ACQ) 
         * shall only be modified when EN is cleared to ‘0’.
         */
        ULONG EN         :1; 

        /* Bits 1-3 */
        ULONG Reserved1  :3;

        /* 
         * [Command Set Selected] This field specifies the command set that is 
         * selected for use for the I/O Submission Queues. Software shall only 
         * select a supported command set, as indicated in CAP.CSS. The command 
         * set shall only be changed when the controller is disabled (CC.EN is 
         * cleared to ‘0’). The command set selected shall be used for all I/O 
         * Submission Queues. Value 000b == NVM command set.  Values 001b-111b 
         * Reserved.
         */
        ULONG CSS        :3; 

        /* 
         * [Memory Page Size] This field indicates the host memory page size. 
         * The memory page size is (2 ^ (12 + MPS)). Thus, the minimum host 
         * memory page size is 4KB and the maximum host memory page size is 
         * 128MB. The value set by host software shall be a supported value as 
         * indicated by the CAP.MPSMAX and CAP.MPSMIN fields. This field 
         * describes the value used for PRP entry size.
         */
        ULONG MPS        :4; 

        /* 
         * [Arbitration Mechanism Selected] This field selects the arbitration 
         * mechanism to be used. This value shall only be changed when EN is 
         * cleared to ‘0’. Software shall only set this field to supported 
         * arbitration mechanisms indicated in CAP.AMS. Value 000b == Round 
         * Robin. Value 001b == Weighted Round Robin + Urgent.  010b-110b == 
         * Reserved. 111b == Vendor Specific.
         */
        ULONG AMS        :3; 

        /* 
         * [Shutdown Notification] This field is used to initiate shutdown 
         * processing when a shutdown is occurring, i.e., a power down condition 
         * is expected. For a normal shutdown notification, it is expected that 
         * the controller is given time to process the shutdown notification. 
         * For an abrupt shutdown notification, the host may not wait for 
         * shutdown processing to complete before power is lost. The shutdown 
         * notification values are defined as: Value 00b == No notification; no 
         * effect. Value 01b == Normal shutdown notification. Value 10b == 
         * Abrupt shutdown notification.  Value 11b == Reserved. Shutdown 
         * notification should be issued by host software prior to any power 
         * down condition and prior to any change of the PCI power management 
         * state. It is recommended that shutdown notification also be sent 
         * prior to a warm reboot. To determine when shutdown processing is 
         * complete, refer to CSTS.SHST. Refer to section 7.6.2 for additional 
         * shutdown processing details.
         */
        ULONG SHN        :2; 

        /* 
         * [I/O Submission Queue Entry Size] This field defines the I/O 
         * Submission Queue entry size that is used for the selected I/O Command 
         * Set. The required and maximum values for this field are specified in 
         * the Identify Controller data structure for each I/O Command Set. The 
         * value is in bytes and is specified as a power of two (2^n).
         */
        ULONG IOSQES     :4; 

        /* 
         * [I/O Completion Queue Entry Size] This field defines the I/O 
         * Completion Queue entry size that is used for the selected I/O Command 
         * Set. The required and maximum values for this field are specified in 
         * the Identify Controller data structure for each I/O Command Set. The 
         * value is in bytes and is specified as a power of two (2^n).
         */
        ULONG IOCQES     :4; 

        /* Bits 24-31 */
        ULONG Reserved2  :8;
    };

    ULONG AsUlong;
} NVMe_CONTROLLER_CONFIGURATION, *PNVMe_CONTROLLER_CONFIGURATION;

/* Table 3.1.6 */
typedef union _NVMe_CONTROLLER_STATUS
{
    struct
    {
        /* 
         * [Ready] This field is set to ‘1’ when the controller is ready to 
         * process commands after CC.EN is set to ‘1’. This field shall be 
         * cleared to ‘0’ when CC.EN is cleared to ‘0’. Commands shall not be 
         * issued to the controller until this field is set to ‘1’ after the 
         * CC.EN bit is set to ‘1’. Failure to follow this requirement produces 
         * undefined results. Software shall wait a minimum of CAP.TO seconds 
         * for this field to be set to ‘1’ after CC.EN transitions from ‘0’ to 
         * ‘1’.
         */
        ULONG RDY        :1; 

        /* 
         * [Controller Fatal Status] Indicates that a fatal controller error 
         * occurred that could not be communicated in the appropriate Completion 
         * Queue. Refer to section 9.5.
         */
        ULONG CFS        :1; 

        /* 
         * [Shutdown Status] This field indicates the status of shutdown 
         * processing that is initiated by the host setting the CC.SHN field 
         * appropriately. The shutdown status values are defined as: Value 00b 
         * == Normal operation (no shutdown has been requested). Value 01b == 
         * Shutdown processing occurring. Value 10b == Shutdown processing 
         * complete. Value 11b == Reserved.
         */
        ULONG SHST       :2; 

        /* Bits 4-31 */
        ULONG Reserved   :28;
    };

    ULONG AsUlong;
} NVMe_CONTROLLER_STATUS, *PNVMe_CONTROLLER_STATUS;

/* Table 3.1.7 */
typedef union _NVMe_ADMIN_QUEUE_ATTRIBUTES
{
    struct
    {
        /* 
         * [Admin Submission Queue Size] Defines the size of the Admin 
         * Submission Queue in entries. Refer to section 4.1.3. The minimum size 
         * of the Admin Submission Queue is two entries. The maximum size of the 
         * Admin Submission Queue is 4096 entries. This is a 0’s based value.
         */
        ULONG ASQS       :12; 

        /* Bits 12-15 */
        ULONG Reserved1  :4;

        /* 
         * [Admin Completion Queue Size] Defines the size of the Admin 
         * Completion Queue in entries. Refer to section 4.1.3. The minimum size 
         * of the Admin Completion Queue is two entries. The maximum size of the 
         * Admin Completion Queue is 4096 entries. This is a 0’s based value.
         */
        ULONG ACQS       :12; 

        /* Bits 28-31 */
        ULONG Reserved2  :4;
    };

    ULONG AsUlong;
} NVMe_ADMIN_QUEUE_ATTRIBUTES, *PNVMe_ADMIN_QUEUE_ATTRIBUTES;

/* Table 3.1.8 */
typedef union _NVMe_SUBMISSION_QUEUE_BASE
{
    struct
    {
        /* Bits 0-11 */
        ULONGLONG Reserved   :12;

        /* 
         * [Admin Submission Queue Base] Indicates the 64-bit physical address 
         * for the Admin Submission Queue. This address shall be memory page 
         * aligned (based on the value in CC.MPS). All Admin commands, including 
         * creation of additional Submission Queues and Completions Queues shall 
         * be submitted to this queue. For the definition of Submission Queues, 
         * refer to section 4.1.
         */
        ULONGLONG ASQB       :52; 
    };

    ULONGLONG AsUlonglong;

    struct
    {
        ULONG LowPart;
        ULONG HighPart;
    };
} NVMe_SUBMISSION_QUEUE_BASE, *PNVMe_SUBMISSION_QUEUE_BASE;

/* Table 3.1.9 */
typedef union _NVMe_COMPLETION_QUEUE_BASE
{
    struct
    {
        /* Bits 0-11 */
        ULONGLONG Reserved   :12;

        /* 
         * [Admin Completion Queue Base] Indicates the 64-bit physical address 
         * for the Admin Completion Queue. This address shall be memory page 
         * aligned (based on the value in CC.MPS). All completion entries for 
         * the commands submitted to the Admin Submission Queue shall be posted 
         * to this Completion Queue. This queue is always associated with 
         * interrupt vector 0. For the definition of Completion Queues, refer to 
         * section 4.1.
         */
        ULONGLONG ACQB       :52; 
    };

    ULONGLONG AsUlonglong;

    struct
    {
        ULONG LowPart;
        ULONG HighPart;
    };
} NVMe_COMPLETION_QUEUE_BASE, *PNVMe_COMPLETION_QUEUE_BASE;

/* Table 3.1.11 */
typedef union _NVMe_QUEUE_Y_DOORBELL
{
    struct
    {
        USHORT  QHT;

        /* Bits 16-31 */
        USHORT  Reserved;
    };

    ULONG AsUlong;
} NVMe_QUEUE_Y_DOORBELL,
  *PNVMe_QUEUE_Y_DOORBELL;

/* Table 3.1 */
typedef struct _NVMe_CONTROLLER_REGISTERS
{
    NVMe_CONTROLLER_CAPABILITIES  CAP;
    NVMe_VERSION                  VS;

    /* 
     * [Interrupt Vector Mask Set] This field is bit significant. If a ‘1’ is
     * written to a bit, then the corresponding interrupt vector is masked. 
     * Writing a ‘0’ to a bit has no effect. When read, this field returns the 
     * current interrupt mask value. If a bit has a value of a ‘1’, then the 
     * corresponding interrupt vector is masked. If a bit has a value of ‘0’, 
     * then the corresponding interrupt vector is not masked.
     */
    ULONG                         IVMS; 

    /* 
     * [Interrupt Vector Mask Clear] This field is bit significant. If a ‘1’ is 
     * written to a bit, then the corresponding interrupt vector is unmasked.
     * Writing a ‘0’ to a bit has no effect. When read, this field returns the
     * current interrupt mask value. If a bit has a value of a ‘1’, then the 
     * corresponding interrupt vector is masked, If a bit has a value of ‘0’, 
     * then the corresponding interrupt vector is not masked.
     */
    ULONG                         INTMC; 

    NVMe_CONTROLLER_CONFIGURATION CC;
    ULONG                         Reserved1;
    NVMe_CONTROLLER_STATUS        CSTS;
    ULONG                         Reserved2;
    NVMe_ADMIN_QUEUE_ATTRIBUTES   AQA;
    NVMe_SUBMISSION_QUEUE_BASE    ASQ;
    NVMe_COMPLETION_QUEUE_BASE    ACQ;

    /* Bytes 0x38 - 0xEFF */
    ULONG                         Reserved3[0x3B2];

    /* Bytes 0xF00 - 0xFFF */
    ULONG                         CommandSetSpecific[0x40];

    /* variable sized array limited by the BAR size */
    NVMe_QUEUE_Y_DOORBELL           IODB[1];
} NVMe_CONTROLLER_REGISTERS, *PNVMe_CONTROLLER_REGISTERS;

#pragma pack(pop, regs)

#endif /* __NVME_REG_H__ */
