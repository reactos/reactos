
#ifndef __TSMAIN_H__
#define __TSMAIN_H__

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    tsmain.h

Abstract:

    header file for tsmain.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/

//
// Wizard32 command and parameter definitions
//

//
//
// INPUT:
//  pParam  -- troubleshooter parameter. The contents vary depends on
//          the command specified in the block. See below
//          for detail.
//
//
// OUTPUT:
//  TRUE -- if the function succeeded.
//  FALSE -- if the function failed. GetLastError() should be able
//        to retreive the error code


typedef enum tagTShooterCommand
{
    TSHOOTER_QUERY = 0,
    TSHOOTER_ABOUT,
    TSHOOTER_ADDPAGES
}TSHOOTER_COMMAND, *PTSHOOTER_COMMAND;

// parameter header.
typedef struct tagTShooterParamHeader
{
    DWORD cbSize;           // size of the entire structure
    TSHOOTER_COMMAND Command;       // command
}TSHOOTER_PARAMHEADER, *PTSHOOTER_PARAMHEADER;

//
// QUERY command asks the troubleshooter the following questions:
// (1). if the troubleshooter supports the given DeviceId/Problem combination.
// (2). the brief description about the troubleshooter
// (3). the ranking on the DeviceId and the problem.
// If the troubleshooter does not support the device id and problem
// combination, it should resturn FALSE and set the error code
// to ERROR_INVALID_FUNCTION. The DescBuffer and DescBufferSize
// can be ignored. If the provided DescBuffer is too small, it should
// fill DescBufferSize with the required size, set the error code
// to ERROR_INSUFFICIENT_BUFFER and return FALSE.
//
// parameter definition for TSHOOTER_QUERY command
//
// The Header.Command  must be TSHOOTER_QUERY;
// The header.cbSize must be sizeof(TSHOOTER_QUERY_PARAM);
//
// IN DeviceId  -- device instance id
// IN Problem   -- Configuration manager defined problem number
// IN OUT DescBuffer -- buffer to receive the troubleshooter description text
// IN OUT DescBufferSize -- Description buffer size in char(byte for ANSI)
//              Troubleshooter should fill this field with required
//              size on return.
// OUT DeviceRank   -- to receive the device ranking
// OUT ProblemRank -- to receive the problem ranking
//
//
typedef struct tagTShooterQueryParam
{
    TSHOOTER_PARAMHEADER    Header;
    LPCTSTR         DeviceId;
    ULONG           Problem;
    LPTSTR          DescBuffer;
    DWORD           DescBufferSize;
    DWORD           DeviceRank;
    DWORD           ProblemRank;
}TSHOOTER_QUERYPARAM, *PTSHOOTER_QUERYPARAM;

// The TSHOOTER_ABOUT asks the troubleshooter to display its about dialog box.
// The about dialog box should tell user what the troubleshooter is all about.
//
// The about dialog box is supposed to be Modal. If the troubleshooter
// implements a modaless about dialog box, it should disable
// the given hwndOwner after the dialog box is created and reenable
// it after the dialog box is destroyed.
//
// parameter definition for ABOUT troubleshooter command
//
// Header.Command must be TSHOOTER_ABOUT;
// Header.cbSize must be sizeof(TSHOOTER_ABOUT_PARAM);
//
// IN hwndOwner -- the window handle to serve as the owner window
//         of the troubleshooter about dialog box
//
//
typedef struct tagTShooterAboutParam
{
    TSHOOTER_PARAMHEADER    Header;
    HWND            hwndOwner;
}TSHOOTER_ABOUTPARAM, *PTSHOOTER_ABOUTPARAM;

//
// The TSHOOTER_ADDPAGES asks the troubleshooter to add its wizard
// pages to the provided property sheet header.
//
//
// parameter definition for ADDPAGES troubleshooter command
//
// Header.Command must be TSHOOTER_ADDPAGRES;
// Header.cbSize must be sizeof(TSHOOTER_ADDPAGES_PARAM);
//
// IN DeviceId  -- the hardware id of the device
// IN Problem   -- Configuration Manager defined problem number
// IN OUT ppsh  -- property sheet header to which the troubleshooter
//         add its pages
// IN MaxPages  -- total pages alloated for the ppsh.
//         The troubleshooter should not add more than
//         (MaxPages - ppsh.nPages) pages
//

typedef struct tagTShooterAddPagesParam
{
    TSHOOTER_PARAMHEADER    Header;
    LPCTSTR         DeviceId;
    ULONG           Problem;
    LPPROPSHEETHEADER       PropSheetHeader;
    DWORD           MaxPages;
}TSHOOTER_ADDPAGESPARAM, *PTSHOOTER_ADDPAGESPARAM;

// Each Troubleshooting wizard must provide an entry point for Device Manager
// to call:

typedef BOOL (APIENTRY *WIZARDENTRY)(PTSHOOTER_PARAMHEADER pParam);



typedef enum tagTShooterWizadType
{
    TWT_ANY = 0,                // any type of troubleshooter wizard
    TWT_PROBLEM_SPECIFIC,           // problem specific wizards
    TWT_CLASS_SPECIFIC,             // class specific wizards
    TWT_GENERAL_PURPOSE,            // general purpose
    TWT_DEVMGR_DEFAULT              // device manager default
}TSHOOTERWIZARDTYPE, *PTSHOOTERWIZARTYPE;


