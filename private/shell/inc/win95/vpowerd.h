/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       VPOWERD.H
*
*  VERSION:     1.0
*
*  DATE:        01 Oct 1993
*
*  AUTHOR:      TCS
*
*  Definitions for the Virtual Power Management Device.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  01 Oct 1993 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_VPOWERD
#define _INC_VPOWERD

#ifndef Not_VxD

//
//  Virtual Power Management Device service table.
//

/*XLATOFF*/
#define VPOWERD_Service                 Declare_Service
/*XLATON*/

/*MACROS*/
Begin_Service_Table(VPOWERD, VxD)

    VPOWERD_Service     (_VPOWERD_Get_Version, VxD_PAGEABLE_CODE)
    VPOWERD_Service     (_VPOWERD_Get_APM_BIOS_Version, VxD_PAGEABLE_CODE)
    VPOWERD_Service     (_VPOWERD_Get_Power_Management_Level, VxD_PAGEABLE_CODE)
    VPOWERD_Service     (_VPOWERD_Set_Power_Management_Level, VxD_PAGEABLE_CODE)
    VPOWERD_Service     (_VPOWERD_Set_Device_Power_State, VxD_PAGEABLE_CODE)
    VPOWERD_Service     (_VPOWERD_Set_System_Power_State, VxD_LOCKED_CODE)
    VPOWERD_Service     (_VPOWERD_Restore_Power_On_Defaults, VxD_PAGEABLE_CODE)
    VPOWERD_Service     (_VPOWERD_Get_Power_Status, VxD_PAGEABLE_CODE)
    VPOWERD_Service     (_VPOWERD_Get_Power_State, VxD_PAGEABLE_CODE)
    VPOWERD_Service     (_VPOWERD_OEM_APM_Function, VxD_PAGEABLE_CODE)
    VPOWERD_Service     (_VPOWERD_Register_Power_Handler, VxD_PAGEABLE_CODE)
    VPOWERD_Service     (_VPOWERD_Deregister_Power_Handler, VxD_PAGEABLE_CODE)
    VPOWERD_Service     (_VPOWERD_W32_Get_System_Power_Status, VxD_PAGEABLE_CODE)	
	VPOWERD_Service     (_VPOWERD_W32_Set_System_Power_State, VxD_PAGEABLE_CODE)
	// APM 1.2 services
	VPOWERD_Service     (_VPOWERD_Get_Capabilities, VxD_PAGEABLE_CODE)
	VPOWERD_Service     (_VPOWERD_Enable_Resume_On_Ring, VxD_PAGEABLE_CODE)
	VPOWERD_Service     (_VPOWERD_Disable_Resume_On_Ring, VxD_PAGEABLE_CODE)
	VPOWERD_Service     (_VPOWERD_Set_Resume_Timer, VxD_PAGEABLE_CODE)
	VPOWERD_Service     (_VPOWERD_Get_Resume_Timer, VxD_PAGEABLE_CODE)
	VPOWERD_Service     (_VPOWERD_Disable_Resume_Timer, VxD_PAGEABLE_CODE)
	VPOWERD_Service     (_VPOWERD_Enable_Timer_Based_Requests, VxD_PAGEABLE_CODE)
	VPOWERD_Service     (_VPOWERD_Disable_Timer_Based_Requests, VxD_PAGEABLE_CODE)
	VPOWERD_Service     (_VPOWERD_W32_Get_Power_Status, VxD_PAGEABLE_CODE)
	VPOWERD_Service		(_VPOWERD_Get_Timer_Based_Requests_Status, VxD_PAGEABLE_CODE)
	VPOWERD_Service		(_VPOWERD_Get_Ring_Resume_Status, VxD_PAGEABLE_CODE)

End_Service_Table(VPOWERD, VxD)
/*ENDMACROS*/


#endif

/*XLATOFF*/

#ifdef IS_32
#define POWERFAR
#else
#define POWERFAR                        _far
#endif

/*XLATON*/

//
//  Standard return type from VPOWERD services and handlers.
//
//  Error codes 0x00000001 through 0x000000FF are reserved for APM firmware
//  errors.
//
//  Error codes above 0x80000000 are reserved for definition by VPOWERD
//

typedef DWORD                           POWERRET;

#define PR_SUCCESS                      0x00000000

