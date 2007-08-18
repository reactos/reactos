/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/cabman.h
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
#define CM_MODE_CREATE_SIMPLE 3

/* Classes */

class CCABManager : public CDFParser {
public:
	CCABManager();
	virtual ~CCABManager();
	bool ParseCmdline(int argc, char* argv[]);
	bool Run();
private:
	void Usage();
	bool CreateCabinet();
	bool CreateSimpleCabinet();
	bool DisplayCabinet();
	bool ExtractFromCabinet();
	/* Event handlers */
	virtual bool OnOverwrite(PCFFILE File, char* FileName);
	virtual void OnExtract(PCFFILE File, char* FileName);
	virtual void OnDiskChange(char* CabinetName, char* DiskLabel);
	virtual void OnAdd(PCFFILE Entry, char* FileName);
	/* Configuration */
	bool ProcessAll;
	ULONG Mode;
	bool PromptOnOverwrite;
	char Location[MAX_PATH];
	char FileName[MAX_PATH];
};

#endif /* __CABMAN_H */

/* EOF */
