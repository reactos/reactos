/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     Test program for cabinet classes
 * FILE:        apps/cabman/test.h
 * PURPOSE:     Test program header
 */
#ifndef __TEST_H
#define __TEST_H

#define CAB_READ_ONLY   // Define for smaller read only version
#include "cabinet.h"


/* Classes */

class CCABTest : public CCabinet {
public:
    CCABTest();
    virtual ~CCABTest();
    VOID ExtractFromCabinet();
    /* Event handlers */
    virtual BOOL OnOverwrite(PCFFILE File, LPTSTR FileName);
    virtual VOID OnExtract(PCFFILE File, LPTSTR FileName);
    virtual VOID OnDiskChange(LPTSTR CabinetName, LPTSTR DiskLabel);
private:
    /* Configuration */
    BOOL PromptOnOverwrite;
};

#endif /* __CABMAN_H */

/* EOF */
