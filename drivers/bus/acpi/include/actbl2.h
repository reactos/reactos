/******************************************************************************
 *
 * Name: actbl2.h - ACPI Specification Revision 2.0 Tables
 *       $Revision: 1.1 $
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2000, 2001 R. Byron Moore
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ACTBL2_H__
#define __ACTBL2_H__

/**************************************/
/* Prefered Power Management Profiles */
/**************************************/
#define PM_UNSPECIFIED        0
#define PM_DESKTOP            1
#define PM_MOBILE             2
#define PM_WORKSTATION        3
#define PM_ENTERPRISE_SERVER  4
#define PM_SOHO_SERVER        5
#define PM_APPLIANCE_PC       6

/*********************************************/
/* ACPI Boot Arch Flags, See spec Table 5-10 */
/*********************************************/
#define BAF_LEGACY_DEVICES             0x0001
#define BAF_8042_KEYBOARD_CONTROLLER   0x0002

#define FADT2_REVISION_ID     3

#pragma pack(1)

/*************************************/
/* ACPI Specification Rev 2.0 for    */
/* the Root System Description Table */
/*************************************/
typedef struct
{
	ACPI_TABLE_HEADER   header;                 /* Table header */
	u32                 table_offset_entry [1]; /* Array of pointers to  */
			   /* other tables' headers */
} RSDT_DESCRIPTOR_REV2;


/********************************************/
/* ACPI Specification Rev 2.0 for the       */
/* Extended System Description Table (XSDT) */
/********************************************/
typedef struct
{
	ACPI_TABLE_HEADER   header;                 /* Table header */
	UINT64              table_offset_entry [1]; /* Array of pointers to  */
			   /* other tables' headers */
} XSDT_DESCRIPTOR_REV2;

/***************************************/
/* ACPI Specification Rev 2.0 for      */
/* the Firmware ACPI Control Structure */
/***************************************/
typedef struct
{
	NATIVE_CHAR         signature[4];           /* signature "FACS" */
	u32                 length;                 /* length of structure, in bytes */
	u32                 hardware_signature;     /* hardware configuration signature */
	u32                 firmware_waking_vector; /* 32bit physical address of the Firmware Waking Vector. */
	u32                 global_lock;            /* Global Lock used to synchronize access to shared hardware resources */
	u32                 S4_bios_f       : 1;    /* Indicates if S4_bIOS support is present */
	u32                 reserved1       : 31;   /* must be 0 */
	UINT64              Xfirmware_waking_vector; /* 64bit physical address of the Firmware Waking Vector. */
	u8                  version;                /* Version of this table */
	u8                  reserved3 [31];         /* reserved - must be zero */

} FACS_DESCRIPTOR_REV2;


/***************************************/
/* ACPI Specification Rev 2.0 for      */
/* the Generic Address Structure (GAS) */
/***************************************/
typedef struct
{
	u8                  address_space_id;   /* Address space where struct or register exists. */
	u8                  register_bit_width; /* Size in bits of given register */
	u8                  register_bit_offset; /* Bit offset within the register */
	u8                  reserved;           /* Must be 0 */
	UINT64              address;            /* 64-bit address of struct or register */

} ACPI_GAS;


