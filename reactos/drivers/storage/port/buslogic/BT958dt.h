/*
 * vmscsi-- Miniport driver for the Buslogic BT 958 SCSI Controller
 *          under Windows 2000/XP/Server 2003
 *
 *          Based in parts on the buslogic driver for the same device
 *          available with the GNU Linux Operating System.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#ifndef _BT958dt_h_
#define _BT958dt_h_

// BT958ExtendedSetupInfoGuid - BT958ExtendedSetupInfo
// BT958 Extended Setup Information (Operation Code 8Dh)
#define BT958Wmi_ExtendedSetupInfo_Guid \
    { 0xcbd60d59,0xce49,0x4cf4, { 0xab,0x7a,0xde,0xc6,0xf4,0x98,0x8d,0x1a } }

DEFINE_GUID(BT958ExtendedSetupInfoGuid_GUID, \
            0xcbd60d59,0xce49,0x4cf4,0xab,0x7a,0xde,0xc6,0xf4,0x98,0x8d,0x1a);


typedef struct _BT958ExtendedSetupInfo
{
    //
    UCHAR BusType;
    #define BT958ExtendedSetupInfo_BusType_SIZE sizeof(UCHAR)
    #define BT958ExtendedSetupInfo_BusType_ID 1

    //
    UCHAR BIOS_Address;
    #define BT958ExtendedSetupInfo_BIOS_Address_SIZE sizeof(UCHAR)
    #define BT958ExtendedSetupInfo_BIOS_Address_ID 2

    //
    USHORT ScatterGatherLimit;
    #define BT958ExtendedSetupInfo_ScatterGatherLimit_SIZE sizeof(USHORT)
    #define BT958ExtendedSetupInfo_ScatterGatherLimit_ID 3

    //
    UCHAR MailboxCount;
    #define BT958ExtendedSetupInfo_MailboxCount_SIZE sizeof(UCHAR)
    #define BT958ExtendedSetupInfo_MailboxCount_ID 4

    //
    ULONG BaseMailboxAddress;
    #define BT958ExtendedSetupInfo_BaseMailboxAddress_SIZE sizeof(ULONG)
    #define BT958ExtendedSetupInfo_BaseMailboxAddress_ID 5

    //
    BOOLEAN FastOnEISA;
    #define BT958ExtendedSetupInfo_FastOnEISA_SIZE sizeof(BOOLEAN)
    #define BT958ExtendedSetupInfo_FastOnEISA_ID 6

    //
    BOOLEAN LevelSensitiveInterrupt;
    #define BT958ExtendedSetupInfo_LevelSensitiveInterrupt_SIZE sizeof(BOOLEAN)
    #define BT958ExtendedSetupInfo_LevelSensitiveInterrupt_ID 7

    //
    UCHAR FirmwareRevision[3];
    #define BT958ExtendedSetupInfo_FirmwareRevision_SIZE sizeof(UCHAR[3])
    #define BT958ExtendedSetupInfo_FirmwareRevision_ID 8

    //
    BOOLEAN HostWideSCSI;
    #define BT958ExtendedSetupInfo_HostWideSCSI_SIZE sizeof(BOOLEAN)
    #define BT958ExtendedSetupInfo_HostWideSCSI_ID 9

    //
    BOOLEAN HostDifferentialSCSI;
    #define BT958ExtendedSetupInfo_HostDifferentialSCSI_SIZE sizeof(BOOLEAN)
    #define BT958ExtendedSetupInfo_HostDifferentialSCSI_ID 10

    //
    BOOLEAN HostSupportsSCAM;
    #define BT958ExtendedSetupInfo_HostSupportsSCAM_SIZE sizeof(BOOLEAN)
    #define BT958ExtendedSetupInfo_HostSupportsSCAM_ID 11

    //
    BOOLEAN HostUltraSCSI;
    #define BT958ExtendedSetupInfo_HostUltraSCSI_SIZE sizeof(BOOLEAN)
    #define BT958ExtendedSetupInfo_HostUltraSCSI_ID 12

    //
    BOOLEAN HostSmartTermination;
    #define BT958ExtendedSetupInfo_HostSmartTermination_SIZE sizeof(BOOLEAN)
    #define BT958ExtendedSetupInfo_HostSmartTermination_ID 13

} BT958ExtendedSetupInfo, *PBT958ExtendedSetupInfo;

#endif
