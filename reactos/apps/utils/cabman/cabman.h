/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        apps/cabman/cabman.h
 * PURPOSE:     Cabinet manager header
 */
#ifndef __CABMAN_H
#define __CABMAN_H

#include "cabinet.h"
#include "dfp.h"

/* Cabinet manager modes */
#define CM_MODE_CREATE   0
#define CM_MODE_DISPLAY  1
#define CM_MODE_EXTRACT  2


/* Classes */

class CCABManager : public CDFParser {
public:
    CCABManager();
    virtual ~CCABManager();
    BOOL ParseCmdline(INT argc, PCHAR argv[]);
    VOID Run();
private:
    VOID Usage();
    VOID CreateCabinet();
    VOID DisplayCabinet();
    VOID ExtractFromCabinet();
    /* Event handlers */
    virtual BOOL OnOverwrite(PCFFILE File, LPTSTR FileName);
    virtual VOID OnExtract(PCFFILE File, LPTSTR FileName);
    virtual VOID OnDiskChange(LPTSTR CabinetName, LPTSTR DiskLabel);
    virtual VOID OnAdd(PCFFILE Entry, LPTSTR FileName);
    /* Configuration */
    BOOL ProcessAll;
    DWORD Mode;
    BOOL PromptOnOverwrite;
    TCHAR Location[MAX_PATH];
    TCHAR FileName[MAX_PATH];
};

#endif /* __CABMAN_H */

/* EOF */
