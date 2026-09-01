#pragma once

#include <ndk/kbd.h>
#include <hidclass.h>

typedef struct tagKBDNLSLAYER
{
    USHORT OEMIdentifier;
    USHORT LayoutInformation;
    UINT NumOfVkToF;
    struct _VK_TO_FUNCTION_TABLE *pVkToF;
    INT NumOfMouseVKey;
    PUSHORT pusMouseVKey;
} KBDNLSLAYER, *PKBDNLSLAYER;
 
typedef struct tagKBDFILE
{
    HEAD head;
    struct tagKBDFILE *pkfNext;
    WCHAR awchKF[20];
    HANDLE hBase;
    struct _KBDTABLES *pKbdTbl;
    ULONG Size;
    PKBDNLSLAYER pKbdNlsTbl;
} KBDFILE, *PKBDFILE;

typedef struct tagKL
{
    HEAD head;
    struct tagKL *pklNext;
    struct tagKL *pklPrev;
    DWORD dwKL_Flags;
    HKL hkl;
    PKBDFILE spkf;
    DWORD dwFontSigs;
    UINT iBaseCharset;
    USHORT CodePage;
    WCHAR wchDiacritic;
    PIMEINFOEX piiex;
} KL, *PKL;

typedef struct _ATTACHINFO
{
  struct _ATTACHINFO *paiNext;
  PTHREADINFO pti1;
  PTHREADINFO pti2;
} ATTACHINFO, *PATTACHINFO;

extern PATTACHINFO gpai;

/* Key States */
#define KS_DOWN_BIT      0x80
#define KS_LOCK_BIT      0x01
/* Scan Codes */
#define SC_KEY_UP        0x8000
/* lParam bits */
#define LP_DO_NOT_CARE_BIT (1<<25) // For GetKeyNameText

/* General */
CODE_SEG("INIT") NTSTATUS NTAPI InitInputImpl(VOID);
VOID NTAPI RawInputThreadMain(VOID);
BOOL FASTCALL IntBlockInput(PTHREADINFO W32Thread, BOOL BlockIt);
NTSTATUS FASTCALL UserAttachThreadInput(PTHREADINFO,PTHREADINFO,BOOL);
BOOL FASTCALL IsRemoveAttachThread(PTHREADINFO);
VOID FASTCALL DoTheScreenSaver(VOID);
#define ThreadHasInputAccess(W32Thread) (TRUE)

/* Keyboard */
CODE_SEG("INIT") NTSTATUS NTAPI InitKeyboardImpl(VOID);
VOID NTAPI UserInitKeyboard(HANDLE hKeyboardDevice);
PKL W32kGetDefaultKeyLayout(VOID);
VOID NTAPI UserProcessKeyboardInput(PKEYBOARD_INPUT_DATA pKeyInput);
BOOL NTAPI UserSendKeyboardInput(KEYBDINPUT *pKbdInput, BOOL bInjected);
PKL NTAPI UserHklToKbl(HKL hKl);
BOOL NTAPI UserSetDefaultInputLang(HKL hKl);
extern DWORD gdwLanguageToggleKey;
extern DWORD gdwLayoutToggleKey;
extern BOOL gbEnableHexNumpad;

/* Mouse */
WORD FASTCALL UserGetMouseButtonsState(VOID);
VOID NTAPI UserProcessMouseInput(PMOUSE_INPUT_DATA pMouseInputData);
BOOL NTAPI UserSendMouseInput(MOUSEINPUT *pMouseInput, BOOL bInjected);

/* IMM */
UINT FASTCALL IntImmProcessKey(
    _In_ PUSER_MESSAGE_QUEUE MessageQueue,
    _In_ PWND pWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam);
VOID FASTCALL IntFreeImeHotKeys(VOID);

