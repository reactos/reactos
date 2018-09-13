PAGE 58,132
;******************************************************************************
TITLE SAGE - SAGE VxD
;******************************************************************************
;
;   Title:      SAGE.ASM - SAGE VxD
;
;   Version:    0.060
;
;   Date:       04/18/95
;
;   Author:     Bob Eichenlaub    
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE     REV                 DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;     04/18/95      Initial version - be
;     07/06/95      serial IRQ detection - be; credit to rjc for the basic approach
;     05/23/97      [darrenmi] major clean-up and support multiple clients
;                   for IE4
;
;==============================================================================

        .386p

;******************************************************************************
;                             I N C L U D E S
;******************************************************************************

        .XLIST
        INCLUDE VMM.Inc
        INCLUDE VWIN32.Inc
        INCLUDE VPICD.Inc
        INCLUDE SHELL.Inc
        INCLUDE VXDLDR.Inc
        INCLUDE regdef.Inc
        INCLUDE Debug.Inc
        INCLUDE Sage.Inc
        .LIST

;public SAGE_Update_User_Activity

;******************************************************************************
;              V I R T U A L   D E V I C E   D E C L A R A T I O N
;------------------------------------------------------------------------------
; The VxD declaration statement defines the VxD name, version number,
; control proc entry point, VxD ID, initialization order, and VM API 
; entry points.
;
; - Defined VxD ID: See VXDID.TXT for more information
; - Init order: If serial port detection is enabled then this Vxd MUST loaded
;               before VCOMM.VxD and after VPICD.VxD,
;               See VMM.INC for the complete
;               definition of the init order of the standard devices.
;               
;******************************************************************************

Declare_Virtual_Device sage, 1, 0, SAGE_Control, VSageID, UNDEFINED_INIT_ORDER

;******************************************************************************
;                                D A T A
;******************************************************************************

;
; Locked data
;
VxD_LOCKED_DATA_SEG

Window_List             dd  0, 0, 0, 0, 0, 0, 0, 0
cClients                dd  0       ; number of valid windows in window list
Time_Out_Idle           dd  10000   ; the interval between message posts
Time_Out_Handle         dd  0       ; Handle to the global time out we create
Last_Activity           dd  0       ; ticks that last activity occured at
Hooked_Proc             dd  0       ; shell's user_activity entry that we hooked
fUser_Active            dd  0       ; did user do anything?
fHooked_Service         dd  0       ; did we ever hook the service?

VxD_LOCKED_DATA_ENDS

;******************************************************************************
;                               C O D E
;------------------------------------------------------------------------------
; The 'body' of the VxD is in the standard code segment.
;******************************************************************************

VxD_CODE_SEG

BeginProc SAGE_Start_Idle_Timer

        push    esi

        ; check to see if we've already got a timer
        mov     esi, [Time_Out_Handle]
        test    esi, esi
        jnz     start1

        ; get a timer
        mov     eax, [Time_Out_Idle]
        mov     esi, OFFSET32 SAGE_User_Idle_Check
        VMMCall Set_Global_Time_Out
        mov     [Time_Out_Handle], esi

start1:
        pop     esi
        ret

EndProc SAGE_Start_Idle_Timer

BeginProc SAGE_Stop_Idle_Timer

        push    esi

        ; check to see if we have a timer
        mov     esi, [Time_Out_Handle]
        test    esi, esi
        jz      stop1

        ; kill it
        VMMCall Cancel_Time_Out
        xor     esi, esi
        mov     [Time_Out_Handle], esi

stop1:
        pop     esi
        ret

EndProc SAGE_Stop_Idle_Timer


;******************************************************************************
;
;   SAGE_Device_IO
;
;   DESCRIPTION:
;       This is the routine that is called when the CreateFile or
;       DeviceIoControl is made
;
;   ENTRY:
;       ESI = Pointer to args (see VWIN32.INC for struct definition)
;
;   EXIT:
;       EAX = return value
;
;   USES:
;       flags
;
;==============================================================================

