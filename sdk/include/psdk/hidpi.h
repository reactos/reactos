/*
 * hidpi.h
 *
 * Public Interface for HID parsing library.
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __HIDPI_H__
#define __HIDPI_H__

#include "hidusage.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_HIDPI_)
  #define HIDAPI
#else
  #define HIDAPI DECLSPEC_IMPORT
#endif

typedef PUCHAR PHIDP_REPORT_DESCRIPTOR;
typedef struct _HIDP_PREPARSED_DATA * PHIDP_PREPARSED_DATA;

typedef struct _HIDP_UNKNOWN_TOKEN {
  UCHAR  Token;
  UCHAR  Reserved[3];
  ULONG  BitField;
} HIDP_UNKNOWN_TOKEN, *PHIDP_UNKNOWN_TOKEN;

typedef enum _HIDP_KEYBOARD_DIRECTION {
  HidP_Keyboard_Break,
  HidP_Keyboard_Make
} HIDP_KEYBOARD_DIRECTION;

typedef struct _HIDP_KEYBOARD_MODIFIER_STATE {
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      ULONG  LeftControl : 1;
      ULONG  LeftShift : 1;
      ULONG  LeftAlt : 1;
      ULONG  LeftGUI : 1;
      ULONG  RightControl : 1;
      ULONG  RightShift : 1;
      ULONG  RightAlt : 1;
      ULONG  RigthGUI : 1;
      ULONG  CapsLock : 1;
      ULONG  ScollLock : 1;
      ULONG  NumLock : 1;
      ULONG  Reserved : 21;
    } DUMMYSTRUCTNAME;
    ULONG ul;
  } DUMMYUNIONNAME;
} HIDP_KEYBOARD_MODIFIER_STATE, *PHIDP_KEYBOARD_MODIFIER_STATE;

typedef BOOLEAN
(NTAPI *PHIDP_INSERT_SCANCODES)(
  _In_opt_ PVOID Context,
  _In_reads_bytes_(Length) PCHAR NewScanCodes,
  _In_ ULONG Length);

typedef struct _USAGE_AND_PAGE {
  USAGE  Usage;
  USAGE  UsagePage;
} USAGE_AND_PAGE, *PUSAGE_AND_PAGE;

typedef struct _HIDD_ATTRIBUTES {
  ULONG   Size;
  USHORT  VendorID;
  USHORT  ProductID;
  USHORT  VersionNumber;
} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;

typedef struct _HIDD_CONFIGURATION {
  PVOID  cookie;
  ULONG  size;
  ULONG  RingBufferSize;
} HIDD_CONFIGURATION, *PHIDD_CONFIGURATION;

_Must_inspect_result_
HIDAPI
NTSTATUS
NTAPI
HidP_TranslateUsageAndPagesToI8042ScanCodes(
  _In_reads_(UsageListLength) PUSAGE_AND_PAGE ChangedUsageList,
  _In_ ULONG UsageListLength,
  _In_ HIDP_KEYBOARD_DIRECTION KeyAction,
  _Inout_ PHIDP_KEYBOARD_MODIFIER_STATE ModifierState,
  _In_ PHIDP_INSERT_SCANCODES InsertCodesProcedure,
  _In_opt_ PVOID InsertCodesContext);

_Must_inspect_result_
HIDAPI
NTSTATUS
NTAPI
HidP_TranslateUsagesToI8042ScanCodes(
  _In_reads_(UsageListLength) PUSAGE ChangedUsageList,
  _In_ ULONG UsageListLength,
  _In_ HIDP_KEYBOARD_DIRECTION KeyAction,
  _Inout_ PHIDP_KEYBOARD_MODIFIER_STATE ModifierState,
  _In_ PHIDP_INSERT_SCANCODES InsertCodesProcedure,
  _In_opt_ PVOID InsertCodesContext);

typedef struct _HIDP_BUTTON_CAPS {
  USAGE  UsagePage;
  UCHAR  ReportID;
  BOOLEAN  IsAlias;
  USHORT  BitField;
  USHORT  LinkCollection;
  USAGE  LinkUsage;
  USAGE  LinkUsagePage;
  BOOLEAN  IsRange;
  BOOLEAN  IsStringRange;
  BOOLEAN  IsDesignatorRange;
  BOOLEAN  IsAbsolute;
  ULONG  Reserved[10];
  _ANONYMOUS_UNION union {
    struct {
      USAGE  UsageMin, UsageMax;
      USHORT  StringMin, StringMax;
      USHORT  DesignatorMin, DesignatorMax;
      USHORT  DataIndexMin, DataIndexMax;
    } Range;
    struct  {
       USAGE  Usage, Reserved1;
       USHORT  StringIndex, Reserved2;
       USHORT  DesignatorIndex, Reserved3;
       USHORT  DataIndex, Reserved4;
    } NotRange;
  } DUMMYUNIONNAME;
} HIDP_BUTTON_CAPS, *PHIDP_BUTTON_CAPS;

typedef struct _HIDP_CAPS {
  USAGE  Usage;
  USAGE  UsagePage;
  USHORT  InputReportByteLength;
  USHORT  OutputReportByteLength;
  USHORT  FeatureReportByteLength;
  USHORT  Reserved[17];
  USHORT  NumberLinkCollectionNodes;
  USHORT  NumberInputButtonCaps;
  USHORT  NumberInputValueCaps;
  USHORT  NumberInputDataIndices;
  USHORT  NumberOutputButtonCaps;
  USHORT  NumberOutputValueCaps;
  USHORT  NumberOutputDataIndices;
  USHORT  NumberFeatureButtonCaps;
  USHORT  NumberFeatureValueCaps;
  USHORT  NumberFeatureDataIndices;
} HIDP_CAPS, *PHIDP_CAPS;

typedef struct _HIDP_DATA {
  USHORT  DataIndex;
  USHORT  Reserved;
  _ANONYMOUS_UNION union {
    ULONG  RawValue;
    BOOLEAN  On;
  }  DUMMYUNIONNAME;
} HIDP_DATA, *PHIDP_DATA;

typedef struct _HIDP_EXTENDED_ATTRIBUTES {
  UCHAR  NumGlobalUnknowns;
  UCHAR  Reserved[3];
  PHIDP_UNKNOWN_TOKEN  GlobalUnknowns;
  ULONG  Data[1];
} HIDP_EXTENDED_ATTRIBUTES, *PHIDP_EXTENDED_ATTRIBUTES;

#define HIDP_LINK_COLLECTION_ROOT         ((USHORT) -1)
#define HIDP_LINK_COLLECTION_UNSPECIFIED  ((USHORT) 0)

typedef struct _HIDP_LINK_COLLECTION_NODE {
  USAGE  LinkUsage;
  USAGE  LinkUsagePage;
  USHORT  Parent;
  USHORT  NumberOfChildren;
  USHORT  NextSibling;
  USHORT  FirstChild;
  ULONG  CollectionType: 8;
  ULONG  IsAlias: 1;
  ULONG  Reserved: 23;
  PVOID  UserContext;
} HIDP_LINK_COLLECTION_NODE, *PHIDP_LINK_COLLECTION_NODE;

typedef struct _HIDP_VALUE_CAPS {
  USAGE  UsagePage;
  UCHAR  ReportID;
  BOOLEAN  IsAlias;
  USHORT  BitField;
  USHORT  LinkCollection;
  USAGE  LinkUsage;
  USAGE  LinkUsagePage;
  BOOLEAN  IsRange;
  BOOLEAN  IsStringRange;
  BOOLEAN  IsDesignatorRange;
  BOOLEAN  IsAbsolute;
  BOOLEAN  HasNull;
  UCHAR  Reserved;
  USHORT  BitSize;
  USHORT  ReportCount;
  USHORT  Reserved2[5];
  ULONG  UnitsExp;
  ULONG  Units;
  LONG  LogicalMin, LogicalMax;
  LONG  PhysicalMin, PhysicalMax;
    _ANONYMOUS_UNION union {
      struct {
        USAGE  UsageMin, UsageMax;
        USHORT  StringMin, StringMax;
        USHORT  DesignatorMin, DesignatorMax;
        USHORT  DataIndexMin, DataIndexMax;
      } Range;
      struct {
        USAGE  Usage, Reserved1;
        USHORT  StringIndex, Reserved2;
        USHORT  DesignatorIndex, Reserved3;
        USHORT  DataIndex, Reserved4;
      } NotRange;
    } DUMMYUNIONNAME;
} HIDP_VALUE_CAPS, *PHIDP_VALUE_CAPS;

typedef enum _HIDP_REPORT_TYPE {
  HidP_Input,
  HidP_Output,
  HidP_Feature
} HIDP_REPORT_TYPE;

#define FACILITY_HID_ERROR_CODE           0x11

#define HIDP_ERROR_CODES(SEV, CODE) \
  ((NTSTATUS) (((SEV) << 28) | (FACILITY_HID_ERROR_CODE << 16) | (CODE)))

#define HIDP_STATUS_SUCCESS                 (HIDP_ERROR_CODES(0x0, 0))
#define HIDP_STATUS_NULL                    (HIDP_ERROR_CODES(0x8, 1))
#define HIDP_STATUS_INVALID_PREPARSED_DATA  (HIDP_ERROR_CODES(0xC, 1))
#define HIDP_STATUS_INVALID_REPORT_TYPE     (HIDP_ERROR_CODES(0xC, 2))
#define HIDP_STATUS_INVALID_REPORT_LENGTH   (HIDP_ERROR_CODES(0xC, 3))
#define HIDP_STATUS_USAGE_NOT_FOUND         (HIDP_ERROR_CODES(0xC, 4))
#define HIDP_STATUS_VALUE_OUT_OF_RANGE      (HIDP_ERROR_CODES(0xC, 5))
#define HIDP_STATUS_BAD_LOG_PHY_VALUES      (HIDP_ERROR_CODES(0xC, 6))
#define HIDP_STATUS_BUFFER_TOO_SMALL        (HIDP_ERROR_CODES(0xC, 7))
#define HIDP_STATUS_INTERNAL_ERROR          (HIDP_ERROR_CODES(0xC, 8))
#define HIDP_STATUS_I8042_TRANS_UNKNOWN     (HIDP_ERROR_CODES(0xC, 9))
#define HIDP_STATUS_INCOMPATIBLE_REPORT_ID  (HIDP_ERROR_CODES(0xC, 0xA))
#define HIDP_STATUS_NOT_VALUE_ARRAY         (HIDP_ERROR_CODES(0xC, 0xB))
#define HIDP_STATUS_IS_VALUE_ARRAY          (HIDP_ERROR_CODES(0xC, 0xC))
#define HIDP_STATUS_DATA_INDEX_NOT_FOUND    (HIDP_ERROR_CODES(0xC, 0xD))
#define HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE (HIDP_ERROR_CODES(0xC, 0xE))
#define HIDP_STATUS_BUTTON_NOT_PRESSED      (HIDP_ERROR_CODES(0xC, 0xF))
#define HIDP_STATUS_REPORT_DOES_NOT_EXIST   (HIDP_ERROR_CODES(0xC, 0x10))
#define HIDP_STATUS_NOT_IMPLEMENTED         (HIDP_ERROR_CODES(0xC, 0x20))
#define HIDP_STATUS_I8242_TRANS_UNKNOWN     HIDP_STATUS_I8042_TRANS_UNKNOWN


#if !defined(_HIDPI_NO_FUNCTION_MACROS_)
/*
 * NTSTATUS
 * HidP_GetButtonCaps(
 *   IN HIDP_REPORT_TYPE  ReportType,
 *   OUT PHIDP_BUTTON_CAPS  ButtonCaps,
 *   IN OUT PULONG  ButtonCapsLength,
 *   IN PHIDP_PREPARSED_DATA  PreparsedData);
 */
