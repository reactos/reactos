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

#ifndef __HIDPI_H
#define __HIDPI_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"
#include "hidusage.h"

#if defined(_HIDPI_)
  #define HIDAPI DECLSPEC_EXPORT
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

typedef BOOLEAN (DDKAPI *PHIDP_INSERT_SCANCODES)(
  IN PVOID  Context,
  IN PCHAR  NewScanCodes,
  IN ULONG  Length);

typedef struct _USAGE_AND_PAGE {
  USAGE  Usage;
  USAGE  UsagePage;
} USAGE_AND_PAGE, *PUSAGE_AND_PAGE;

HIDAPI
NTSTATUS
DDKAPI
HidP_TranslateUsageAndPagesToI8042ScanCodes(
  IN PUSAGE_AND_PAGE  ChangedUsageList,
  IN ULONG  UsageListLength,
  IN HIDP_KEYBOARD_DIRECTION  KeyAction,
  IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
  IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
  IN PVOID  InsertCodesContext);

HIDAPI
NTSTATUS
DDKAPI
HidP_TranslateUsagesToI8042ScanCodes(
  IN PUSAGE  ChangedUsageList,
  IN ULONG  UsageListLength,
  IN HIDP_KEYBOARD_DIRECTION  KeyAction,
  IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
  IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
  IN PVOID  InsertCodesContext);

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

HIDAPI
NTSTATUS
DDKAPI
HidP_GetCaps(
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  OUT PHIDP_CAPS  Capabilities);

HIDAPI
NTSTATUS
DDKAPI
HidP_GetData(
  IN HIDP_REPORT_TYPE  ReportType,
  OUT PHIDP_DATA  DataList,
  IN OUT PULONG  DataLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN PCHAR  Report,
  IN ULONG  ReportLength);

HIDAPI
NTSTATUS
DDKAPI
HidP_GetExtendedAttributes(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USHORT  DataIndex,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  OUT PHIDP_EXTENDED_ATTRIBUTES  Attributes,
  IN OUT PULONG  LengthAttributes);

HIDAPI
NTSTATUS
DDKAPI
HidP_GetLinkCollectionNodes(
  OUT PHIDP_LINK_COLLECTION_NODE  LinkCollectionNodes,
  IN OUT PULONG  LinkCollectionNodesLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData);

HIDAPI
NTSTATUS
DDKAPI
HidP_GetScaledUsageValue(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage,
  IN USHORT  LinkCollection  OPTIONAL,
  IN USAGE  Usage,
  OUT PLONG  UsageValue,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN PCHAR  Report,
  IN ULONG  ReportLength);

HIDAPI
NTSTATUS
DDKAPI
HidP_GetSpecificButtonCaps(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage,
  IN USHORT  LinkCollection,
  IN USAGE  Usage,
  OUT PHIDP_BUTTON_CAPS  ButtonCaps,
  IN OUT PULONG  ButtonCapsLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData);

HIDAPI
NTSTATUS
DDKAPI
HidP_GetSpecificValueCaps(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage,
  IN USHORT  LinkCollection,
  IN USAGE  Usage,
  OUT PHIDP_VALUE_CAPS  ValueCaps,
  IN OUT PULONG  ValueCapsLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData);

HIDAPI
NTSTATUS
DDKAPI
HidP_GetUsages(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage,
  IN USHORT  LinkCollection  OPTIONAL,
  OUT USAGE  *UsageList,
  IN OUT ULONG  *UsageLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN PCHAR  Report,
  IN ULONG  ReportLength);

HIDAPI
NTSTATUS
DDKAPI
HidP_GetUsagesEx(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USHORT  LinkCollection,
  OUT PUSAGE_AND_PAGE  ButtonList,
  IN OUT ULONG  *UsageLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN PCHAR  Report,
  IN ULONG  ReportLength);

HIDAPI
NTSTATUS
DDKAPI
HidP_GetUsageValue(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage,
  IN USHORT  LinkCollection,
  IN USAGE  Usage,
  OUT PULONG  UsageValue,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN PCHAR  Report,
  IN ULONG  ReportLength);