/* Input devices */
typedef struct _MOUSE_DEVICE_INFO
{
    MOUSE_ATTRIBUTES Attributes;
    MOUSE_INPUT_DATA Data;
} MOUSE_DEVICE_INFO, *PMOUSE_DEVICE_INFO;

typedef struct _KEYBOARD_DEVICE_INFO
{
    KEYBOARD_ATTRIBUTES Attributes;
    KEYBOARD_INPUT_DATA Data;
} KEYBOARD_DEVICE_INFO, *PKEYBOARD_DEVICE_INFO;

typedef struct _HID_DEVICE_INFO
{
    PHIDP_PREPARSED_DATA PreparsedData;
    HID_COLLECTION_INFORMATION CollectionInformation;
    HIDP_CAPS Caps;
} HID_DEVICE_INFO, *PHID_DEVICE_INFO;

typedef struct _INPUT_DEVICE_INFO
{
#define INPUT_DEVICE_TYPE_MOUSE    0 /* RIM_TYPEMOUSE in winuser.h */
#define INPUT_DEVICE_TYPE_KEYBOARD 1 /* RIM_TYPEKEYBOARD in winuser.h */
#define INPUT_DEVICE_TYPE_HID      2 /* RIM_TYPEHID in winuser.h */
    UCHAR DeviceType;
    UNICODE_STRING DeviceName;
    struct _INPUT_DEVICE_INFO *pNextDeviceInfo;
    HANDLE Handle;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    union
    {
        MOUSE_DEVICE_INFO Mouse;
        KEYBOARD_DEVICE_INFO Keyboard;
        HID_DEVICE_INFO Hid;
    };
} INPUT_DEVICE_INFO, *PINPUT_DEVICE_INFO;

extern DWORD gSystemFS;
extern UINT gSystemCPCharSet; 
extern HANDLE ghKeyboardDevice;
extern PTHREADINFO ptiRawInput;
extern BYTE gafAsyncKeyState[256 * 2 / 8]; // 2 bits per key
extern PINPUT_DEVICE_INFO gpInputDeviceInfo;
extern PERESOURCE gpDeviceInfoListMutex;

FORCEINLINE
VOID
AcquireDeviceInfoListMutex(VOID)
{
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(gpDeviceInfoListMutex, TRUE);
}

FORCEINLINE
VOID
ReleaseDeviceInfoListMutex(VOID)
{
    ExReleaseResourceLite(gpDeviceInfoListMutex);
    KeLeaveCriticalRegion();
}

#define GET_KS_BYTE(vk) ((vk) * 2 / 8)
#define GET_KS_DOWN_BIT(vk) (1 << (((vk) % 4)*2))
#define GET_KS_LOCK_BIT(vk) (1 << (((vk) % 4)*2 + 1))
#define IS_KEY_DOWN(ks, vk) (((ks)[GET_KS_BYTE(vk)] & GET_KS_DOWN_BIT(vk)) ? TRUE : FALSE)
#define IS_KEY_LOCKED(ks, vk) (((ks)[GET_KS_BYTE(vk)] & GET_KS_LOCK_BIT(vk)) ? TRUE : FALSE)
#define SET_KEY_DOWN(ks, vk, down) (ks)[GET_KS_BYTE(vk)] = ((down) ? \
                                                            ((ks)[GET_KS_BYTE(vk)] | GET_KS_DOWN_BIT(vk)) : \
                                                            ((ks)[GET_KS_BYTE(vk)] & ~GET_KS_DOWN_BIT(vk)))
#define SET_KEY_LOCKED(ks, vk, down) (ks)[GET_KS_BYTE(vk)] = ((down) ? \
                                                              ((ks)[GET_KS_BYTE(vk)] | GET_KS_LOCK_BIT(vk)) : \
                                                              ((ks)[GET_KS_BYTE(vk)] & ~GET_KS_LOCK_BIT(vk)))

extern PKL gspklBaseLayout;
extern KEYBOARD_ATTRIBUTES gKeyboardInfo;
