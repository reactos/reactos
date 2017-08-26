#pragma once
#include <windef.h>
#include <atlstr.h>

class CAvailableApps;

HWND CreateMainWindow();
DWORD_PTR ListViewGetlParam(INT item);
INT ListViewAddItem(INT ItemIndex, INT IconIndex, LPWSTR lpName, LPARAM lParam);
VOID SetStatusBarText(LPCWSTR szText);
VOID NewRichEditText(LPCWSTR szText, DWORD flags);
VOID InsertRichEditText(LPCWSTR szText, DWORD flags);

VOID SetStatusBarText(const ATL::CStringW& szText);
INT ListViewAddItem(INT ItemIndex, INT IconIndex, const ATL::CStringW& Name, LPARAM lParam);
VOID NewRichEditText(const ATL::CStringW& szText, DWORD flags);
VOID InsertRichEditText(const ATL::CStringW& szText, DWORD flags);
CAvailableApps * GetAvailableApps();
extern HWND hListView;
extern ATL::CStringW szSearchPattern;