HIDAPI
NTSTATUS
DDKAPI
HidP_GetUsageValueArray(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage,
  IN USHORT  LinkCollection  OPTIONAL,
  IN USAGE  Usage,
  OUT PCHAR  UsageValue,
  IN USHORT  UsageValueByteLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN PCHAR  Report,
  IN ULONG  ReportLength);

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

HIDAPI
NTSTATUS
DDKAPI
HidP_InitializeReportForID(
  IN HIDP_REPORT_TYPE  ReportType,
  IN UCHAR  ReportID,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN OUT PCHAR  Report,
  IN ULONG  ReportLength);

/*
 * BOOLEAN
 * HidP_IsSameUsageAndPage(
 *   USAGE_AND_PAGE  u1,
 *   USAGE_AND_PAGE  u2);
 */
#define HidP_IsSameUsageAndPage(u1, u2) ((* (PULONG) &u1) == (* (PULONG) &u2))

HIDAPI
ULONG
DDKAPI
HidP_MaxDataListLength(
  IN HIDP_REPORT_TYPE  ReportType,
  IN PHIDP_PREPARSED_DATA  PreparsedData);

HIDAPI
ULONG
DDKAPI
HidP_MaxUsageListLength(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage  OPTIONAL,
  IN PHIDP_PREPARSED_DATA  PreparsedData);

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

HIDAPI
NTSTATUS
DDKAPI
HidP_SetData(
  IN HIDP_REPORT_TYPE  ReportType,
  IN PHIDP_DATA  DataList,
  IN OUT PULONG  DataLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN OUT PCHAR  Report,
  IN ULONG  ReportLength);

HIDAPI
NTSTATUS
DDKAPI
HidP_SetScaledUsageValue(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage, 
  IN USHORT  LinkCollection  OPTIONAL,
  IN USAGE  Usage,
  IN LONG  UsageValue,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN OUT PCHAR  Report,
  IN ULONG  ReportLength);

HIDAPI
NTSTATUS
DDKAPI
HidP_SetUsages(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage,
  IN USHORT  LinkCollection, /* Optional */
  IN PUSAGE  UsageList,
  IN OUT PULONG  UsageLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN OUT PCHAR  Report,
  IN ULONG  ReportLength);

HIDAPI
NTSTATUS
DDKAPI
HidP_SetUsageValue(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage,
  IN USHORT  LinkCollection,
  IN USAGE  Usage,
  IN ULONG  UsageValue,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN OUT PCHAR  Report,
  IN ULONG  ReportLength);

HIDAPI
NTSTATUS
DDKAPI
HidP_SetUsageValueArray(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage,
  IN USHORT  LinkCollection  OPTIONAL,
  IN USAGE  Usage,
  IN PCHAR  UsageValue,
  IN USHORT  UsageValueByteLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  OUT PCHAR  Report,
  IN ULONG  ReportLength);

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

HIDAPI
NTSTATUS
DDKAPI
HidP_UnsetUsages(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage,
  IN USHORT  LinkCollection,
  IN PUSAGE  UsageList,
  IN OUT PULONG  UsageLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN OUT PCHAR  Report,
  IN ULONG  ReportLength);

HIDAPI
NTSTATUS
DDKAPI
HidP_UsageAndPageListDifference(
  IN PUSAGE_AND_PAGE  PreviousUsageList,
  IN PUSAGE_AND_PAGE  CurrentUsageList,
  OUT PUSAGE_AND_PAGE  BreakUsageList,
  OUT PUSAGE_AND_PAGE  MakeUsageList,
  IN ULONG  UsageListLength);

HIDAPI
NTSTATUS
DDKAPI
HidP_UsageListDifference(
  IN PUSAGE  PreviousUsageList,
  IN PUSAGE  CurrentUsageList,
  OUT PUSAGE  BreakUsageList,
  OUT PUSAGE  MakeUsageList,
  IN ULONG  UsageListLength);

#ifdef __cplusplus
}
#endif

#endif /* __HIDPI_H */