BeginProc SAGE_Device_IO

        mov     ecx, [esi.dwIOControlCode]
        test    ecx, ecx
        jnz     next1

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ;
        ; DIOC_GETVERSION
        ;

        ; hook activity service if necessary
        bts     [fHooked_Service], 0
        jc      getver1

        ; hook activity service
        mov     esi, offset32 SAGE_Update_User_Activity
        GetVxDServiceOrdinal eax, SHELL_Update_User_Activity
        VMMCall Hook_Device_Service

getver1:
        xor     eax, eax                    ; success
        ret

next1:
        cmp     ecx,-1
        jne     next2

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ;
        ; DIOC_CLOSE
        ;

        ; see if we have clients to close
        cmp     [cClients], 0
        jz      close1

        ; we do... see if it's the last one...
        dec     [cClients]
        cmp     [cClients], 0
        jnz     close1

        ; last client going away - clean up
        call    SAGE_Stop_Idle_Timer

        btr     [fHooked_Service], 0
        jnc     close1

        ; unhook activity service
        GetVxDServiceOrdinal    eax,SHELL_Update_User_Activity
        mov     esi, offset32 SAGE_Update_User_Activity
        VMMCall Unhook_Device_Service

close1:
        xor     eax,eax     ; success
        ret

next2:
        cmp     ecx,1
        jne     next3

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ;
        ; Set handle and timeout
        ;
        mov     ebx, [esi.lpvInBuffer]

        ; Try to find window handle
        mov     eax, [ebx]
        mov     ecx, [cClients]
        test    ecx, ecx
        jz      addtolist

ioloop:
        cmp     eax, [Window_List + 4 * ecx - 4]
        je      timeout

        dec     ecx
        jnz     ioloop

        ; Can't find it - add it to window list
        mov     ecx, [cClients]
        cmp     ecx, 8
        jnl     timeout

addtolist:
        inc     [cClients]
        mov     [Window_List + 4 * ecx], eax

timeout:
        ; update timeout if specified
        mov     eax, [ebx+8]
        test    eax, eax
        jz      config1

        mov     [Time_Out_Idle], eax

config1:
        call    SAGE_Start_Idle_Timer

        xor     eax, eax    ; success
        ret
next3:
        cmp     ecx, 2
        jne     next4

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ;
        ; Query last activity
        ;
        mov     ebx, [esi.lpvInBuffer]
        mov     eax, [Last_Activity]
        mov     [ebx], eax
        xor     eax, eax
        ret

next4:

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ;
        ; some unsupported value...
        ;
        mov     eax, ERROR_NOT_SUPPORTED
        ret

EndProc SAGE_Device_IO


VxD_CODE_ENDS



;******************************************************************************
;                      P A G E   L O C K E D   C O D E
;------------------------------------------------------------------------------
;       Memory is a scarce resource. Use this only where necessary.
;******************************************************************************
VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   SAGE_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's to control system operation. Every VxD needs this function
;       regardless if messages are processed or not. The control proc must
;       be in the LOCKED code segment.
;
;       The Control_Dispatch macro used in this procedure simplifies
;       the handling of messages. To handle a particular message, add
;       a Control_Dispatch statement with the message name, followed
;       by the procedure that should handle the message. 
;
;   ENTRY:
;       EAX = Message number
;       EBX = VM Handle
;
;==============================================================================

BeginProc SAGE_Control

        Control_Dispatch W32_DEVICEIOCONTROL, SAGE_Device_IO
        clc
        ret

EndProc SAGE_Control


