/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    exterr.h

Abstract:

    Taken from mvdm\dos\v86\inc\error.inc

Author:

    Richard L Firth (rfirth) 17-Oct-1992

Revision History:

--*/

/** ERROR.INC - DOS Error Codes
;
;    The newer (DOS 2.0 and above) "XENIX-style" calls
;    return error codes through AX. If an error occurred then
;    the carry bit will be set and the error code is in AX. If no error
;    occurred then the carry bit is reset and AX contains returned info.
;
;    Since the set of error codes is being extended as we extend the operating
;    system, we have provided a means for applications to ask the system for a
;    recommended course of action when they receive an error.
;
;    The GetExtendedError system call returns a universal error, an error
;    location and a recommended course of action.   The universal error code is
;    a symptom of the error REGARDLESS of the context in which GetExtendedError
;    is issued.
*/

//  2.0 error codes

#define error_invalid_function      1
#define error_file_not_found        2
#define error_path_not_found        3
#define error_too_many_open_files   4
#define error_access_denied         5
#define error_invalid_handle        6
#define error_arena_trashed         7
#define error_not_enough_memory     8
#define error_invalid_block         9
#define error_bad_environment       10
#define error_bad_format            11
#define error_invalid_access        12
#define error_invalid_data          13
/**** reserved          EQU 14  ; *****/
#define error_invalid_drive         15
#define error_current_directory     16
#define error_not_same_device       17
#define error_no_more_files         18

//  These are the universal int 24 mappings for the old INT 24 set of errors

#define error_write_protect         19
#define error_bad_unit              20
#define error_not_ready             21
#define error_bad_command           22
#define error_CRC                   23
#define error_bad_length            24
#define error_Seek                  25
#define error_not_DOS_disk          26
#define error_sector_not_found      27
#define error_out_of_paper          28
#define error_write_fault           29
#define error_read_fault            30
#define error_gen_failure           31

//  the new 3.0 error codes reported through INT 24

#define error_sharing_violation     32
#define error_lock_violation        33
#define error_wrong_disk            34
#define error_FCB_unavailable       35
#define error_sharing_buffer_exceeded   36
#define error_Code_Page_Mismatched  37    // DOS 4.00           ;AN000;
#define error_handle_EOF            38    // DOS 4.00           ;AN000;
#define error_handle_Disk_Full      39    // DOS 4.00           ;AN000;

//  New OEM network-related errors are 50-79

#define error_not_supported         50

#define error_net_access_denied     65    //M028

//  End of INT 24 reportable errors

#define error_file_exists           80
#define error_DUP_FCB               81  // *****
#define error_cannot_make           82
#define error_FAIL_I24              83

//  New 3.0 network related error codes

#define error_out_of_structures     84
#define error_Already_assigned      85
#define error_invalid_password      86
#define error_invalid_parameter     87
#define error_NET_write_fault       88
#define error_sys_comp_not_loaded   90    // DOS 4.00               ;AN000;



//  BREAK <Interrupt 24 error codes>

/** Int24 Error Codes **/

#define error_I24_write_protect     0
#define error_I24_bad_unit          1
#define error_I24_not_ready         2
#define error_I24_bad_command       3
#define error_I24_CRC               4
#define error_I24_bad_length        5
#define error_I24_Seek              6
#define error_I24_not_DOS_disk      7
#define error_I24_sector_not_found  8
#define error_I24_out_of_paper      9
#define error_I24_write_fault       0xA
#define error_I24_read_fault        0xB
#define error_I24_gen_failure       0xC
// NOTE: Code 0DH is used by MT-DOS.
#define error_I24_wrong_disk        0xF


//  THE FOLLOWING ARE MASKS FOR THE AH REGISTER ON Int 24
//
//  NOTE: ABORT is ALWAYS allowed

#define Allowed_FAIL        0x08    // 00001000B
#define Allowed_RETRY       0x10    // 00010000B
#define Allowed_IGNORE      0x20    // 00100000B

#define I24_operation       0x01    // 00000001B    ;Z if READ,NZ if Write
#define I24_area            0x60    // 00000110B    ; 00 if DOS
                                    //              ; 01 if FAT
                                    //              ; 10 if root DIR
                                    //              ; 11 if DATA
#define I24_class           0x80    // 10000000B    ;Z if DISK, NZ if FAT or char


//  BREAK <GetExtendedError CLASSes ACTIONs LOCUSs>

/** The GetExtendedError call takes an error code and returns CLASS,
;   ACTION and LOCUS codes to help programs determine the proper action
;   to take for error codes that they don't explicitly understand.
*/

//  Values for error CLASS

#define errCLASS_OutRes     1   // Out of Resource
#define errCLASS_TempSit    2   // Temporary Situation
#define errCLASS_Auth       3   // Permission problem
#define errCLASS_Intrn      4   // Internal System Error
#define errCLASS_HrdFail    5   // Hardware Failure
#define errCLASS_SysFail    6   // System Failure
#define errCLASS_Apperr     7   // Application Error
#define errCLASS_NotFnd     8   // Not Found
#define errCLASS_BadFmt     9   // Bad Format
#define errCLASS_Locked     10  // Locked
#define errCLASS_Media      11  // Media Failure
#define errCLASS_Already    12  // Collision with Existing Item
#define errCLASS_Unk        13  // Unknown/other

//  Values for error ACTION

#define errACT_Retry        1   // Retry
#define errACT_DlyRet       2   // Delay Retry, retry after pause
#define errACT_User         3   // Ask user to regive info
#define errACT_Abort        4   // abort with clean up
#define errACT_Panic        5   // abort immediately
#define errACT_Ignore       6   // ignore
#define errACT_IntRet       7   // Retry after User Intervention

//  Values for error LOCUS

#define errLOC_Unk          1   // No appropriate value
#define errLOC_Disk         2   // Random Access Mass Storage
#define errLOC_Net          3   // Network
#define errLOC_SerDev       4   // Serial Device
#define errLOC_Mem          5   // Memory
