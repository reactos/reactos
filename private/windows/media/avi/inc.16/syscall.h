/*
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1992-1994
 *      All Rights Reserved.
 */

#define Abort                           0x00
#define Std_Con_Input                   0x01
#define Std_Con_Output                  0x02
#define Std_Aux_Input                   0x03
#define Std_Aux_Output                  0x04
#define Std_Printer_Output              0x05
#define Raw_Con_IO                      0x06
#define Raw_Con_Input                   0x07
#define Std_Con_Input_No_Echo           0x08
#define Std_Con_String_Output           0x09
#define Std_Con_String_Input            0x0A
#define Std_Con_Input_Status            0x0B
#define Std_Con_Input_Flush             0x0C
#define Disk_Reset                      0x0D
#define Set_Default_Drive               0x0E
#define FCB_Open                        0x0F
#define FCB_Close                       0x10
#define Dir_Search_First                0x11
#define Dir_Search_Next                 0x12
#define FCB_Delete                      0x13
#define FCB_Seq_Read                    0x14
#define FCB_Seq_Write                   0x15
#define FCB_Create                      0x16
#define FCB_Rename                      0x17
#define Get_Default_Drive               0x19
#define Set_DMA                         0x1A
#define Get_Default_DPB                 0x1F    /* ;Internal */
#define FCB_Random_Read                 0x21
#define FCB_Random_Write                0x22
#define Get_FCB_File_Length             0x23
#define Get_FCB_Position                0x24
#define Set_Interrupt_Vector            0x25
#define Create_Process_Data_Block       0x26
#define FCB_Random_Read_Block           0x27
#define FCB_Random_Write_Block          0x28
#define Parse_File_Descriptor           0x29
#define Get_Date                        0x2A
#define Set_Date                        0x2B
#define Get_Time                        0x2C
#define Set_Time                        0x2D
#define Set_Verify_On_Write             0x2E

/*
 *  Extended functionality group
 */
#define Get_DMA                         0x2F
#define Get_Version                     0x30
#define Keep_Process                    0x31
#define Get_DPB                         0x32    /* ;Internal */
#define Set_CTRL_C_Trapping             0x33
#define Get_InDOS_Flag                  0x34
#define Get_Interrupt_Vector            0x35
#define Get_Drive_Freespace             0x36
#define Char_Oper                       0x37
#define International                   0x38

/*
 *  Directory Group
 */
#define MKDir                           0x39
#define RMDir                           0x3A
#define CHDir                           0x3B

/*
 *  File Group
 */
#define Creat                           0x3C
#define Open                            0x3D
#define Close                           0x3E
#define Read                            0x3F
#define Write                           0x40
#define Unlink                          0x41
#define LSeek                           0x42
#define CHMod                           0x43
#define IOCtl                           0x44
#define XDup                            0x45
#define XDup2                           0x46
#define Current_Dir                     0x47

/*
 *  Memory Group
 */
#define Alloc                           0x48
#define Dealloc                         0x49
#define Setblock                        0x4A

/*
 *  Process Group
 */
#define Exec                            0x4B
#define Exit                            0x4C
#define WaitProcess                     0x4D
#define Find_First                      0x4E

/*
 *  Special Group
 */
#define Find_Next                       0x4F

/*
 *  Special System Group
 */
#define Set_Current_PDB                 0x50    /* ;Internal */
#define Get_Current_PDB                 0x51    /* ;Internal */
#define Get_In_Vars                     0x52    /* ;Internal */
#define SetDPB                          0x53    /* ;Internal */
#define Get_Verify_On_Write             0x54
#define Dup_PDB                         0x55    /* ;Internal */
#define Rename                          0x56
#define File_Times                      0x57
#define File_Times_Get_Mod                0x00
#define File_Times_Set_Mod                0x01
#define File_Times_Get_EA                 0x02  // For OS/2
#define File_Times_Set_EA                 0x03  // For OS/2
#define File_Times_Get_Acc                0x04
#define File_Times_Set_Acc                0x05

#define AllocOper                       0x58

/*
 *  Network extention system calls
 */
#define GetExtendedError                0x59
#define CreateTempFile                  0x5A
#define CreateNewFile                   0x5B
#define LockOper                        0x5C    // Lock and Unlock
#define ServerCall                      0x5D    // CommitAll, ServerDOSCall,    /* ;Internal */
                                                // CloseByName, CloseUser,      /* ;Internal */
                                                // CloseUserProcess,            /* ;Internal */
                                                // GetOpenFileList              /* ;Internal */
#define UserOper                        0x5E    // Get and Set
#define AssignOper                      0x5F    // On, Off, Get, Set, Cancel
#define xNameTrans                      0x60
#define PathParse                       0x61
#define GetCurrentPSP                   0x62
#define Hongeul                         0x63
#define ECS_CALL                        0x63    // DBCS support
#define Set_Printer_Flag                0x64    /* ;Internal */
#define GetExtCntry                     0x65
#define GetSetCdPg                      0x66
#define ExtHandle                       0x67
#define Commit                          0x68
#define GetSetMediaID                   0x69
#define IFS_IOCTL                       0x6B
#define ExtOpen                         0x6C
#define ROM_FIND_FIRST                  0x6D    /* ;Internal */
#define ROM_FIND_NEXT                   0x6E    /* ;Internal */
#define ROM_EXCLUDE                     0x6F    /* ;Internal */
#define GetSetNLS                       0x70
#define LN_Generic                      0x71
#define LN_FindClose                    0x72
#define Get_Set_DriveInfo               0x73
#define Get_DriveInfo                     0x00
#define Set_DriveInfo                     0x01
#define DriveInfo_AccDate                   0x00
#define DriveInfo_Commit                    0x01
#define AccDate_Disable                       0x00
#define AccDate_Enable                        0x02
#define Commit_Enable                         0x00
#define Commit_Disable                        0x08
#define Set_Oem_Handler                 0xF8
#define OEM_C1                          0xF9
#define OEM_C2                          0xFA
#define OEM_C3                          0xFB
#define OEM_C4                          0xFC
#define OEM_C5                          0xFD
#define OEM_C6                          0xFE
#define OEM_C7                          0xFF