BeginDoc
;******************************************************************************
;
; SAGE_Update_User_Activity
;
; DESCRIPTION:
;
;   This service is called by VMD, VKD to tell us that user input occured
; ENTRY:    None
; EXIT:     None
; USES:     NONE
;==============================================================================
EndDoc
BeginProc SAGE_Update_User_Activity, HOOK_PROC, Hooked_Proc

    push    eax

    ; flag something happened
    inc     [fUser_Active]

    ; save off the time
    VMMCall Get_Last_Updated_System_Time
    mov     [Last_Activity], eax

    pop     eax
    jmp     Hooked_Proc

EndProc SAGE_Update_User_Activity


VxD_LOCKED_CODE_ENDS

VxD_PAGEABLE_CODE_SEG

;******************************************************************************
;
; SAGE_User_Idle_Check
;
; DESCRIPTION:
;
;   This checks if key/mouse or serial port (comm) event has occurred.
;
; Entry:
;   None
; Exit:
;   None
; Uses:
;   ALL
;******************************************************************************

BeginProc SAGE_User_Idle_Check,High_Freq,PUBLIC                                 
                                                                                
    ;                                                                           
    ; clear handle                                                              
    ;                                                                           
    xor     ecx, ecx                                                            
    mov     [Time_Out_Handle], ecx                                              
                                                                                
    ;                                                                           
    ; check for idleness                                                        
    ;                                                                           
    xchg    ecx, [fUser_Active]
    test    ecx, ecx                                                            
    jz      ResetTimer                                                          
                                                                                
    ;                                                                           
    ; Not idle so post a message to all clients who want to know
    ;                     
    xor     eax, eax                                                            
    mov     ecx, [cClients]
    test    ecx, ecx                                                            
    jz      ResetTimer                                                          

loop0:

    ; get next window
    mov     ebx, [4 * ecx + Window_List - 4]

    ; skip if it's -1
    cmp     ebx, -1
    je      loop1
                                                                                
    ; post message
    push    ecx
    VxDCall _SHELL_PostMessage, <ebx, WM_SAGE_MSG, eax, eax, eax, eax>          
    pop     ecx

loop1:
    dec     ecx
    jnz     loop0

    ;                                                                           
    ; reset the timer so we check again later                                   
    ;                                                                           
ResetTimer:                                                                     
    call    SAGE_Start_Idle_Timer
    ret                                                                         
                                                                                
EndProc SAGE_User_Idle_Check                                                    

VxD_PAGEABLE_CODE_ENDS


;******************************************************************************
;                       R E A L   M O D E   C O D E
;******************************************************************************

;******************************************************************************
;
;       Real mode initialization code
;
;   DESCRIPTION:
;       This code is called when the system is still in real mode, and
;       the VxDs are being loaded.
;
;       This routine as coded shows how a VxD (with a defined VxD ID)
;       could check to see if it was being loaded twice, and abort the 
;       second without an error message. Note that this would require
;       that the VxD have an ID other than Undefined_Device_ID. See
;       the file VXDID.TXT more details.
;
;   ENTRY:
;       AX = VMM Version
;       BX = Flags
;               Bit 0: duplicate device ID already loaded 
;               Bit 1: duplicate ID was from the INT 2F device list
;               Bit 2: this device is from the INT 2F device list
;       EDX = Reference data from INT 2F response, or 0
;       SI = Environment segment, passed from MS-DOS
;
;   EXIT:
;       BX = ptr to list of pages to exclude (0, if none)
;       SI = ptr to list of instance data items (0, if none)
;       EDX = DWORD of reference data to be passed to protect mode init
;
;==============================================================================

VxD_REAL_INIT_SEG

BeginProc SAGE_Real_Init_Proc

        test    bx, Duplicate_Device_ID ; check for already loaded
        jnz     short duplicate         ; jump if so

        xor     bx, bx                  ; no exclusion table
        xor     si, si                  ; no instance data table
        xor     edx, edx                ; no reference data

        mov     ax, Device_Load_Ok
        ret

duplicate:
        mov     ax, Abort_Device_Load + No_Fail_Message
        ret

EndProc SAGE_Real_Init_Proc


VxD_REAL_INIT_ENDS


        END
