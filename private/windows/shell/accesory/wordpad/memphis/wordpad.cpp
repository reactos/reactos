// wordpad.cpp : Defines the class behaviors for the application.
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"

#include "wordpad.h"
#include "mainfrm.h"
#include "ipframe.h"
#include "wordpdoc.h"
#include "wordpvw.h"
#include "strings.h"
#include "key.h"
#include "filenewd.h"
#include <locale.h>
#include <winnls.h>
#include <winreg.h>
#include "fixhelp.h"
#if _WIN32_IE < 0x400
#undef _WIN32_IE
#define _WIN32_IE   0x0400
#endif
#include <shlobj.h>

#define szRichName "RICHED20"

extern BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
static BOOL RegisterHelper(LPCTSTR* rglpszRegister, LPCTSTR* rglpszSymbols,
   BOOL bReplace);

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CLIPFORMAT cfEmbeddedObject;
CLIPFORMAT cfRTF;
CLIPFORMAT cfRTO;

int CWordPadApp::m_nOpenMsg = RegisterWindowMessage(_T("WordPadOpenMessage"));
int CWordPadApp::m_nPrinterChangedMsg = RegisterWindowMessage(_T("WordPadPrinterChanged"));
int CWordPadApp::m_nOLEHelpMsg = RegisterWindowMessage(SZOLEUI_MSG_HELP);

CUnit CWordPadApp::m_units[] =
{
//    TPU,  SmallDiv,MedDiv,LargeDiv,MinMove,szAbbrev,         bSpace
CUnit(1440, 180,     720,   1440,    90,     IDS_INCH1_ABBREV, FALSE),//"
CUnit(568,  142,     284,   568,     142,    IDS_CM_ABBREV,    TRUE),//cm's
CUnit(20,   120,     720,   720,     100,    IDS_POINT_ABBREV, TRUE),//points
CUnit(240,  240,     1440,  1440,    120,    IDS_PICA_ABBREV,  TRUE),//picas
CUnit(1440, 180,     720,   1440,    90,     IDS_INCH2_ABBREV, FALSE),//in
CUnit(1440, 180,     720,   1440,    90,     IDS_INCH3_ABBREV, FALSE),//inch
CUnit(1440, 180,     720,   1440,    90,     IDS_INCH4_ABBREV, FALSE),//inches

// Non-localized units

CUnit(1440, 180,     720,   1440,    90,     IDS_INCH1_NOLOC,  FALSE),//"
CUnit(1440, 180,     720,   1440,    90,     IDS_INCH2_NOLOC,  FALSE),//in
CUnit(1440, 180,     720,   1440,    90,     IDS_INCH3_NOLOC,  FALSE),//inch
CUnit(1440, 180,     720,   1440,    90,     IDS_INCH4_NOLOC,  FALSE),//inches
CUnit(568,  142,     284,   568,     142,    IDS_CM_NOLOC,     TRUE),//cm's
CUnit(20,   120,     720,   720,     100,    IDS_POINT_NOLOC,  TRUE),//points
CUnit(240,  240,     1440,  1440,    120,    IDS_PICA_NOLOC,   TRUE)//picas
};

const int CWordPadApp::m_nPrimaryNumUnits = 4;
const int CWordPadApp::m_nNumUnits = sizeof(m_units) / sizeof(m_units[0]);


/////////////////////////////////////////////////////////////////////////////
// CWordPadApp

BEGIN_MESSAGE_MAP(CWordPadApp, CWinApp)
   //{{AFX_MSG_MAP(CWordPadApp)
   ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
   ON_COMMAND(ID_FILE_NEW, OnFileNew)
   ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CWordPadCommandLineInfo::ParseParam(const char* pszParam,BOOL bFlag,BOOL bLast)
{
   if (bFlag)
   {
      if (lstrcmpA(pszParam, "t") == 0)
      {
         m_bForceTextMode = TRUE;
         return;
      }
   }
   CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
}

/////////////////////////////////////////////////////////////////////////////
// CWordPadApp construction

CWordPadApp::CWordPadApp() 
      : m_optionsText(0), 
        m_optionsRTF(1),
        m_optionsWord(2), 
        m_optionsWrite(2), 
        m_optionsIP(2), 
        m_optionsNull(0),
        m_initialization_phase(InitializationPending),
        m_pInitializationThread(NULL)
{
// _tsetlocale(LC_ALL, _T(""));
   setlocale(LC_ALL, "");     // change made by t-stefb

   m_nFilterIndex = 1;
   DWORD dwVersion = ::GetVersion();
   m_bWin4 = (BYTE)dwVersion >= 4;
#ifndef _UNICODE
   m_bWin31 = (dwVersion > 0x80000000 && !m_bWin4);
#endif
   m_nDefFont = (m_bWin4) ? DEFAULT_GUI_FONT : ANSI_VAR_FONT;
   m_dcScreen.Attach(::GetDC(NULL));
   m_bLargeIcons = m_dcScreen.GetDeviceCaps(LOGPIXELSX) >= 120;
   m_bForceOEM = FALSE;
}