#define PR_PM_DISABLED                  0x00000001
#define PR_RM_CONNECT_EXISTS            0x00000002
#define PR_INTERFACE_NOT_CONNECTED      0x00000003
#define PR_16BIT_PM_CONNECT_EXISTS      0x00000005
#define PR_16BIT_PM_UNSUPPORTED         0x00000006
#define PR_32BIT_PM_CONNECT_EXISTS      0x00000007
#define PR_32BIT_PM_UNSUPPORTED         0x00000008
#define PR_UNRECOGNIZED_DEVICE_ID       0x00000009
#define PR_PARAMETER_OUT_OF_RANGE       0x0000000A
#define PR_INTERFACE_NOT_ENGAGED        0x0000000B
// 	APM 1.2 error codes
#define PR_FUNC_NOT_SUPPORTED			0x0000000C
#define PR_RESUME_TIMER_DISABLED		0x0000000D
//	end
#define PR_CANNOT_ENTER_STATE           0x00000060
#define PR_NO_PM_EVENTS_PENDING         0x00000080
#define PR_APM_NOT_PRESENT              0x00000086
#define PR_UNDEFINED_FUNCTION           0x000000FF

#define PR_DEFAULT                      0x80000000
#define PR_FAILURE                      0x80000001
#define PR_REQUEST_VETOED               0x80000002
#define PR_INVALID_POINTER              0x80000003
#define PR_INVALID_FLAG                 0x80000004
#define PR_INVALID_PMLEVEL              0x80000005
#define PR_INVALID_DEVICE_ID            0x80000006
#define PR_INVALID_POWER_STATE          0x80000007
#define PR_INVALID_REQUEST_TYPE         0x80000008
#define PR_OUT_OF_MEMORY                0x80000009
#define PR_DUPLICATE_POWER_HANDLER      0x8000000A
#define PR_POWER_HANDLER_NOT_FOUND      0x8000000B
#define PR_INVALID_FUNCTION             0x8000000C

//
//  Power device ID type and standard IDs as defined by the APM 1.1
//  specification.
//

typedef DWORD                           POWER_DEVICE_ID;

#define PDI_APM_BIOS                    0x0000
#define PDI_MANAGED_BY_APM_BIOS         0x0001
#define PDI_MANAGED_BY_APM_BIOS_OLD     0xFFFF
// 	APM 1.2
#define PDI_SPECIFIC_BATTERY			0x8000	// OR in the 1 based battery unit number
//	end

//
//  Power state type and standard power states as defined by the APM 1.1
//  specification.
//

typedef DWORD                           POWER_STATE;
typedef POWER_STATE POWERFAR*           LPPOWER_STATE;

#define PSTATE_APM_ENABLED              0x0000
#define PSTATE_STANDBY                  0x0001
#define PSTATE_SUSPEND                  0x0002
#define PSTATE_OFF                      0x0003
//	APM 1.2
#define PSTATE_HIBERNATE				0x0006
//  end

//
//  Valid power management levels.
//

#define PMLEVEL_ADVANCED                0
#define PMLEVEL_STANDARD                1
#define PMLEVEL_OFF                     2
#define PMLEVEL_MAXIMUM                 PMLEVEL_OFF

//
//  Valid request types.
//

#define REQTYPE_USER_INITIATED          0x00000000
#define REQTYPE_TIMER_INITIATED         0x00000001
#define REQTYPE_FORCED_REQUEST          0x00000002
#define REQTYPE_BIOS_CRITICAL_SUSPEND   0x00000003
#define REQTYPE_FROM_BIOS_FLAG          0x80000000                  // ;Internal

// valid values for Status returned by Get_Timer_Based_Requests_Status and Get_Ring_Resume_Status
#define CAPABILITY_ENABLED	0x00000001
#define CAPABILITY_DISABLED	0x00000000

//
//  Power status structures returned by _VPOWERD_Get_Power_Status and
//  _VPOWERD_W32_Get_Power_Status.
//

#ifndef NOPOWERSTATUSDEFINES

#define AC_LINE_OFFLINE                 0x00
#define AC_LINE_ONLINE                  0x01
#define AC_LINE_BACKUP_POWER            0x02
#define AC_LINE_UNKNOWN                 0xFF

#define BATTERY_STATUS_HIGH             0x00
#define BATTERY_STATUS_LOW              0x01
#define BATTERY_STATUS_CRITICAL         0x02
#define BATTERY_STATUS_CHARGING         0x03
#define BATTERY_STATUS_UNKNOWN          0xFF