//
// class that represent a wizard32 troubleshooter
//
class CWizard
{
public:
    CWizard(HMODULE hWizardDll, FARPROC WizardEntry)
    : m_WizardEntry((WIZARDENTRY)WizardEntry),
      m_hWizardDll(hWizardDll),
      m_Problem(0),
      m_DeviceRank(0),
      m_ProblemRank(0),
      m_AddedPages(0),
      m_pDevice(NULL)
    {}
    ~CWizard()
    {
        if (m_hWizardDll)
        FreeLibrary(m_hWizardDll);
    }
    LPCTSTR GetDescription()
    {
        return m_strDescription.IsEmpty() ? NULL : (LPCTSTR)m_strDescription;
    }
    virtual BOOL Query(CDevice* pDevice, ULONG Problem);
    virtual BOOL About(HWND hwndOwner);
    virtual BOOL AddPages(LPPROPSHEETHEADER ppsh, DWORD MaxPages);
    ULONG   DeviceRank()
    {
        return m_DeviceRank;
    }
    ULONG   ProblemRank()
    {
        return m_ProblemRank;
    }
    UINT    m_AddedPages;
protected:
    WIZARDENTRY m_WizardEntry;
    HINSTANCE   m_hWizardDll;
    String  m_strDescription;
    ULONG   m_Problem;
    ULONG   m_DeviceRank;
    ULONG   m_ProblemRank;
    CDevice*    m_pDevice;
};


//
// Class that collects all available troubleshooter
//
class CWizardList
{
public:
    CWizardList(TSHOOTERWIZARDTYPE Type = TWT_ANY) : m_Type(Type)
    {}
    ~CWizardList();
    BOOL Create(CDevice* pDevice, ULONG Problem);
    int NumberOfWizards()
    {
    return m_listWizards.GetCount();
    }
    BOOL GetFirstWizard(CWizard** ppWizard, PVOID* pContext);
    BOOL GetNextWizard(CWizard** ppWizard, PVOID& Context);
private:
    BOOL CreateWizardsFromStrings(LPTSTR msz, CDevice* pDevice, ULONG Problem);
    CList<CWizard*, CWizard*> m_listWizards;
    TSHOOTERWIZARDTYPE      m_Type;
};

//
// class that represents the troubleshooter wizard introduction page
//
class CWizardIntro : public CPropSheetPage
{
public:
    CWizardIntro() : m_pDevice(NULL), m_hFontBold(NULL),
             m_hFontBigBold(NULL),
             m_pSelectedWizard(NULL), m_Problem(0),
             CPropSheetPage(g_hInstance, IDD_WIZINTRO)
    {
    }
    virtual ~CWizardIntro()
    {
        if (m_hFontBold)
        DeleteObject(m_hFontBold);
        if (m_hFontBigBold)
        DeleteObject(m_hFontBigBold);
    }
    virtual BOOL OnInitDialog(LPPROPSHEETPAGE ppsp);
    virtual BOOL OnWizNext();
    virtual BOOL OnSetActive();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    HPROPSHEETPAGE Create(CDevice* pDevice);
private:
    CDevice*    m_pDevice;
    HFONT   m_hFontBold;
    HFONT   m_hFontBigBold;
    CWizardList m_Wizards;
    ULONG   m_Problem;
    CWizard*    m_pSelectedWizard;
};


//
// class that represents the troubleshooter property sheet
//
class CWizard98
{
public:
    CWizard98(HWND hwndParent, UINT MaxPages = 32);

    BOOL CreateIntroPage(CDevice* pDevice);
    UINT GetMaxPages()
    {
        return m_MaxPages;
    }
    INT_PTR DoSheet()
    {
        return ::PropertySheet(&m_psh);
    }
    PROPSHEETHEADER m_psh;
private:
    CDevice* m_pDevice;
    UINT    m_MaxPages;
};



class CTSLauncher
{
public:
    CTSLauncher() : m_hTSL(NULL)
    {}
    ~CTSLauncher()
    {
        Close();
    }
    BOOL Open(LPCTSTR DeviceId, const GUID& ClassGuid, ULONG Problem)
    {
        return FALSE;
    }
    BOOL Close()
    {
        m_hTSL = NULL;
        return TRUE;
    }
    BOOL Go()
    {
        return FALSE;
    }
    BOOL EnumerateStatus(int Index, DWORD& Status)
    {
        return FALSE;
    }
private:
    HANDLE   m_hTSL;
};

#if 0

typedef enum tagFixItCommand
{
    FIXIT_COMMAND_DONOTHING = 0,
    FIXIT_COMMAND_UPGRADEDRIVERS,
    FIXIT_COMMAND_REINSTALL,
    FIXIT_COMMAND_ENABLEDEVICE
    FIXIT_COMMAND_RESTARTCOMPUTER
} FIXIT_COMMAND, *PFIXIT_COMMAND;

class CProblemAgent
{
public:
    CProblemAgent(CDevice* pDevice, ULONG Problem, ULONG Status);
    ~CProblemAgent();
    // retreive the problem description text
    LPCTSTR ProblemText()
    {
        return m_strDescription.IsEmpty() ? NULL : (LPCTSTR)m_strDescription;
    }
    LPCTSTR InstructionText()
    {
        return m_strInstruction.IsEmpty() ? NULL : (LPCTSTR)m_strInstruction;
    }
    // fix the problem
    virtual BOOL FixIt(HWND hwndOwner)
    {
        return TRUE;
    }
protected:
    BOOL UpdateDriver(HWND hwndOwner, m_pDevice);
    BOOL Reinstall(HWND hwndOwner);
    BOOL RestartComputer(HWND hwndOwner);
    BOOL EnableDevice()
    CDevice*    m_pDevice;
    ULONG   m_Problem;
    ULONG   m_Status;
    String  m_strDescription;
    String  m_strInstruction;
    FIXITCOMMAND m_Command;
};

#endif



INT_PTR
StartTroubleshootingWizard(
    HWND hWndParent,
    CDevice* pDevice
    );
#endif // __PROBLEM_H__
