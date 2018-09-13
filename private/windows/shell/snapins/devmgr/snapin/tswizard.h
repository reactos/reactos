
#ifndef __TSWIZARD_H__
#define __TSWIZARD_H__

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    tswizard.h

Abstract:

    header file for tswizard.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/

typedef enum tagFixCommand
{
    FIX_COMMAND_DONOTHING = 0,
    FIX_COMMAND_UPGRADEDRIVERS,
    FIX_COMMAND_REINSTALL,
    FIX_COMMAND_ENABLEDEVICE,
    FIX_COMMAND_STARTDEVICE,
    FIX_COMMAND_RESTARTCOMPUTER,
    FIX_COMMAND_RESOURCE,
    FIX_COMMAND_TROUBLESHOOTER
} FIX_COMMAND, *PFIX_COMMAND;

typedef struct tagCMProblemInfo
{
    BOOL    Query;      // true if we have something to fix the problem
    FIX_COMMAND FixCommand; // command to fix the problem
    int     idInstFirst;    // instruction text string id
    int     idInstCount;    // how many instruction string id
    int     idFixit;        // fix it string id
}CMPROBLEM_INFO, *PCMPROBLEM_INFO;

class CProblemAgent
{
public:
    CProblemAgent(CDevice* pDevice, ULONG Problem, BOOL SeparateProcess);
    ~CProblemAgent()
    {}
    // retreive the problem description text
    DWORD InstructionText(LPTSTR Buffer, DWORD BufferSize);
    DWORD FixitText(LPTSTR Buffer, DWORD BufferSize);
    // fix the problem
    BOOL FixIt(HWND hwndOwner);
    BOOL UpgradeDriver(HWND hwndOwner, CDevice* pDevice);
    BOOL Reinstall(HWND hwndOwner, CDevice* pDevice);
    BOOL RestartComputer(HWND hwndOwner, CDevice* pDevice);
    BOOL EnableDevice(HWND hwndOwner, CDevice* pDevice);
    BOOL StartTroubleShooter(HWND hwndOwner, CDevice *pDevice, LPTSTR ChmFile, LPTSTR HtmlTroubleShooter);
    BOOL GetTroubleShooter(CDevice* pDevice, LPTSTR ChmFile, LPTSTR HtmlTroubleShooter);
    void LaunchHtlmTroubleShooter(HWND hwndOwner, LPTSTR ChmFile, LPTSTR HtmlTroubleShooter);

protected:
    CDevice*    m_pDevice;
    ULONG   m_Problem;
    int     m_idInstFirst;
    int     m_idInstCount;
    int     m_idFixit;
    BOOL    m_SeparateProcess;
    FIX_COMMAND m_FixCommand;
};

class CWizard98
{
public:
    CWizard98(HWND hwndParent, UINT MaxPages = 32);
    ~CWizard98()
    {}
    INT_PTR DoSheet() {

        return ::PropertySheet(&m_psh);
    }

    void InsertPage(HPROPSHEETPAGE hPage) {

        if (hPage && (m_psh.nPages < m_MaxPages)) {

            m_psh.phpage[m_psh.nPages++] = hPage;
        }
    }

    static INT CALLBACK WizardCallback(HWND hwndDlg, UINT uMsg, LPARAM lParam);

    PROPSHEETHEADER m_psh;

private:
    UINT m_MaxPages;
};

#endif  // #ifndef  __TSWIZARD_H__