#define HidP_GetButtonCaps(_Type_, _Caps_, _Len_, _Data_) \
  HidP_GetSpecificButtonCaps(_Type_, 0, 0, 0, _Caps_, _Len_, _Data_)

/*
 * NTSTATUS
 * HidP_GetButtons(
 *   IN HIDP_REPORT_TYPE  ReportType,
 *   IN USAGE  UsagePage,
 *   IN USHORT  LinkCollection,
 *   OUT USAGE  *UsageList,
 *   IN OUT ULONG  *UsageLength,
 *   IN PHIDP_PREPARSED_DATA  PreparsedData,
 *   IN PCHAR  Report,
 *   IN ULONG  ReportLength);
 */
#define HidP_GetButtons(Rty, UPa, LCo, ULi, ULe, Ppd, Rep, RLe) \
  HidP_GetUsages(Rty, UPa, LCo, ULi, ULe, Ppd, Rep, RLe)

#define HidP_GetButtonListLength(RTy, UPa, Ppd) \
  HidP_GetUsageListLength(Rty, UPa, Ppd)


/*
 * NTSTATUS
 * HidP_GetButtonsEx(
 *   IN HIDP_REPORT_TYPE  ReportType,
 *   IN USHORT  LinkCollection,
 *   OUT PUSAGE_AND_PAGE  ButtonList,
 *   IN OUT ULONG  *UsageLength,
 *   IN PHIDP_PREPARSED_DATA  PreparsedData,
 *   IN PCHAR  Report,
 *   IN ULONG  ReportLength);
 */
