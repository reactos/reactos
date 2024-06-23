/*
 * Windows Error Reporting definitions
 *
 * Copyright (C) 2010 Louis Lenders
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WERAPI_H
#define __WINE_WERAPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Only 10 parameter are allowed in WerReportSetParameter */
#define WER_MAX_PARAM_COUNT 10
#define WER_P0 0
#define WER_P1 1
#define WER_P2 2
#define WER_P3 3
#define WER_P4 4
#define WER_P5 5
#define WER_P6 6
#define WER_P7 7
#define WER_P8 8
#define WER_P9 9

/* Flags for WerReportSubmit */
#define WER_SUBMIT_HONOR_RECOVERY           0x0001
#define WER_SUBMIT_HONOR_RESTART            0x0002
#define WER_SUBMIT_QUEUE                    0x0004
#define WER_SUBMIT_SHOW_DEBUG               0x0008
#define WER_SUBMIT_ADD_REGISTERED_DATA      0x0010
#define WER_SUBMIT_OUTOFPROCESS             0x0020
#define WER_SUBMIT_NO_CLOSE_UI              0x0040
#define WER_SUBMIT_NO_QUEUE                 0x0080
#define WER_SUBMIT_NO_ARCHIVE               0x0100
#define WER_SUBMIT_START_MINIMIZED          0x0200
#define WER_SUBMIT_OUTOFPROCESS_ASYNC       0x0400
#define WER_SUBMIT_BYPASS_DATA_THROTTLING   0x0800
#define WER_SUBMIT_ARCHIVE_PARAMETERS_ONLY  0x1000
#define WER_SUBMIT_REPORT_MACHINE_ID        0x2000

#define WER_MAX_PREFERRED_MODULES           128
#define WER_MAX_PREFERRED_MODULES_BUFFER    256

/* #### */

typedef HANDLE HREPORT;

typedef enum _WER_CONSENT
{
    WerConsentNotAsked = 1,
    WerConsentApproved,
    WerConsentDenied,
    WerConsentAlwaysPrompt,
    WerConsentMax
} WER_CONSENT;

typedef enum _WER_FILE_TYPE
{
    WerFileTypeMicrodump = 1,
    WerFileTypeMinidump,
    WerFileTypeHeapdump,
    WerFileTypeUserDocument,
    WerFileTypeOther,
    WerFileTypeMax
} WER_FILE_TYPE;

typedef enum _WER_REGISTER_FILE_TYPE
{
    WerRegFileTypeUserDocument = 1,
    WerRegFileTypeOther = 2,
    WerRegFileTypeMax
} WER_REGISTER_FILE_TYPE;

typedef struct _WER_REPORT_INFORMATION
{
    DWORD   dwSize;
    HANDLE  hProcess;
    WCHAR   wzConsentKey[64];
    WCHAR   wzFriendlyEventName[128];
    WCHAR   wzApplicationName[128];
    WCHAR   wzApplicationPath[MAX_PATH];
    WCHAR   wzDescription[512];
    HWND    hwndParent;
} WER_REPORT_INFORMATION, *PWER_REPORT_INFORMATION;


typedef enum _WER_REPORT_TYPE
{
    WerReportNonCritical = 0,
    WerReportCritical,
    WerReportApplicationCrash,
    WerReportApplicationHang,
    WerReportKernel,
    WerReportInvalid
} WER_REPORT_TYPE;

typedef enum _WER_SUBMIT_RESULT
{
    WerReportQueued = 1,
    WerReportUploaded,
    WerReportDebug,
    WerReportFailed,
    WerDisabled,
    WerReportCancelled,
    WerDisabledQueue,
    WerReportAsync,
    WerCustomAction
} WER_SUBMIT_RESULT, *PWER_SUBMIT_RESULT;

typedef enum _WER_DUMP_TYPE
{
    WerDumpTypeMicroDump = 1,
    WerDumpTypeMiniDump,
    WerDumpTypeHeapDump,
    WerDumpTypeMax
} WER_DUMP_TYPE;

typedef enum _WER_REPORT_UI
{
    WerUIAdditionalDataDlgHeader = 1,
    WerUIIconFilePath = 2,
    WerUIConsentDlgHeader = 3,
    WerUIConsentDlgBody = 4,
    WerUIOnlineSolutionCheckText = 5,
    WerUIOfflineSolutionCheckText = 6,
    WerUICloseText = 7,
    WerUICloseDlgHeader = 8,
    WerUICloseDlgBody = 9,
    WerUICloseDlgButtonText = 10,
    WerUICustomActionButtonText = 11,
    WerUIMax
} WER_REPORT_UI;

/* #### */

typedef struct _WER_DUMP_CUSTOM_OPTIONS
{
    DWORD dwSize;
    DWORD dwMask;
    DWORD dwDumpFlags;
    BOOL  bOnlyThisThread;
    DWORD dwExceptionThreadFlags;
    DWORD dwOtherThreadFlags;
    DWORD dwExceptionThreadExFlags;
    DWORD dwOtherThreadExFlags;
    DWORD dwPreferredModuleFlags;
    DWORD dwOtherModuleFlags;
    WCHAR wzPreferredModuleList[WER_MAX_PREFERRED_MODULES_BUFFER];

} WER_DUMP_CUSTOM_OPTIONS, *PWER_DUMP_CUSTOM_OPTIONS;

typedef struct _WER_EXCEPTION_INFORMATION
{
    PEXCEPTION_POINTERS pExceptionPointers;
    BOOL bClientPointers;
} WER_EXCEPTION_INFORMATION, *PWER_EXCEPTION_INFORMATION;

/* #### */

HRESULT WINAPI WerAddExcludedApplication(PCWSTR, BOOL);
HRESULT WINAPI WerGetFlags(HANDLE process, DWORD *flags);
HRESULT WINAPI WerRegisterFile(PCWSTR file, WER_REGISTER_FILE_TYPE regfiletype, DWORD flags);
HRESULT WINAPI WerRegisterMemoryBlock(void *block, DWORD size);
HRESULT WINAPI WerRegisterRuntimeExceptionModule(PCWSTR callbackdll, void *context);
HRESULT WINAPI WerRemoveExcludedApplication(PCWSTR, BOOL);
HRESULT WINAPI WerReportAddFile(HREPORT, PCWSTR, WER_FILE_TYPE, DWORD);
HRESULT WINAPI WerReportCloseHandle(HREPORT);
HRESULT WINAPI WerReportCreate(PCWSTR, WER_REPORT_TYPE, PWER_REPORT_INFORMATION, HREPORT*);
HRESULT WINAPI WerReportSetParameter(HREPORT, DWORD, PCWSTR, PCWSTR);
HRESULT WINAPI WerReportSetUIOption(HREPORT, WER_REPORT_UI, PCWSTR);
HRESULT WINAPI WerReportSubmit(HREPORT, WER_CONSENT, DWORD, PWER_SUBMIT_RESULT);
HRESULT WINAPI WerSetFlags(DWORD flags);
HRESULT WINAPI WerUnregisterFile(PCWSTR file);
HRESULT WINAPI WerUnregisterMemoryBlock(void *block);
HRESULT WINAPI WerUnregisterRuntimeExceptionModule(PCWSTR callbackdll, void *context);


#ifdef __cplusplus
}
#endif

#endif /* __WINE_WERAPI_H */