CWordPadApp::~CWordPadApp()
{
   if (m_dcScreen.m_hDC != NULL)
      ::ReleaseDC(NULL, m_dcScreen.Detach());

    delete m_pInitializationThread;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWordPadApp object

CWordPadApp theApp;

// Register the application's document templates.  Document templates
//  serve as the connection between documents, frame windows and views.
static CSingleDocTemplate DocTemplate(
      IDR_MAINFRAME,
      RUNTIME_CLASS(CWordPadDoc),
      RUNTIME_CLASS(CMainFrame),       // main SDI frame window
      RUNTIME_CLASS(CWordPadView));

// This identifier was generated to be statistically unique for your app.
// You may change it if you prefer to choose a specific identifier.
static const CLSID BASED_CODE clsid =
{ 0x73FDDC80L, 0xAEA9, 0x101A, { 0x98, 0xA7, 0x00, 0xAA, 0x00, 0x37, 0x49, 0x59} };

/////////////////////////////////////////////////////////////////////////////
// CWordPadApp initialization


BOOL CWordPadApp::InitInstance()
{
   ParseCommandLine(cmdInfo);

   if (::FindWindow(szWordPadClass, NULL) && IsDocOpen(cmdInfo.m_strFileName))
      return FALSE;

   SetRegistryKey(szRegKey);

   // The resistry stuff shouldn't be localized so just hardcode it

   if (NULL != m_pszProfileName)
        free((void *) m_pszProfileName);

   m_pszProfileName = _tcsdup(TEXT("Wordpad"));

   LoadOptions();

   Enable3dControls();
   if (!cmdInfo.m_bRunEmbedded)
   {
      switch (m_nCmdShow)
      {
         case SW_RESTORE:
         case SW_SHOW:
         case SW_SHOWDEFAULT:
         case SW_SHOWNA:
         case SW_SHOWNOACTIVATE:
         case SW_SHOWNORMAL:
         case SW_SHOWMAXIMIZED:
            if (m_bMaximized)
               m_nCmdShow = SW_SHOWMAXIMIZED;
            break;
      }
   }
   else
   {
      //Excel 4 will start OLE servers minimized
      m_nCmdShow = SW_SHOWNORMAL;
   }
   int nCmdShow = m_nCmdShow;

   LoadAbbrevStrings();

   m_pszHelpFilePath = _tcsdup(WORDPAD_HELP_FILE) ;

   // Initialize OLE libraries
   if (!AfxOleInit())
   {
      AfxMessageBox(IDP_OLE_INIT_FAILED);
      return FALSE;
   }
   RegisterFormats();

   // Initialize RichEdit control
   if (LoadLibrary(_T(szRichName)) == NULL)
   {
      AfxMessageBox(IDS_RICHED_LOAD_FAIL, MB_OK|MB_ICONEXCLAMATION);
      return FALSE;
   }

   // Standard initialization
   // If you are not using these features and wish to reduce the size
   //  of your final executable, you should remove from the following
   //  the specific initialization routines you do not need.

   LoadStdProfileSettings();  // Load standard INI file options (including MRU)

   // Register the application's document templates.  Document templates
   //  serve as the connection between documents, frame windows and views.

   DocTemplate.SetContainerInfo(IDR_CNTR_INPLACE);
   DocTemplate.SetServerInfo(
      IDR_SRVR_EMBEDDED, IDR_SRVR_INPLACE,
      RUNTIME_CLASS(CInPlaceFrame));

   // Connect the COleTemplateServer to the document template.
   //  The COleTemplateServer creates new documents on behalf
   //  of requesting OLE containers by using information
   //  specified in the document template.
   m_server.ConnectTemplate(clsid, &DocTemplate, TRUE);


    //
    // Setup deferred initialization now so the printer can start
    // initializing in case we get a print or printto command at startup
    //

    m_pInitializationThread = AfxBeginThread(
                                    DoDeferredInitialization,
                                    this,
                                    THREAD_PRIORITY_IDLE,
                                    0,
                                    CREATE_SUSPENDED);

    if (NULL != m_pInitializationThread)
    {
        m_pInitializationThread->m_bAutoDelete = FALSE;
        m_pInitializationThread->ResumeThread();
    }

   // Check to see if launched as OLE server
   if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
   {
      // Register all OLE server (factories) as running.  This enables the
      //  OLE libraries to create objects from other applications.
      COleTemplateServer::RegisterAll();
      AfxOleSetUserCtrl(FALSE);

      // Application was run with /Embedding or /Automation.  Don't show the
      //  main window in this case.
      return TRUE;
   }

   // make sure the main window is showing
   m_bPromptForType = FALSE;
   OnFileNew();
   m_bPromptForType = TRUE;
   m_nCmdShow = -1;
   if (m_pMainWnd == NULL) // i.e. OnFileNew failed
      return FALSE;

   if (!cmdInfo.m_strFileName.IsEmpty())  // open an existing document
      m_nCmdShow = nCmdShow;
   // Dispatch commands specified on the command line
   if (cmdInfo.m_nShellCommand != CCommandLineInfo::FileNew &&
      !ProcessShellCommand(cmdInfo))
   {
      return FALSE;
   }

   // Enable File Manager drag/drop open
   m_pMainWnd->DragAcceptFiles();

    //
    // Set the current directory to "My Documents" so that will be the default
    // location for the first save/open
    //

    TCHAR szDefaultPath[MAX_PATH];

    if (SHGetSpecialFolderPath(NULL, szDefaultPath, CSIDL_PERSONAL, FALSE))
        SetCurrentDirectory(szDefaultPath);

    return TRUE;
}

BOOL CWordPadApp::IsDocOpen(LPCTSTR lpszFileName)
{
   if (lpszFileName[0] == NULL)
      return FALSE;
   TCHAR szPath[_MAX_PATH];
   AfxFullPath(szPath, lpszFileName);
   ATOM atom = GlobalAddAtom(szPath);
   ASSERT(atom != NULL);
   if (atom == NULL)
      return FALSE;
   EnumWindows(StaticEnumProc, (LPARAM)&atom);
   if (atom == NULL)
      return TRUE;
   DeleteAtom(atom);
   return FALSE;
}

BOOL CALLBACK CWordPadApp::StaticEnumProc(HWND hWnd, LPARAM lParam)
{
   TCHAR szClassName[30];
   GetClassName(hWnd, szClassName, 30);
   if (lstrcmp(szClassName, szWordPadClass) != 0)
      return TRUE;

   ATOM* pAtom = (ATOM*)lParam;
   ASSERT(pAtom != NULL);
   DWORD dw = NULL;
   ::SendMessageTimeout(hWnd, m_nOpenMsg, NULL, (LPARAM)*pAtom,
      SMTO_ABORTIFHUNG, 500, &dw);
   if (dw)
   {
      ::SetForegroundWindow(hWnd);
      DeleteAtom(*pAtom);
      *pAtom = NULL;
      return FALSE;
   }           
   return TRUE;
}

void CWordPadApp::RegisterFormats()
{
   cfEmbeddedObject = (CLIPFORMAT)::RegisterClipboardFormat(_T("Embedded Object"));
   cfRTF = (CLIPFORMAT)::RegisterClipboardFormat(CF_RTF);
   cfRTO = (CLIPFORMAT)::RegisterClipboardFormat(CF_RETEXTOBJ);
}

CDocOptions& CWordPadApp::GetDocOptions(int nDocType)
{
   switch (nDocType)
   {
      case RD_WINWORD6:
      case RD_WORDPAD:
      case RD_WORD97:
         return m_optionsWord;
      case RD_RICHTEXT:
         return m_optionsRTF;
      case RD_TEXT:
      case RD_OEMTEXT:
      case RD_UNICODETEXT:
         return m_optionsText;
      case RD_WRITE:
         return m_optionsWrite;
      case RD_EMBEDDED:
         return m_optionsIP;
   }
   ASSERT(FALSE);
   return m_optionsNull;
}

CDockState& CWordPadApp::GetDockState(int nDocType, BOOL bPrimary)
{
   return GetDocOptions(nDocType).GetDockState(bPrimary);
}

void CWordPadApp::SaveOptions()
{
   WriteProfileInt(szSection, szWordSel, m_bWordSel);
   WriteProfileInt(szSection, szUnits, GetUnits());
   WriteProfileInt(szSection, szMaximized, m_bMaximized);
   WriteProfileBinary(szSection, szFrameRect, (BYTE*)&m_rectInitialFrame,
      sizeof(CRect));
   WriteProfileBinary(szSection, szPageMargin, (BYTE*)&m_rectPageMargin,
      sizeof(CRect));
   m_optionsText.SaveOptions(szTextSection);
   m_optionsRTF.SaveOptions(szRTFSection);
   m_optionsWord.SaveOptions(szWordSection);
   m_optionsWrite.SaveOptions(szWriteSection);
   m_optionsIP.SaveOptions(szIPSection);
}

void CWordPadApp::LoadOptions()
{
   BYTE* pb = NULL;
   UINT nLen = 0;

   HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
   if (hFont == NULL)
      hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
   VERIFY(GetObject(hFont, sizeof(LOGFONT), &m_lf));

   m_bWordSel = GetProfileInt(szSection, szWordSel, TRUE);
   TCHAR buf[2];
   buf[0] = NULL;
   GetLocaleInfo(GetUserDefaultLCID(), LOCALE_IMEASURE, buf, 2);
   int nDefUnits = buf[0] == _T('1') ? 0 : 1;
   SetUnits(GetProfileInt(szSection, szUnits, nDefUnits));
   m_bMaximized = GetProfileInt(szSection, szMaximized, (int)FALSE);

   if (GetProfileBinary(szSection, szFrameRect, &pb, &nLen))
   {
      ASSERT(nLen == sizeof(CRect));
      memcpy(&m_rectInitialFrame, pb, sizeof(CRect));
      delete pb;
   }
   else
      m_rectInitialFrame.SetRect(0,0,0,0);


   CRect rectScreen(0, 0, GetSystemMetrics(SM_CXSCREEN),
      GetSystemMetrics(SM_CYSCREEN));
   CRect rectInt;
   rectInt.IntersectRect(&rectScreen, &m_rectInitialFrame);
   if (rectInt.Width() < 10 || rectInt.Height() < 10)
      m_rectInitialFrame.SetRect(0, 0, 0, 0);

   if (GetProfileBinary(szSection, szPageMargin, &pb, &nLen))
   {
      ASSERT(nLen == sizeof(CRect));
      memcpy(&m_rectPageMargin, pb, sizeof(CRect));
      delete pb;
   }
   else
      m_rectPageMargin.SetRect(1800, 1440, 1800, 1440);  

   m_optionsText.LoadOptions(szTextSection);
   m_optionsRTF.LoadOptions(szRTFSection);
   m_optionsWord.LoadOptions(szWordSection);
   m_optionsWrite.LoadOptions(szWriteSection);
   m_optionsIP.LoadOptions(szIPSection);
}

void CWordPadApp::LoadAbbrevStrings()
{
   for (int i=0;i<m_nNumUnits;i++)
      m_units[i].m_strAbbrev.LoadString(m_units[i].m_nAbbrevID);
}

BOOL CWordPadApp::ParseMeasurement(LPTSTR buf, int& lVal)
{
   TCHAR* pch;
   if (buf[0] == NULL)
      return FALSE;
   float f = (float)_tcstod(buf,&pch);

   // eat white space, if any
   while (isspace(*pch))
      pch++;

   if (pch[0] == NULL) // default
   {
      lVal = (f < 0.f) ? (int)(f*GetTPU()-0.5f) : (int)(f*GetTPU()+0.5f);
      return TRUE;
   }
   for (int i=0;i<m_nNumUnits;i++)
   {
      if (lstrcmpi(pch, GetAbbrev(i)) == 0)
      {
         lVal = (f < 0.f) ? (int)(f*GetTPU(i)-0.5f) : (int)(f*GetTPU(i)+0.5f);
         return TRUE;
      }
   }
   return FALSE;
}

void CWordPadApp::PrintTwips(TCHAR* buf, int nValue, int nDec)
{
   ASSERT(nDec == 2);
   int div = GetTPU();
   int lval = nValue;
   BOOL bNeg = FALSE;
   
   int* pVal = new int[nDec+1];

   if (lval < 0)
   {
      bNeg = TRUE;
      lval = -lval;
   }

   for (int i=0;i<=nDec;i++)
   {
      pVal[i] = lval/div; //integer number
      lval -= pVal[i]*div;
      lval *= 10;
   }
   i--;
   if (lval >= div/2)
      pVal[i]++;

   while ((pVal[i] == 10) && (i != 0))
   {
      pVal[i] = 0;
      pVal[--i]++;
   }

   while (nDec && pVal[nDec] == 0)
      nDec--;

   _stprintf(buf, _T("%.*f"), nDec, (float)nValue/(float)div);

   if (m_units[m_nUnits].m_bSpaceAbbrev)
      lstrcat(buf, _T(" "));
   lstrcat(buf, GetAbbrev());
   delete []pVal;
}

/////////////////////////////////////////////////////////////////////////////
// CWordPadApp commands

void CWordPadApp::OnAppAbout()
{
   CString strTitle;
   VERIFY(strTitle.LoadString(AFX_IDS_APP_TITLE));
   ShellAbout(m_pMainWnd->GetSafeHwnd(), strTitle, _T(""), LoadIcon(IDR_MAINFRAME));
}

int CWordPadApp::ExitInstance()
{
   FreeLibrary(GetModuleHandle(_T(szRichName)));
   SaveOptions();

   //
   // This tooltip code is a workaround for a bug in MFC 4.0.
   // It <should> be fixed in MFC 4.1.  When NT starts using
   // MFC 4.1 we can remove this.
   //

   _AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
   CToolTipCtrl* pToolTip = pThreadState->m_pToolTip;

   if (NULL != pToolTip)
   {
       pToolTip->DestroyWindow();
       delete pToolTip;
       pThreadState->m_pToolTip = NULL ;
   }

   return CWinApp::ExitInstance();
}

void CWordPadApp::OnFileNew()
{
   int nDocType = -1;
   if (!m_bPromptForType)
   {
      if (cmdInfo.m_bForceTextMode)
         nDocType = RD_TEXT;
      else if (!cmdInfo.m_strFileName.IsEmpty())
      {
         CFileException fe;
         nDocType = GetDocTypeFromName(cmdInfo.m_strFileName, fe);
      }
      if (nDocType == -1)
         nDocType = RD_DEFAULT;
   }
   else
   {
      CFileNewDialog dlg;
      if (dlg.DoModal() == IDCANCEL)
         return;

      nDocType = (dlg.m_nSel == 0) ? RD_DEFAULT:   //Word 6
               (dlg.m_nSel == 1) ? RD_RICHTEXT :   //RTF
               (dlg.m_nSel == 2) ? RD_TEXT :       //text
               RD_UNICODETEXT;

      if (nDocType != RD_TEXT)
         cmdInfo.m_bForceTextMode = FALSE;
   }
   m_nNewDocType = nDocType;
   DocTemplate.OpenDocumentFile(NULL);
      // if returns NULL, the user has already been alerted
}

// prompt for file name - used for open and save as
// static function called from app
BOOL CWordPadApp::PromptForFileName(CString& fileName, UINT nIDSTitle,
   DWORD dwFlags, BOOL bOpenFileDialog, int* pType)
{
   ScanForConverters();
   CFileDialog dlgFile(bOpenFileDialog);
   CString title;

   VERIFY(title.LoadString(nIDSTitle));
   
   dlgFile.m_ofn.Flags |= dwFlags;

   int nIndex = m_nFilterIndex;
   if (!bOpenFileDialog)
   {
      int nDocType = (pType != NULL) ? *pType : RD_DEFAULT;
      nIndex = GetIndexFromType(nDocType, bOpenFileDialog);
      if (nIndex == -1)
         nIndex = GetIndexFromType(RD_DEFAULT, bOpenFileDialog);
      if (nIndex == -1)
         nIndex = GetIndexFromType(RD_NATIVE, bOpenFileDialog);
      ASSERT(nIndex != -1);
      nIndex++;
   }
   dlgFile.m_ofn.nFilterIndex = nIndex;
   // strDefExt is necessary to hold onto the memory from GetExtFromType
   CString strDefExt = GetExtFromType(GetTypeFromIndex(nIndex-1, bOpenFileDialog));

   //
   // The open file dialog doesn't want the extension to start with '.' but
   // thats how GetExtFromType gives it to us.
   //

   dlgFile.m_ofn.lpstrDefExt = strDefExt;
   ASSERT(TEXT('.') == *dlgFile.m_ofn.lpstrDefExt);
   ++dlgFile.m_ofn.lpstrDefExt;

   CString strFilter = GetFileTypes(bOpenFileDialog);
   dlgFile.m_ofn.lpstrFilter = strFilter;
   dlgFile.m_ofn.lpstrTitle = title;
   dlgFile.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);

   BOOL bRet = (dlgFile.DoModal() == IDOK) ? TRUE : FALSE;
   fileName.ReleaseBuffer();
   if (bRet)
   {
      if (bOpenFileDialog)
         m_nFilterIndex = dlgFile.m_ofn.nFilterIndex;
      if (pType != NULL)
      {
         int nIndex = (int)dlgFile.m_ofn.nFilterIndex - 1;
         ASSERT(nIndex >= 0);
         *pType = GetTypeFromIndex(nIndex, bOpenFileDialog);
      }
   }
   return bRet;
}