#define HidP_GetButtonsEx(RT, LC, BL, UL, PD, R, RL)  \
  HidP_GetUsagesEx(RT, LC, BL, UL, PD, R, RL)

#endif /* _HIDPI_NO_FUNCTION_MACROS_ */

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
HIDAPI
NTSTATUS
NTAPI
HidP_GetCaps(
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _Out_ PHIDP_CAPS Capabilities);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
HIDAPI
NTSTATUS
NTAPI
HidP_GetData(
  _In_ HIDP_REPORT_TYPE ReportType,
  _Out_writes_to_(*DataLength, *DataLength) PHIDP_DATA DataList,
  _Inout_ PULONG DataLength,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _Out_writes_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
HIDAPI
NTSTATUS
NTAPI
HidP_GetExtendedAttributes(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USHORT DataIndex,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _Out_writes_to_(*LengthAttributes, *LengthAttributes) PHIDP_EXTENDED_ATTRIBUTES Attributes,
  _Inout_ OUT PULONG LengthAttributes);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
HIDAPI
NTSTATUS
NTAPI
HidP_GetLinkCollectionNodes(
  _Out_ PHIDP_LINK_COLLECTION_NODE LinkCollectionNodes,
  _Inout_ PULONG LinkCollectionNodesLength,
  _In_ PHIDP_PREPARSED_DATA PreparsedData);

_Must_inspect_result_
HIDAPI
NTSTATUS
NTAPI
HidP_GetScaledUsageValue(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USAGE UsagePage,
  _In_ USHORT LinkCollection,
  _In_ USAGE Usage,
  _Out_ PLONG UsageValue,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _In_reads_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
HIDAPI
NTSTATUS
NTAPI
HidP_GetSpecificButtonCaps(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USAGE UsagePage,
  _In_ USHORT LinkCollection,
  _In_ USAGE Usage,
  _Out_ PHIDP_BUTTON_CAPS ButtonCaps,
  _Inout_ PUSHORT ButtonCapsLength,
  _In_ PHIDP_PREPARSED_DATA PreparsedData);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
HIDAPI
NTSTATUS
NTAPI
HidP_GetSpecificValueCaps(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USAGE UsagePage,
  _In_ USHORT LinkCollection,
  _In_ USAGE Usage,
  _Out_ PHIDP_VALUE_CAPS ValueCaps,
  _Inout_ PUSHORT ValueCapsLength,
  _In_ PHIDP_PREPARSED_DATA PreparsedData);

_Must_inspect_result_
HIDAPI
NTSTATUS
NTAPI
HidP_GetUsages(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USAGE UsagePage,
  _In_ USHORT LinkCollection,
  _Out_writes_to_(*UsageLength, *UsageLength) USAGE *UsageList,
  _Inout_ ULONG *UsageLength,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _Out_writes_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
HIDAPI
NTSTATUS
NTAPI
HidP_GetUsagesEx(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USHORT LinkCollection,
  _Inout_updates_to_(*UsageLength, *UsageLength) PUSAGE_AND_PAGE ButtonList,
  _Inout_ ULONG *UsageLength,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _In_reads_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

_Must_inspect_result_
HIDAPI
NTSTATUS
NTAPI
HidP_GetUsageValue(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USAGE UsagePage,
  _In_ USHORT LinkCollection,
  _In_ USAGE Usage,
  _Out_ PULONG UsageValue,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _In_reads_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

_Must_inspect_result_
HIDAPI
NTSTATUS
NTAPI
HidP_GetUsageValueArray(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USAGE UsagePage,
  _In_ USHORT LinkCollection,
  _In_ USAGE Usage,
  _Inout_updates_bytes_(UsageValueByteLength) PCHAR UsageValue,
  _In_ USHORT UsageValueByteLength,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _In_reads_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

#if !defined(_HIDPI_NO_FUNCTION_MACROS_)

/*
 * NTSTATUS
 * HidP_GetValueCaps(
 *   IN HIDP_REPORT_TYPE  ReportType,
 *   OUT PHIDP_VALUE_CAPS  ValueCaps,
 *   IN OUT PULONG  ValueCapsLength,
 *   IN PHIDP_PREPARSED_DATA  PreparsedData);
 */
#define HidP_GetValueCaps(_Type_, _Caps_, _Len_, _Data_) \
  HidP_GetSpecificValueCaps (_Type_, 0, 0, 0, _Caps_, _Len_, _Data_)

#endif /* _HIDPI_NO_FUNCTION_MACROS_ */

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
HIDAPI
NTSTATUS
NTAPI
HidP_InitializeReportForID(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ UCHAR ReportID,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _Out_writes_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

#if !defined(_HIDPI_NO_FUNCTION_MACROS_)

/*
 * BOOLEAN
 * HidP_IsSameUsageAndPage(
 *   USAGE_AND_PAGE  u1,
 *   USAGE_AND_PAGE  u2);
 */
#define HidP_IsSameUsageAndPage(u1, u2) ((* (PULONG) &u1) == (* (PULONG) &u2))

#endif /* _HIDPI_NO_FUNCTION_MACROS_ */

_IRQL_requires_max_(DISPATCH_LEVEL)
HIDAPI
ULONG
NTAPI
HidP_MaxDataListLength(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ PHIDP_PREPARSED_DATA PreparsedData);

_IRQL_requires_max_(PASSIVE_LEVEL)
HIDAPI
ULONG
NTAPI
HidP_MaxUsageListLength(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USAGE UsagePage,
  _In_ PHIDP_PREPARSED_DATA PreparsedData);

#if !defined(_HIDPI_NO_FUNCTION_MACROS_)

/*
 * NTSTATUS
 * HidP_SetButtons(
 *   IN HIDP_REPORT_TYPE  ReportType,
 *   IN USAGE  UsagePage,
 *   IN USHORT  LinkCollection,
 *   IN PUSAGE  UsageList,
 *   IN OUT PULONG  UsageLength,
 *   IN PHIDP_PREPARSED_DATA  PreparsedData,
 *   IN OUT PCHAR  Report,
 *   IN ULONG  ReportLength);
 */
#define HidP_SetButtons(RT, UP, LC, UL1, UL2, PD, R, RL) \
  HidP_SetUsages(RT, UP, LC, UL1, UL2, PD, R, RL)

#endif /* _HIDPI_NO_FUNCTION_MACROS_ */

_Must_inspect_result_
HIDAPI
NTSTATUS
NTAPI
HidP_SetData(
  _In_ HIDP_REPORT_TYPE ReportType,
  _Inout_updates_to_(*DataLength, *DataLength) PHIDP_DATA DataList,
  _Inout_ PULONG DataLength,
  _In_ PHIDP_PREPARSED_DATA  PreparsedData,
  _In_reads_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

_Must_inspect_result_
HIDAPI
NTSTATUS
NTAPI
HidP_SetScaledUsageValue(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USAGE UsagePage,
  _In_ USHORT LinkCollection,
  _In_ USAGE Usage,
  _In_ LONG UsageValue,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _Inout_updates_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

_Must_inspect_result_
HIDAPI
NTSTATUS
NTAPI
HidP_SetUsages(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USAGE UsagePage,
  _In_ USHORT LinkCollection, /* Optional */
  _Inout_updates_to_(*UsageLength, *UsageLength) PUSAGE UsageList,
  _Inout_ PULONG UsageLength,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _In_reads_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

_Must_inspect_result_
HIDAPI
NTSTATUS
NTAPI
HidP_SetUsageValue(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USAGE UsagePage,
  _In_ USHORT LinkCollection,
  _In_ USAGE Usage,
  _In_ ULONG UsageValue,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _Inout_updates_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

_Must_inspect_result_
HIDAPI
NTSTATUS
NTAPI
HidP_SetUsageValueArray(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USAGE UsagePage,
  _In_ USHORT LinkCollection,
  _In_ USAGE Usage,
  _In_reads_bytes_(UsageValueByteLength) PCHAR UsageValue,
  _In_ USHORT UsageValueByteLength,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _Inout_updates_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

#if !defined(_HIDPI_NO_FUNCTION_MACROS_)

/*
 * NTSTATUS
 * HidP_UnsetButtons(
 *   IN HIDP_REPORT_TYPE  ReportType,
 *   IN USAGE  UsagePage,
 *   IN USHORT  LinkCollection,
 *   IN PUSAGE  UsageList,
 *   IN OUT PULONG  UsageLength,
 *   IN PHIDP_PREPARSED_DATA  PreparsedData,
 *   IN OUT PCHAR  Report,
 *   IN ULONG  ReportLength);
 */
#define HidP_UnsetButtons(RT, UP, LC, UL1, UL2, PD, R, RL) \
  HidP_UnsetUsages(RT, UP, LC, UL1, UL2, PD, R, RL)

#endif /* _HIDPI_NO_FUNCTION_MACROS_ */

_Must_inspect_result_
HIDAPI
NTSTATUS
NTAPI
HidP_UnsetUsages(
  _In_ HIDP_REPORT_TYPE ReportType,
  _In_ USAGE UsagePage,
  _In_opt_ USHORT LinkCollection,
  _Inout_updates_to_(*UsageLength, *UsageLength) PUSAGE UsageList,
  _Inout_ PULONG UsageLength,
  _In_ PHIDP_PREPARSED_DATA PreparsedData,
  _In_reads_bytes_(ReportLength) PCHAR Report,
  _In_ ULONG ReportLength);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
HIDAPI
NTSTATUS
NTAPI
HidP_UsageAndPageListDifference(
  _In_reads_(UsageListLength) PUSAGE_AND_PAGE PreviousUsageList,
  _In_reads_(UsageListLength) PUSAGE_AND_PAGE CurrentUsageList,
  _Out_writes_(UsageListLength) PUSAGE_AND_PAGE BreakUsageList,
  _Out_writes_(UsageListLength) PUSAGE_AND_PAGE MakeUsageList,
  _In_ ULONG UsageListLength);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
HIDAPI
NTSTATUS
NTAPI
HidP_UsageListDifference(
  _In_reads_(UsageListLength) PUSAGE PreviousUsageList,
  _In_reads_(UsageListLength) PUSAGE CurrentUsageList,
  _Out_writes_(UsageListLength) PUSAGE BreakUsageList,
  _Out_writes_(UsageListLength) PUSAGE MakeUsageList,
  _In_ ULONG UsageListLength);

#ifdef __cplusplus
}
#endif

#endif /* __HIDPI_H__ */