#define BATTERY_FLAG_HIGH               0x01
#define BATTERY_FLAG_LOW                0x02
#define BATTERY_FLAG_CRITICAL           0x04
#define BATTERY_FLAG_CHARGING           0x08
//	APM 1.2 
#define BATTERY_NOT_PRESENT				0x10
//	end
#define BATTERY_FLAG_NO_BATTERY         0x80
#define BATTERY_FLAG_UNKNOWN            0xFF

#define BATTERY_PERCENTAGE_UNKNOWN      0xFF

#define BATTERY_LIFE_MINUTES_MASK       0x8000
#define BATTERY_LIFE_UNKNOWN            0xFFFF

#define BATTERY_LIFE_W32_UNKNOWN        0xFFFFFFFF

#endif  // NOPOWERSTATUSDEFINES

typedef struct _POWER_STATUS {
    BYTE PS_AC_Line_Status;
    BYTE PS_Battery_Status;
    BYTE PS_Battery_Flag;
    BYTE PS_Battery_Life_Percentage;
    WORD PS_Battery_Life_Time;	
}   POWER_STATUS;

typedef POWER_STATUS POWERFAR* LPPOWER_STATUS;

typedef struct _POWERTIME {  // st  
    WORD Year;
    WORD Month;
    WORD DayOfWeek;
    WORD Day;
    WORD Hour;
    WORD Minute;
    WORD Second;
    WORD Milliseconds;
} POWERTIME;

typedef POWERTIME POWERFAR* LPPOWERTIME;

typedef struct APM_CAPABILITIES_S	{
		WORD Capabilities;
		BYTE BatteryCount;
		BYTE Reserved;
}APM_CAPABILITIES, *PAPM_CAPABILITIES;

/*
Capability flags
Bit 0 = 1  System can enter global standby state. Indicates BIOS will post standby and standby-resume events.
Bit 1 = 1  System can enter global suspend state. Indicates BIOS will post suspend and suspend-resume events.
Bit 2 = 1  Resume timer will wake up from standby.
Bit 3 = 1  Resume timer will wake up from suspend.
Bit 4 = 1  Resume on ring indicator (internal COM or modem) will wake up from standby.
Bit 5 = 1  Resume on ring indicator (internal COM or modem) will wake up from suspend.
Bit 6 = 1  PCMCIA Ring indicator will wake up from standby.
Bit 7 = 1  PCMCIA Ring indicator will wake up from suspend.
Other bits  Reserved (must be set to 0)
*/

#define GLOBAL_STANDBY_SUPPORTED_BIT		0
#define GLOBAL_STANDBY_SUPPORTED			(1 << GLOBAL_STANDBY_SUPPORTED_BIT)

#define GLOBAL_SUSPEND_SUPPORTED_BIT  		1
#define GLOBAL_SUSPEND_SUPPORTED			(1 << GLOBAL_SUSPEND_SUPPORTED_BIT)

#define WAKE_ON_TIMER_STANDBY_BIT			2
#define WAKE_ON_TIMER_STANDBY				(1 << WAKE_ON_TIMER_STANDBY_BIT)
		
#define WAKE_ON_TIMER_SUSPEND_BIT			3
#define WAKE_ON_TIMER_SUSPEND				(1 << WAKE_ON_TIMER_SUSPEND_BIT)

#define RING_RESUME_INTERNAL_STANDBY_BIT	4
#define RING_RESUME_INTERNAL_STANDBY		(1 << RING_RESUME_INTERNAL_STANDBY_BIT)

#define RING_RESUME_INTERNAL_SUSPEND_BIT	5
#define RING_RESUME_INTERNAL_SUSPEND		(1 << RING_RESUME_INTERNAL_SUSPEND_BIT)

#define RING_RESUME_PCMCIA_STANDBY_BIT		6
#define RING_RESUME_PCMCIA_STANDBY			(1 << RING_RESUME_PCMCIA_STANDBY_BIT)

#define RING_RESUME_PCMCIA_SUSPEND_BIT		7
#define RING_RESUME_PCMCIA_SUSPEND			(1 << RING_RESUME_PCMCIA_SUSPEND_BIT)


typedef struct _WIN32_SYSTEM_POWER_STATUS {
    BYTE W32PS_AC_Line_Status;
    BYTE W32PS_Battery_Flag;
    BYTE W32PS_Battery_Life_Percent;
    BYTE W32PS_Reserved1;
    DWORD W32PS_Battery_Life_Time;
    DWORD W32PS_Battery_Full_Life_Time;
}   WIN32_SYSTEM_POWER_STATUS;

