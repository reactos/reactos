/*
**------------------------------------------------------------------------------
** Module:  Disk Space Cleanup Manager
** File:    seldrive.h
**
** Purpose: Code that implements the "Select Drive" dialog
**
** Notes:   
** Mod Log: Created by Jason Cobb (12/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/
#ifndef SELDRIVE_H
#define SELDRIVE_H

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#include "common.h"
#include "diskutil.h"

/*
**------------------------------------------------------------------------------
** Typedefs
**------------------------------------------------------------------------------
*/
typedef enum		// drvlist
{
  	dvlANYLOCAL		// any non-network, non-CDRom drive
} drvlist;

/*
**------------------------------------------------------------------------------
** Prototypes
**------------------------------------------------------------------------------
*/
BOOL 
SelectDrive(
    PTSTR szDrive, 
    drvlist dvl
    );

void
GetBootDrive(
	PTCHAR pDrive,
	DWORD Size
	);
	
#endif

