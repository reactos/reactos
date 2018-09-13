#ifndef _SHWIZARD_H_
#define _SHWIZARD_H_

#include <windows.h>
#include <windowsx.h>
#include <ccstock.h>
#include <prsht.h>
#include <crtfree.h>
#include "resource.h"

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

#define c_szHelpFile    TEXT("Shell.hlp")

// constants
#define NUM_PAGES       8
#define GUIDSIZE        50
#define UPDATE_BITMAP   0
#define UPDATE_WEB_VIEW 1
#define UPDATE_COMMENT  2
#define NO_TEMPLATE     0
#define SYSTEM_TEMPLATE 1
#define CUSTOM_TEMPLATE 2

#define REG_WEBVIEW_TEMPLATES       TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView\\Templates")
#define REG_VAL_DISPLAYNAME         TEXT("DisplayName")
#define REG_VAL_CUSTOMIZABLE        TEXT("Customizable")
#define REG_VAL_TEMPLATEFILE        TEXT("TemplateFile")
#define REG_VAL_PREVIEWBITMAPFILE   TEXT("PreviewBitmapFile")
#define REG_VAL_DESCRIPTION         TEXT("Description")
#define REG_VAL_VERSION             TEXT("Version")
#define REG_SUPPORTINGFILES         TEXT("Supporting Files")
#define REG_VAL_TEMPLATE_CHECKED    TEXT("TemplateChecked")
#define REG_VAL_LISTVIEW_CHECKED    TEXT("ListviewChecked")
#define REG_VAL_COMMENT_CHECKED     TEXT("CommentChecked")

#define REG_FC_WIZARD               TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Folder Customization Wizard")
#define REG_VAL_CUSTOMCOLORS        TEXT("Custom Colors")

#define REG_VAL_PERSISTEDBACKGROUNDFILENAME     TEXT("PersistedBackgroundFileName")

#ifdef UNICODE
#define SZ_CANBEUNICODE     TEXT("@")
#else
#define SZ_CANBEUNICODE     TEXT("")
#endif

#define PM_NONE         0
#define PM_LOCAL        1
#define PM_REMOTE       2

// Property Sheet control id's (determined with Spy++)
#define ID_APPLY_NOW    0x3021
#define ID_WIZBACK      0x3023
#define ID_WIZNEXT      0x3024
#define ID_WIZFINISH    0x3025

#define WM_SETIMAGE         (WM_USER + 300) // This should be sent by the client of the html
                                            // thumbnail control with lParam = (LPCTSTR)HtmlFilename
#define WM_INITSUBPROC      (WM_USER + 301) // This should be sent by the client of the html
                                            // thumbnail control to initialize the subproc
#define WM_UNINITSUBPROC    (WM_USER + 302) // This should be sent by the client of the html
                                            // thumbnail control to uninitialize the subproc

// globals
extern int g_fNextPage;        // page selection flag, used in StartPage.c
extern int g_iFlagA;

extern HINSTANCE g_hAppInst;

// The original ThumbnailWndProc, for subprocessing
extern WNDPROC g_lpThumbnailWndProc;

typedef struct
{
    int iId;    // For internal use. To keep track of the item returned from the template listview.
    TCHAR szKey[20];
    TCHAR szFriendlyName[100];
    TCHAR chCustomizable[2];
    TCHAR szTemplateFile[MAX_PATH];
    TCHAR szPreviewBitmapFile[MAX_PATH];
    TCHAR szDecsription[MAX_PATH];
} WebViewTemplateInfo;

typedef struct {
    int         iChanged;
    COLORREF    crColor;
} SHORTCUTCOLOR;

typedef struct
{
    UINT ui;
    BOOL bChoice;
} PATHCHOICE;

class CCTF_CommonInfo
{
public:
    HFONT GetTitleFont();
    BOOL WasThisOptionalPathUsed(UINT uiPath);
    BOOL WasThisFeatureUnCustomized(UINT uiPath);
    BOOL WasItCustomized();
    BOOL WasItUnCustomized();
    BOOL SetPathChoice(UINT uiPath, BOOL bChoice);
    BOOL SetUnCustomizedFeature(UINT uiFeature, BOOL bUnCustomized);
    void SetPath(UINT uiPath);
    
    UINT OnNext(HWND hwndDlg);
    UINT OnBack(HWND hwndDlg);
    void OnSetActive(HWND hwndDlg);
    void OnCancel(HWND hwndDlg);
    void OnFinishCustomization(HWND hwndDlg);
    void OnFinishUnCustomization(HWND hwndDlg);

    CCTF_CommonInfo();
    ~CCTF_CommonInfo();

private:
    UINT GetNextPrevPage_Helper(BOOL bNext);
    UINT GetNextPage_Helper(int iChoice);
    UINT GetPrevPage_Helper(int iChoice);
    UINT GetNextPage();
    UINT GetPrevPage();

    HFONT       _hTitleFont; // The title font for the Welcome and Completion pages
    UINT        _uiCurrentPage;
    PATHCHOICE  _uiPathChoices[3];
    PATHCHOICE  _uiFeatures[4];
    UINT        _uiPath;
    BOOL        _bCustomized;
    BOOL        _bUnCustomized;
};