typedef WIN32_SYSTEM_POWER_STATUS POWERFAR* LPWIN32_SYSTEM_POWER_STATUS;

//
//  OEM APM Register Structure used by _VPOWERD_OEM_APM_Function.
//

struct _OEM_APM_BYTE_REGS {
    WORD OEMAPM_Reserved1[6];
    BYTE OEMAPM_BL;
    BYTE OEMAPM_BH;
    WORD OEMAPM_Reserved2;
    BYTE OEMAPM_DL;
    BYTE OEMAPM_DH;
    WORD OEMAPM_Reserved3;
    BYTE OEMAPM_CL;
    BYTE OEMAPM_CH;
    WORD OEMAPM_Reserved4;
    BYTE OEMAPM_AL;
    BYTE OEMAPM_AH;
    WORD OEMAPM_Reserved5;
    BYTE OEMAPM_Flags;
    BYTE OEMAPM_Reserved6[3];
};

struct _OEM_APM_WORD_REGS {
    WORD OEMAPM_DI;
    WORD OEMAPM_Reserved7;
    WORD OEMAPM_SI;
    WORD OEMAPM_Reserved8;
    WORD OEMAPM_BP;
    WORD OEMAPM_Reserved9;
    WORD OEMAPM_BX;
    WORD OEMAPM_Reserved10;
    WORD OEMAPM_DX;
    WORD OEMAPM_Reserved11;
    WORD OEMAPM_CX;
    WORD OEMAPM_Reserved12;
    WORD OEMAPM_AX;
    WORD OEMAPM_Reserved13[3];
};

struct _OEM_APM_DWORD_REGS {
    DWORD OEMAPM_EDI;
    DWORD OEMAPM_ESI;
    DWORD OEMAPM_EBP;
    DWORD OEMAPM_EBX;
    DWORD OEMAPM_EDX;
    DWORD OEMAPM_ECX;
    DWORD OEMAPM_EAX;
    DWORD OEMAPM_Reserved14;
};

typedef union _OEM_APM_REGS {
    struct _OEM_APM_BYTE_REGS ByteRegs;
    struct _OEM_APM_WORD_REGS WordRegs;
    struct _OEM_APM_DWORD_REGS DwordRegs;
}   OEM_APM_REGS;

typedef OEM_APM_REGS POWERFAR*          LPOEM_APM_REGS;

//
//  Possible power function codes that are sent to POWER_HANDLER callbacks.
//

typedef DWORD                           POWERFUNC;

#define PF_SUSPEND_PHASE1               0x00000000
#define PF_SUSPEND_PHASE2               0x00000001
#define PF_SUSPEND_INTS_OFF             0x00000002
#define PF_RESUME_INTS_OFF              0x00000003
#define PF_RESUME_PHASE2                0x00000004
#define PF_RESUME_PHASE1                0x00000005
#define PF_BATTERY_LOW                  0x00000006
#define PF_POWER_STATUS_CHANGE          0x00000007
#define PF_UPDATE_TIME                  0x00000008
#define PF_CAPABILITIES_CHANGE			0x00000009
#define PF_APMOEMEVENT_FIRST            0x00000200
#define PF_APMOEMEVENT_LAST             0x000002FF

//
//
//

#define PFG_UI_ALLOWED                  0x00000001
#define PFG_CANNOT_FAIL                 0x00000002
#define PFG_REQUEST_VETOED              0x00000004
#define PFG_REVERSE                     0x00000008
#define PFG_STANDBY                     0x00000010
#define PFG_CRITICAL                    0x00000020
#ifdef SUPPORT_HIBERNATE
#define PFG_HIBERNATE					0x00000040	// NEW for APM 1.2
#endif
#define PFG_KEPT_POWER					0x00000080

//
//  Standard POWER_HANDLER priority levels.
//

#define PHPL_PBT_BROADCAST              0x40000000
#define PHPL_UNKNOWN                    0x80000000
#define PHPL_CONFIGMG                   0xC0000000
#define PHPL_TIMER			0xE0000000

//
//
//

typedef POWERRET (*POWER_HANDLER)(POWERFUNC, DWORD);

//
//  VPOWERD BroadcastSystemMessage API parameter blocks.
//