/************************************/
/* ACPI Specification Rev 2.0 for   */
/* the Fixed ACPI Description Table */
/************************************/
typedef struct
{
	ACPI_TABLE_HEADER   header;            /* table header */
	u32                 V1_firmware_ctrl;  /* 32-bit physical address of FACS */
	u32                 V1_dsdt;           /* 32-bit physical address of DSDT */
	u8                  reserved1;         /* System Interrupt Model isn't used in ACPI 2.0*/
	u8                  prefer_PM_profile; /* Conveys preferred power management profile to OSPM. */
	u16                 sci_int;           /* System vector of SCI interrupt */
	u32                 smi_cmd;           /* Port address of SMI command port */
	u8                  acpi_enable;       /* value to write to smi_cmd to enable ACPI */
	u8                  acpi_disable;      /* value to write to smi_cmd to disable ACPI */
	u8                  S4_bios_req;       /* Value to write to SMI CMD to enter S4_bIOS state */
	u8                  pstate_cnt;        /* processor performance state control*/
	u32                 V1_pm1a_evt_blk;   /* Port address of Power Mgt 1a Acpi_event Reg Blk */
	u32                 V1_pm1b_evt_blk;   /* Port address of Power Mgt 1b Acpi_event Reg Blk */
	u32                 V1_pm1a_cnt_blk;   /* Port address of Power Mgt 1a Control Reg Blk */
	u32                 V1_pm1b_cnt_blk;   /* Port address of Power Mgt 1b Control Reg Blk */
	u32                 V1_pm2_cnt_blk;    /* Port address of Power Mgt 2 Control Reg Blk */
	u32                 V1_pm_tmr_blk;     /* Port address of Power Mgt Timer Ctrl Reg Blk */
	u32                 V1_gpe0blk;        /* Port addr of General Purpose Acpi_event 0 Reg Blk */
	u32                 V1_gpe1_blk;       /* Port addr of General Purpose Acpi_event 1 Reg Blk */
	u8                  pm1_evt_len;       /* Byte Length of ports at pm1_x_evt_blk */
	u8                  pm1_cnt_len;       /* Byte Length of ports at pm1_x_cnt_blk */
	u8                  pm2_cnt_len;       /* Byte Length of ports at pm2_cnt_blk */
	u8                  pm_tm_len;         /* Byte Length of ports at pm_tm_blk */
	u8                  gpe0blk_len;       /* Byte Length of ports at gpe0_blk */
	u8                  gpe1_blk_len;      /* Byte Length of ports at gpe1_blk */
	u8                  gpe1_base;         /* offset in gpe model where gpe1 events start */
	u8                  cst_cnt;           /* Support for the _CST object and C States change notification.*/
	u16                 plvl2_lat;         /* worst case HW latency to enter/exit C2 state */
	u16                 plvl3_lat;         /* worst case HW latency to enter/exit C3 state */
	u16                 flush_size;        /* number of flush strides that need to be read */
	u16                 flush_stride;      /* Processor's memory cache line width, in bytes */
	u8                  duty_offset;       /* Processor_’s duty cycle index in processor's P_CNT reg*/
	u8                  duty_width;        /* Processor_’s duty cycle value bit width in P_CNT register.*/
	u8                  day_alrm;          /* index to day-of-month alarm in RTC CMOS RAM */
	u8                  mon_alrm;          /* index to month-of-year alarm in RTC CMOS RAM */
	u8                  century;           /* index to century in RTC CMOS RAM */
	u16                 iapc_boot_arch;    /* IA-PC Boot Architecture Flags. See Table 5-10 for description*/
	u8                  reserved2;         /* reserved */
	u32                 wb_invd     : 1;   /* wbinvd instruction works properly */
	u32                 wb_invd_flush : 1; /* wbinvd flushes but does not invalidate */
	u32                 proc_c1     : 1;   /* all processors support C1 state */
	u32                 plvl2_up    : 1;   /* C2 state works on MP system */
	u32                 pwr_button  : 1;   /* Power button is handled as a generic feature */
	u32                 sleep_button : 1;  /* Sleep button is handled as a generic feature, or not present */
	u32                 fixed_rTC   : 1;   /* RTC wakeup stat not in fixed register space */
	u32                 rtcs4       : 1;   /* RTC wakeup stat not possible from S4 */
	u32                 tmr_val_ext : 1;   /* tmr_val is 32 bits */
	u32                 dock_cap    : 1;   /* Supports Docking */
	u32                 reset_reg_sup : 1; /* Indicates system supports system reset via the FADT RESET_REG*/
	u32                 sealed_case : 1;   /* Indicates system has no internal expansion capabilities and case is sealed. */
	u32                 headless    : 1;   /* Indicates system does not have local video capabilities or local input devices.*/
	u32                 cpu_sw_sleep : 1;  /* Indicates to OSPM that a processor native instruction */
			 /* must be executed after writing the SLP_TYPx register. */
	u32                 reserved6   : 18;  /* reserved - must be zero */

	ACPI_GAS            reset_register;    /* Reset register address in GAS format */
	u8                  reset_value;       /* Value to write to the Reset_register port to reset the system. */
	u8                  reserved7[3];      /* These three bytes must be zero */
	UINT64              Xfirmware_ctrl;     /* 64-bit physical address of FACS */
	UINT64              Xdsdt;              /* 64-bit physical address of DSDT */
	ACPI_GAS            Xpm1a_evt_blk;      /* Extended Power Mgt 1a Acpi_event Reg Blk address */
	ACPI_GAS            Xpm1b_evt_blk;      /* Extended Power Mgt 1b Acpi_event Reg Blk address */
	ACPI_GAS            Xpm1a_cnt_blk;      /* Extended Power Mgt 1a Control Reg Blk address */
	ACPI_GAS            Xpm1b_cnt_blk;      /* Extended Power Mgt 1b Control Reg Blk address */
	ACPI_GAS            Xpm2_cnt_blk;       /* Extended Power Mgt 2 Control Reg Blk address */
	ACPI_GAS            Xpm_tmr_blk;        /* Extended Power Mgt Timer Ctrl Reg Blk address */
	ACPI_GAS            Xgpe0blk;           /* Extended General Purpose Acpi_event 0 Reg Blk address */
	ACPI_GAS            Xgpe1_blk;          /* Extended General Purpose Acpi_event 1 Reg Blk address */

}  FADT_DESCRIPTOR_REV2;


#pragma pack()

#endif /* __ACTBL2_H__ */

