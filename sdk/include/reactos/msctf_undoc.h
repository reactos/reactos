/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Private header for msctf.dll
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <msctf.h>
#include <inputscope.h>

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(IID_ITfRangeAnchor, 0x8B99712B, 0x5815, 0x4BCC, 0xB9, 0xA9, 0x53, 0xDB, 0x1C, 0x8D, 0x67, 0x55);

#define INTERFACE ITfRangeAnchor
DECLARE_INTERFACE_(ITfRangeAnchor, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(GetFormattedText)(
        _In_ TfEditCookie ec,
        _Out_ IDataObject **ppDataObject) PURE;
    STDMETHOD(GetEmbedded)(
        _In_ TfEditCookie ec,
        _In_ REFGUID rguidService,
        _In_ REFIID riid,
        _Out_ IUnknown **ppunk) PURE;
    STDMETHOD(InsertEmbedded)(
        _In_ TfEditCookie ec,
        _In_ DWORD dwFlags,
        _In_ IDataObject *pDataObject) PURE;
    STDMETHOD(ShiftStart)(
        _In_ TfEditCookie ec,
        _In_ LONG cchReq,
        _Out_ LONG *pcch,
        _In_ const TF_HALTCOND *pHalt) PURE;
    STDMETHOD(ShiftEnd)(
        _In_ TfEditCookie ec,
        _In_ LONG cchReq,
        _Out_ LONG *pcch,
        _In_ const TF_HALTCOND *pHalt) PURE;
    STDMETHOD(ShiftStartToRange)(
        _In_ TfEditCookie ec,
        _Inout_ ITfRange *pRange,
        _In_ TfAnchor aPos) PURE;
    STDMETHOD(ShiftEndToRange)(
        _In_ TfEditCookie ec,
        _Inout_ ITfRange *pRange,
        _In_ TfAnchor aPos) PURE;
    STDMETHOD(ShiftStartRegion)(
        _In_ TfEditCookie ec,
        _In_ TfShiftDir dir,
        _Out_ BOOL *pfNoRegion) PURE;
    STDMETHOD(ShiftEndRegion)(
        _In_ TfEditCookie ec,
        _In_ TfShiftDir dir,
        _Out_ BOOL *pfNoRegion) PURE;
    STDMETHOD(IsEmpty)(
        _In_ TfEditCookie ec,
        _Out_ BOOL *pfEmpty) PURE;
    STDMETHOD(Collapse)(
        _In_ TfEditCookie ec,
        _In_ TfAnchor aPos) PURE;
    STDMETHOD(IsEqualStart)(
        _In_ TfEditCookie ec,
        _In_ ITfRange *pWith,
        _In_ TfAnchor aPos,
        _Out_ BOOL *pfEqual) PURE;
    STDMETHOD(IsEqualEnd)(
        _In_ TfEditCookie ec,
        _In_ ITfRange *pWith,
        _In_ TfAnchor aPos,
        _Out_ BOOL *pfEqual) PURE;
    STDMETHOD(CompareStart)(
        _In_ TfEditCookie ec,
        _Inout_ ITfRange *pWith,
        _In_ TfAnchor aPos,
        _Out_ LONG *plResult) PURE;
    STDMETHOD(CompareEnd)(
        _In_ TfEditCookie ec,
        _In_ ITfRange *pWith,
        _In_ TfAnchor aPos,
        _Out_ LONG *plResult) PURE;
    STDMETHOD(AdjustForInsert)(
        _In_ TfEditCookie ec,
        _In_ ULONG cchInsert,
        _Out_ BOOL *pfInsertOk) PURE;
    STDMETHOD(GetGravity)(
        _Out_ TfGravity *pgStart,
        _Out_ TfGravity *pgEnd) PURE;
    STDMETHOD(SetGravity)(
        _In_ TfEditCookie ec,
        _In_ TfGravity gStart,
        _In_ TfGravity gEnd) PURE;
    STDMETHOD(Clone)(_Out_ ITfRange **ppClone) PURE;
    STDMETHOD(GetContext)(_Out_ ITfContext **ppContext) PURE;
};
#undef INTERFACE

BOOL WINAPI TF_InitSystem(VOID);
BOOL WINAPI TF_UninitSystem(VOID);
HRESULT WINAPI TF_GetGlobalCompartment(_Out_ ITfCompartmentMgr **ppCompMgr);
HRESULT WINAPI TF_PostAllThreadMsg(_In_opt_ WPARAM wParam, _In_ DWORD dwFlags);
HANDLE WINAPI TF_CreateCicLoadMutex(_Out_ LPBOOL pfWinLogon);
HRESULT WINAPI TF_InvalidAssemblyListCache(VOID);
HRESULT WINAPI TF_InvalidAssemblyListCacheIfExist(VOID);
HICON WINAPI TF_GetLangIcon(_In_ LANGID LangID, _Out_writes_(cchText) PWSTR pszText, _In_ INT cchText);
VOID WINAPI TF_InitMlngInfo(VOID);
INT WINAPI TF_MlngInfoCount(VOID);
INT WINAPI TF_GetMlngIconIndex(_In_ INT iKL);
HICON WINAPI TF_InatExtractIcon(_In_ INT iKL);
HRESULT WINAPI TF_RunInputCPL(VOID);
LONG WINAPI TF_CheckThreadInputIdle(_In_ DWORD dwThreadId, _In_ DWORD dwMilliseconds);
BOOL WINAPI TF_IsInMarshaling(_In_ DWORD dwThreadId);

// This is intentionally misspelled to match the original name:
BOOL WINAPI TF_IsFullScreenWindowAcitvated(VOID);

HRESULT WINAPI TF_CUASAppFix(_In_ LPCSTR pszName);
HRESULT WINAPI TF_ClearLangBarAddIns(_In_ REFGUID rguid);
HRESULT WINAPI TF_GetInputScope(_In_opt_ HWND hWnd, _Out_ ITfInputScope **ppInputScope);
BOOL WINAPI TF_DllDetachInOther(VOID);

BOOL WINAPI
TF_GetMlngHKL(
    _In_ INT iKL,
    _Out_opt_ HKL *phKL,
    _Out_writes_opt_(cchText) LPWSTR pszText,
    _In_ INT cchText);

BOOL WINAPI
TF_GetThreadFlags(
    _In_ DWORD dwThreadId,
    _Out_ LPDWORD pdwFlags1,
    _Out_ LPDWORD pdwFlags2,
    _Out_ LPDWORD pdwFlags3);

#ifdef __cplusplus
} // extern "C"
#endif