void CWordPadApp::OnFileOpen()
{
   // prompt the user (with all document templates)
   CString newName;
   int nType = RD_DEFAULT;
   if (!PromptForFileName(newName, AFX_IDS_OPENFILE,
     OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, TRUE, &nType))
      return; // open cancelled

   if (nType == RD_OEMTEXT)
      m_bForceOEM = TRUE;
   OpenDocumentFile(newName);
   m_bForceOEM = FALSE;
   // if returns NULL, the user has already been alerted
}

BOOL CWordPadApp::OnDDECommand(LPTSTR /*lpszCommand*/)
{
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// DDE and ShellExecute support

//HKEY_CLASSES_ROOT\.RTF = rtffile
//HKEY_CLASSES_ROOT\rtffile =
//HKEY_CLASSES_ROOT\rtffile\CLSID = {73FDDC80-AEA9-101A-98A7-00AA00374959}
//HKEY_CLASSES_ROOT\rtffile\protocol\StdFileEditing\server = WORDPAD.EXE
//HKEY_CLASSES_ROOT\rtffile\protocol\StdFileEditing\verb\0 = &Edit
//HKEY_CLASSES_ROOT\rtffile\shell\open\command = WORDPAD.EXE %1
//HKEY_CLASSES_ROOT\rtffile\shell\print\command = WORDPAD.EXE /p %1

#define REGENTRY(key, value) _T(key) _T("\0\0") _T(value)
#define REGENTRYX(key, valuename, value) _T(key) _T("\0") _T(valuename) _T("\0") _T(value)

static const TCHAR sz00[] = REGENTRY("%2", "%5");
static const TCHAR sz01[] = REGENTRY("%2\\CLSID", "%1");
static const TCHAR sz02[] = REGENTRY("%2\\Insertable", "");
static const TCHAR sz03[] = REGENTRY("%2\\protocol\\StdFileEditing\\verb\\0", "&Edit");
static const TCHAR sz04[] = REGENTRY("%2\\protocol\\StdFileEditing\\server", "%3");
static const TCHAR sz05[] = REGENTRY("CLSID\\%1", "%5");
static const TCHAR sz06[] = REGENTRY("CLSID\\%1\\ProgID", "%2");
static const TCHAR sz07[] = REGENTRY("CLSID\\%1\\InprocHandler32", "ole32.dll");
static const TCHAR sz08[] = REGENTRY("CLSID\\%1\\LocalServer32", "%3");
static const TCHAR sz09[] = REGENTRY("CLSID\\%1\\Verb\\0", "&Edit,0,2");
static const TCHAR sz10[] = REGENTRY("CLSID\\%1\\Verb\\1", "&Open,0,2");
static const TCHAR sz11[] = REGENTRY("CLSID\\%1\\Insertable", "");
static const TCHAR sz12[] = REGENTRY("CLSID\\%1\\AuxUserType\\2", "%4");
static const TCHAR sz13[] = REGENTRY("CLSID\\%1\\AuxUserType\\3", "%6");
static const TCHAR sz14[] = REGENTRY("CLSID\\%1\\DefaultIcon", "%3,1");
static const TCHAR sz15[] = REGENTRY("CLSID\\%1\\MiscStatus", "0");
static const TCHAR sz16[] = REGENTRY("%2\\shell\\open\\command", "%3 \"%%1\"");
static const TCHAR sz17[] = REGENTRY("%2\\shell\\print\\command", "%3 /p \"%%1\"");
static const TCHAR sz18[] = REGENTRY("%7", "%2");
static const TCHAR sz19[] = REGENTRY("%2", ""); // like sz00 only no long type name
static const TCHAR sz20[] = REGENTRY("%2\\shell\\printto\\command", "%3 /pt \"%%1\" \"%%2\" \"%%3\" \"%%4\"");
static const TCHAR sz21[] = REGENTRY("%2\\DefaultIcon", "%3,%8");
static const TCHAR sz22[] = REGENTRYX("%7\\ShellNew", "NullFile", "true");
static const TCHAR sz23[] = REGENTRYX("%7\\ShellNew", "Data", "{\\rtf1}");

// %1 - class ID
// %2 - class name            WordPad.Document.1
// %3 - SFN executable path      C:\PROGRA~1\ACCESS~1\WORDPAD.EXE
// %4 - short type name       Document
// %5 - long type name        Microsoft WordPad Document
// %6 - long application name Microsoft WordPad
// %7 = extension          .rtf
// %8 = default icon       0,1,2,3
#define NUM_REG_ARGS 8

static const LPCTSTR rglpszWordPadRegister[] =
{sz00, sz02, sz03, sz05, sz09, sz10, sz11, sz15, NULL};

static const LPCTSTR rglpszWordPadOverwrite[] =
{sz01, sz04, sz06, sz07, sz08, sz12, sz13, sz14, sz16, sz17, sz20, NULL};

//static const LPCTSTR rglpszExtRegister[] =
//{sz00, sz18, NULL};

//static const LPCTSTR rglpszExtOverwrite[] =
//{sz01, sz16, sz17, sz21, NULL};

static const LPCTSTR rglpszWriExtRegister[] =
{sz18, NULL};
static const LPCTSTR rglpszWriRegister[] =
{sz00, sz01, sz16, sz17, sz20, sz21, NULL};

static const LPCTSTR rglpszRtfExtRegister[] =
{sz18, sz23, NULL};
static const LPCTSTR rglpszRtfRegister[] =
{sz00, sz01, sz16, sz17, sz20, sz21, NULL};

static const LPCTSTR rglpszTxtExtRegister[] =
{sz18, sz22, NULL};
static const LPCTSTR rglpszTxtRegister[] =
{sz00, sz01, sz16, sz17, sz20, sz21, NULL};

static const LPCTSTR rglpszDocExtRegister[] =
{sz18, sz22, NULL};
static const LPCTSTR rglpszDocRegister[] =
{sz00, sz01, sz16, sz17, sz20, sz21, NULL};

static void RegisterExt(LPCTSTR lpszExt, LPCTSTR lpszProgID, UINT nIDTypeName,
   LPCTSTR* rglpszSymbols, LPCTSTR* rglpszExtRegister,
   LPCTSTR* rglpszRegister, int nIcon)
{
   // don't overwrite anything with the extensions
   CString strWhole;
   VERIFY(strWhole.LoadString(nIDTypeName));
   CString str;
   AfxExtractSubString(str, strWhole, DOCTYPE_PROGID);

   rglpszSymbols[1] = lpszProgID;
   rglpszSymbols[4] = str;
   rglpszSymbols[6] = lpszExt;
   TCHAR buf[10];
   wsprintf(buf, _T("%d"), nIcon);
   rglpszSymbols[7] = buf;
   // check for .ext and progid
   CKey key;
   if (!key.Open(HKEY_CLASSES_ROOT, lpszExt)) // .ext doesn't exist
      RegisterHelper(rglpszExtRegister, rglpszSymbols, TRUE);
   key.Close();
   if (!key.Open(HKEY_CLASSES_ROOT, lpszProgID)) // ProgID doesn't exist (i.e. txtfile)
      RegisterHelper(rglpszRegister, rglpszSymbols, TRUE);
}

void CWordPadApp::UpdateRegistry()
{
   USES_CONVERSION;
   LPOLESTR lpszClassID = NULL;
   CDocTemplate* pDocTemplate = &DocTemplate;

   // get registration info from doc template string
   CString strServerName;
   CString strLocalServerName;
   CString strLocalShortName;

   if (!pDocTemplate->GetDocString(strServerName,
      CDocTemplate::regFileTypeId) || strServerName.IsEmpty())
   {
      TRACE0("Error: not enough information in DocTemplate to register OLE server.\n");
      return;
   }
   if (!pDocTemplate->GetDocString(strLocalServerName,
      CDocTemplate::regFileTypeName))
      strLocalServerName = strServerName;     // use non-localized name
   if (!pDocTemplate->GetDocString(strLocalShortName,
      CDocTemplate::fileNewName))
      strLocalShortName = strLocalServerName; // use long name

   ASSERT(strServerName.Find(' ') == -1);  // no spaces allowed

   ::StringFromCLSID(clsid, &lpszClassID);
   ASSERT (lpszClassID != NULL);

   // get path name to server
   TCHAR szLongPathName[_MAX_PATH];
   TCHAR szShortPathName[_MAX_PATH];
   ::GetModuleFileName(AfxGetInstanceHandle(), szLongPathName, _MAX_PATH);
   ::GetShortPathName(szLongPathName, szShortPathName, _MAX_PATH);
   
   LPCTSTR rglpszSymbols[NUM_REG_ARGS];
   rglpszSymbols[0] = OLE2CT(lpszClassID);
   rglpszSymbols[1] = strServerName;
   rglpszSymbols[2] = szShortPathName;
   rglpszSymbols[3] = strLocalShortName;
   rglpszSymbols[4] = strLocalServerName;
   rglpszSymbols[5] = m_pszAppName; // will usually be long, readable name
   rglpszSymbols[6] = NULL;

   if (RegisterHelper((LPCTSTR*)rglpszWordPadRegister, rglpszSymbols, FALSE))
      RegisterHelper((LPCTSTR*)rglpszWordPadOverwrite, rglpszSymbols, TRUE);

// RegisterExt(_T(".txt"), _T("txtfile"), IDS_TEXT_DOC, rglpszSymbols,
//    (LPCTSTR*)rglpszTxtExtRegister, (LPCTSTR*)rglpszTxtRegister, 3);
   RegisterExt(_T(".rtf"), _T("rtffile"), IDS_RICHTEXT_DOC, rglpszSymbols,
      (LPCTSTR*)rglpszRtfExtRegister, (LPCTSTR*)rglpszRtfRegister, 1);
   RegisterExt(_T(".wri"), _T("wrifile"), IDS_WRITE_DOC, rglpszSymbols,
      (LPCTSTR*)rglpszWriExtRegister, (LPCTSTR*)rglpszWriRegister, 2);
   RegisterExt(_T(".doc"), _T("WordPad.Document.1"), IDS_WINWORD6_DOC, rglpszSymbols,
      (LPCTSTR*)rglpszDocExtRegister, (LPCTSTR*)rglpszDocRegister, 1);

   // free memory for class ID
   ASSERT(lpszClassID != NULL);
   CoTaskMemFree(lpszClassID);
}

BOOL RegisterHelper(LPCTSTR* rglpszRegister, LPCTSTR* rglpszSymbols,
   BOOL bReplace)
{
   ASSERT(rglpszRegister != NULL);
   ASSERT(rglpszSymbols != NULL);

   CString strKey;
   CString strValueName;
   CString strValue;

   // keeping a key open makes this go a bit faster
   CKey keyTemp;
   VERIFY(keyTemp.Create(HKEY_CLASSES_ROOT, _T("CLSID")));

   BOOL bResult = TRUE;
   while (*rglpszRegister != NULL)
   {
      LPCTSTR lpszKey = *rglpszRegister++;
      if (*lpszKey == '\0')
         continue;

      LPCTSTR lpszValueName = lpszKey + lstrlen(lpszKey) + 1;
      LPCTSTR lpszValue = lpszValueName + lstrlen(lpszValueName) + 1;

      strKey.ReleaseBuffer(
         FormatMessage(FORMAT_MESSAGE_FROM_STRING |
         FORMAT_MESSAGE_ARGUMENT_ARRAY, lpszKey, NULL,   NULL,
         strKey.GetBuffer(256), 256, (va_list*) rglpszSymbols));
      strValueName = lpszValueName;
      strValue.ReleaseBuffer(
         FormatMessage(FORMAT_MESSAGE_FROM_STRING |
         FORMAT_MESSAGE_ARGUMENT_ARRAY, lpszValue, NULL, NULL,
         strValue.GetBuffer(256), 256, (va_list*) rglpszSymbols));

      if (strKey.IsEmpty())
      {
         TRACE1("Warning: skipping empty key '%s'.\n", lpszKey);
         continue;
      }

      CKey key;
      VERIFY(key.Create(HKEY_CLASSES_ROOT, strKey));
      if (!bReplace)
      {
         CString str;
         if (key.GetStringValue(str, strValueName) && !str.IsEmpty())
            continue;
      }

      if (!key.SetStringValue(strValue, strValueName))
      {
         TRACE2("Error: failed setting key '%s' to value '%s'.\n",
            (LPCTSTR)strKey, (LPCTSTR)strValue);
         bResult = FALSE;
         break;
      }
   }

   return bResult;
}

void CWordPadApp::WinHelp(DWORD dwData, UINT nCmd)
{
   if (g_fDisableStandardHelp)
   {
       return ;
   }

   if (nCmd == HELP_INDEX || nCmd == HELP_CONTENTS)
      nCmd = HELP_FINDER;
   CWinApp::WinHelp(dwData, nCmd);
}

BOOL CWordPadApp::PreTranslateMessage(MSG* pMsg)
{
   if (pMsg->message == WM_PAINT)
      return FALSE;
   // CWinApp::PreTranslateMessage does nothing but call base
   return CWinThread::PreTranslateMessage(pMsg);
}

void CWordPadApp::NotifyPrinterChanged(BOOL bUpdatePrinterSelection)
{
   if (bUpdatePrinterSelection)
      UpdatePrinterSelection(FALSE);
   POSITION pos = m_listPrinterNotify.GetHeadPosition();
   while (pos != NULL)
   {
      HWND hWnd = m_listPrinterNotify.GetNext(pos);
      ::SendMessage(hWnd, m_nPrinterChangedMsg, 0, 0);
   }
}

BOOL CWordPadApp::IsIdleMessage(MSG* pMsg)
{
   if (pMsg->message == WM_MOUSEMOVE || pMsg->message == WM_NCMOUSEMOVE)
      return FALSE;
   return CWinApp::IsIdleMessage(pMsg);
}

#define DN_PADDINGCHARS 16

HGLOBAL CWordPadApp::CreateDevNames()
{
    HGLOBAL hDev = NULL;
    CString strDriverName;
    CString strPrinterName;
    CString strPortName;
 
    if (!cmdInfo.m_strPrinterName.IsEmpty())
    {
        strDriverName = cmdInfo.m_strDriverName;
        strPrinterName = cmdInfo.m_strPrinterName;
        strPortName = cmdInfo.m_strPortName;
    }
    else
    {
        PRINTDLG    printdlg;
        DEVNAMES   *devnames;

        if (!GetPrinterDeviceDefaults(&printdlg))
            return NULL;

        devnames = (DEVNAMES *) ::GlobalLock(printdlg.hDevNames);
        if (NULL == devnames)
            return NULL;

        strDriverName = (LPTSTR) ((BYTE *) devnames) + devnames->wDriverOffset;
        strPrinterName = (LPTSTR) ((BYTE *) devnames) + devnames->wDeviceOffset;
        strPortName = (LPTSTR) ((BYTE *) devnames) + devnames->wOutputOffset;

        ::GlobalUnlock(printdlg.hDevNames);
    }
 
    DWORD cbDevNames ;

    cbDevNames = strDriverName.GetLength() + 1 +
                 strPrinterName.GetLength() + 1 +
                 strPortName.GetLength() + 1 +
                 DN_PADDINGCHARS ;

    cbDevNames *= sizeof(TCHAR) ;
    cbDevNames += sizeof(DEVNAMES) ;

    hDev = GlobalAlloc(GPTR, cbDevNames) ;

    LPDEVNAMES lpDev = (LPDEVNAMES)GlobalLock(hDev) ;

    lpDev->wDriverOffset = sizeof(DEVNAMES) / sizeof(TCHAR) ;
    lstrcpy((LPTSTR) lpDev + lpDev->wDriverOffset, strDriverName) ;

    lpDev->wDeviceOffset = (WORD) (lpDev->wDriverOffset 
                                    + strDriverName.GetLength() + 1);
    lstrcpy((LPTSTR) lpDev + lpDev->wDeviceOffset, strPrinterName) ;

    lpDev->wOutputOffset = (WORD) (lpDev->wDeviceOffset 
                                    + strPrinterName.GetLength() + 1);
    lstrcpy((LPTSTR) lpDev + lpDev->wOutputOffset, strPortName) ;

    lpDev->wDefault = 0;
   
    return hDev;
}

//+---------------------------------------------------------------------------
//
//  Method:     CWordPadApp::DoDeferredInitialization, static
//
//  Synopsis:   Thread entry point for low priority initialization
//
//  Parameters: [pvWordPadApp]          -- Pointer to to CWordPadApp
//
//  Returns:    Thread exit code
//
//---------------------------------------------------------------------------

UINT AFX_CDECL CWordPadApp::DoDeferredInitialization(LPVOID pvWordPadApp)
{
    ASSERT(NULL != pvWordPadApp);


    CWordPadApp *pWordPadApp = (CWordPadApp *) pvWordPadApp;

    pWordPadApp->m_initialization_phase = InitializingPrinter;
    pWordPadApp->CreateDevNames();

    pWordPadApp->m_initialization_phase = UpdatingPrinterRelatedUI;
    pWordPadApp->NotifyPrinterChanged( ((pWordPadApp->m_hDevNames) == NULL) );

    pWordPadApp->m_initialization_phase = UpdatingRegistry;
    pWordPadApp->UpdateRegistry();

    pWordPadApp->m_initialization_phase = InitializationComplete;

    return 0;
}

//+--------------------------------------------------------------------------
//
//  Method:     CWordPadApp::EnsurePrinterIsInitialized
//
//  Synopsis:   Make sure the printer is done initializing
//
//  Parameters: None
//
//  Returns:    void
//
//  Notes:      We'll only wait two minutes.  If the printer takes that long
//              to initialize printing probably is going to be flakey anyway.
//
//              The main purpose of doing printer initialization on a different
//              thread is so that it doesn't get in the way of doing real work.
//              On the other hand if printing is the real work we should try to
//              get things moving so bump it up from idle priority.
//
//---------------------------------------------------------------------------

void CWordPadApp::EnsurePrinterIsInitialized()
{
    int     nWaits = 0;
    BOOL    bBumpedPriority = FALSE;

    if (NULL == m_pInitializationThread)
    {
        ASSERT(NULL != m_pInitializationThread);
        bBumpedPriority = TRUE;
    }

    while (m_initialization_phase <= InitializingPrinter && nWaits < 1200)
    {
        if (!bBumpedPriority)
        {
            m_pInitializationThread->SetThreadPriority(
                                            THREAD_PRIORITY_ABOVE_NORMAL);
            bBumpedPriority = TRUE;
        }

        Sleep(100);
        ++nWaits;
    }

    if (bBumpedPriority)
        m_pInitializationThread->SetThreadPriority(THREAD_PRIORITY_IDLE);
}