typedef struct _VPOWERD_BSMAPI_HEADER {
    DWORD VBAPIH_Packet_Size;
    WORD VBAPIH_Function;
    DWORD VBAPIH_Return_Code;
}   VPOWERD_BSMAPI_HEADER;

#define VBAPIF_W32_SET_SYSTEM_STATE     0x000D

typedef struct _VPOWERD_BSMAPI_W32_SET_SYSTEM_STATE {
    struct _VPOWERD_BSMAPI_HEADER VBWSSS_VBAPIH;
    DWORD VBWSSS_Force_Flag;
}   VPOWERD_BSMAPI_W32_SET_SYSTEM_STATE;

//
//  Virtual Power Management Device service prototypes.
//

/*XLATOFF*/

BOOL
POWERFAR CDECL
VPOWERD_Get_Entry_Point(
    VOID
    );

DWORD
POWERFAR CDECL
_VPOWERD_Get_Version(
    VOID
    );

DWORD
POWERFAR CDECL
_VPOWERD_Get_APM_BIOS_Version(
    VOID
    );

DWORD
POWERFAR CDECL
_VPOWERD_Get_Power_Management_Level(
    VOID
    );

POWERRET
POWERFAR CDECL
_VPOWERD_Set_Power_Management_Level(
    DWORD Power_Management_Level
    );

POWERRET
POWERFAR CDECL
_VPOWERD_Set_Device_Power_State(
    POWER_DEVICE_ID Power_Device_ID,
    POWER_STATE Power_State
    );

POWERRET
POWERFAR CDECL
_VPOWERD_Set_System_Power_State(
    POWER_STATE Power_State,
    DWORD Request_Type
    );

POWERRET
POWERFAR CDECL
_VPOWERD_Restore_Power_On_Defaults(
    VOID
    );

POWERRET
POWERFAR CDECL
_VPOWERD_Get_Power_Status(
    POWER_DEVICE_ID Power_Device_ID,
    LPPOWER_STATUS lpPower_Status
    );

POWERRET
POWERFAR CDECL
_VPOWERD_Get_Power_State(
    POWER_DEVICE_ID Power_Device_ID,
    LPPOWER_STATE lpPower_State
    );

POWERRET
POWERFAR CDECL
_VPOWERD_OEM_APM_Function(
    LPOEM_APM_REGS lpOEM_APM_Regs
    );

POWERRET
POWERFAR CDECL
_VPOWERD_Register_Power_Handler(
    POWER_HANDLER Power_Handler,
    DWORD Priority
    );

POWERRET
POWERFAR CDECL
_VPOWERD_Deregister_Power_Handler(
    POWER_HANDLER Power_Handler
    );

BOOL
POWERFAR CDECL
_VPOWERD_W32_Get_System_Power_Status(
    LPWIN32_SYSTEM_POWER_STATUS lpWin32_System_Power_Status
    );

DWORD
POWERFAR CDECL
_VPOWERD_W32_Set_System_Power_State(
    BOOL Suspend_Flag,
    BOOL Force_Flag
    );


POWERRET
POWERFAR CDECL
_VPOWERD_Get_Capabilities(
	PAPM_CAPABILITIES pAPMCaps
	);

POWERRET
POWERFAR CDECL
_VPOWERD_Enable_Resume_On_Ring(
	VOID
	);

POWERRET
POWERFAR CDECL
_VPOWERD_Disable_Resume_On_Ring(
	VOID
	);

POWERRET
POWERFAR CDECL
_VPOWERD_Set_Resume_Timer(
	LPPOWERTIME pPowerTime
	);

POWERRET
POWERFAR CDECL
_VPOWERD_Get_Resume_Timer(
	LPPOWERTIME pPowerTime
	);

POWERRET
POWERFAR CDECL
_VPOWERD_Disable_Resume_Timer(
	VOID
	);

POWERRET
POWERFAR CDECL
_VPOWERD_Enable_Timer_Based_Requests(
	VOID
	);

POWERRET
POWERFAR CDECL
_VPOWERD_Disable_Timer_Based_Requests(
	VOID
	);

POWERRET
POWERFAR CDECL
_VPOWERD_W32_Get_Power_Status(
	POWER_DEVICE_ID Power_Device_ID,
	LPWIN32_SYSTEM_POWER_STATUS lpWin32_System_Power_Status
	);

/*XLATON*/

#endif // _INC_VPOWERD