extern CCTF_CommonInfo* g_pCommonInfo;

class CCTFWiz_Welcome
{
public:
    static INT_PTR APIENTRY WndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    void OnInit();

    CCTFWiz_Welcome(HWND hwndDlg, CCTF_CommonInfo* pCommonInfo) : _hwndDlg(hwndDlg), _pCommonInfo(pCommonInfo) {};

private:
    HWND _hwndDlg;
    CCTF_CommonInfo* _pCommonInfo;
};

class CCTFWiz_FinishCustomization
{
public:
    static INT_PTR APIENTRY WndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    void OnInit();
    void OnSetActive();

    CCTFWiz_FinishCustomization(HWND hwndDlg, CCTF_CommonInfo* pCommonInfo) : _hwndDlg(hwndDlg), _pCommonInfo(pCommonInfo) {};

private:
    HWND _hwndDlg;
    CCTF_CommonInfo* _pCommonInfo;
};

class CCTFWiz_FinishUnCustomization
{
public:
    static INT_PTR APIENTRY WndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    void OnInit();
    void OnSetActive();

    CCTFWiz_FinishUnCustomization(HWND hwndDlg, CCTF_CommonInfo* pCommonInfo) : _hwndDlg(hwndDlg), _pCommonInfo(pCommonInfo) {};

private:
    HWND _hwndDlg;
    CCTF_CommonInfo* _pCommonInfo;
};

class CCTF_ChoosePath
{
public:
    static INT_PTR APIENTRY WndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    void OnInit();

    void Describe(UINT uiTextId);
    
    CCTF_ChoosePath(HWND hwndDlg, CCTF_CommonInfo* pCommonInfo)
            : _hwndDlg(hwndDlg), _pCommonInfo(pCommonInfo) {};

private:
    HWND _hwndDlg;
    CCTF_CommonInfo* _pCommonInfo;
};

extern SHORTCUTCOLOR   ShortcutColorText;  // default to black, no change
extern SHORTCUTCOLOR   ShortcutColorBkgnd; // default to transparent, no change

extern BOOL g_bTemplateCopied;

extern TCHAR g_szCurFolder[];      // stores current directory
extern TCHAR g_szWinDir[];
extern TCHAR g_szFullHTMLFile[];   // full path to the HTML file...
extern TCHAR g_szIniFile[];        // the path to the ini file..
extern TCHAR g_szBmpFileName[];
extern TCHAR g_szTemplateFile[];
extern TCHAR g_szPreviewBitmapFile[];
extern TCHAR g_szKey[];
extern PROCESS_INFORMATION tpi;
extern COLORREF g_crCustomColors[];

void UpdateChangeBitmap(LPCTSTR pszImageFile);
void UpdateAddWebView ();
void GetTemporaryTemplatePath(LPTSTR pszTemplate);
void ChooseShortcutColor (HWND, SHORTCUTCOLOR *);
void UpdateShortcutColors (LPCTSTR);
void UpdateGlobalFolderInfo (int, LPCTSTR);
void ForceShellToRefresh (void);
BOOL CopyTemplate ();
BOOL TemplateExists (LPCTSTR);
void LaunchRegisteredEditor (void);
void LaunchNotepad (void);
void RestoreMasterTemplate (LPCTSTR);
LRESULT APIENTRY ThumbNailSubClassWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void DisplayBackground(LPCTSTR lpszImageFileName, HWND hwndSubclassed);
void DisplayPreview(LPCTSTR lpszImageFileName, HWND hwndSubclassed);
void DisplayNone(HWND hwndSubclassed);
void ShowPreview(LPCTSTR lpszFileName, HWND hwndSubclassed);
void Subclass(HWND hwnd);
void Unsubclass(HWND hwnd);
int HasPersistMoniker(LPTSTR pszFileName, int cchFileName, LPTSTR pszPreviewFileName, int cchPreviewFileName);
void InstallUnknownHTML(LPTSTR pszTempFileName, int cchTempFileName, BOOL bForce);
void DisplayUnknown(HWND hwndSubclassed);
BOOL ProdWebViewOn(HWND hwndOwner);
HRESULT UpdateComment(LPCTSTR pszHTMLComment);
HRESULT GetCurrentComment(LPTSTR pszHTMLComment, int cchHTMLComment);
BOOL IsBackgroundImageSet();
BOOL IsIconTextColorSet();
BOOL IsFolderCommentSet();
BOOL IsWebViewTemplateSet();
void RemoveBackgroundImage();
void RestoreIconTextColor();
void RemoveFolderComment();
void TCharStringFromGUID(const GUID& guid, TCHAR* pszGUID);
HRESULT RemoveWebViewTemplateSettings();
HRESULT GetWebViewTemplateKeyVersion(LPCTSTR pszKey, LPTSTR pszKeyVersion, int cchKeyVersion);

extern INT_PTR APIENTRY StartPageProc  (HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY PageA3Proc     (HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FinishAProc    (HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY PageT1Proc     (HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FinishTProc    (HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY RemoveProc     (HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FinishRProc    (HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY Comment_WndProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
