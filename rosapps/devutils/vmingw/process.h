/********************************************************************
*	Module:	process.h. This is part of Visual-MinGW.
*
*	License:	Visual-MinGW is covered by GNU General Public License, 
*			Copyright (C) 2001  Manu B.
*			See license.htm for more details.
*
********************************************************************/
#ifndef PROCESS_H
#define PROCESS_H
#include "winui.h"
#include "CList.h"


class CCommandDlg : public CDlgBase
{
	public:
	CCommandDlg();
	~CCommandDlg();

	HWND Create(void);
	LRESULT CALLBACK CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	BOOL	OnInitDialog(HWND hwndFocus, LPARAM lInitParam);
	BOOL OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);
	char cmdLine[1024];

	protected:

	private:   
	HWND hCmdLine;
};

/********************************************************
		creationFlag truth table
	-----------------------------------
				8	4	2	1
				------------------
	NO_PIPE		0	0	0	0
	IN_PIPE		0	0	0	1
	OUT_PIPE		0	0	1	0
	ERR_PIPE		0	1	0	0
	OUTERR_PIPE	1	0	1	0

*********************************************************/
#define NO_PIPE			0x0000
#define IN_PIPE			0x0001
#define OUT_PIPE		0x0002
#define ERR_PIPE			0x0004
#define OUTERR_PIPE		0x000A

#define STDOUT_NONE			0
#define STDOUT_FILE_APPEND	1
#define STDOUT_USER			2

class CTask : public CNode
{
	public:
	CTask();
	~CTask();

	char	cmdLine[MAX_PATH];
	char	szFileName[MAX_PATH];
	WORD creationFlag;
	WORD outputFlag;

	protected:

	private:

};

class CStack : public CList
{
	public:
	CStack();
	~CStack();

	int Push(CTask * newTask);
	CTask * Pop(void);
	void Flush(void);

	protected:

	private:   
	CTask * retBuf;
	void DetachCurrent(void);
};

class CPipes
{
	public:
	CPipes();
	~CPipes();

	HANDLE hIn[2];
	HANDLE hOut[2];
	HANDLE hErr[2];

	bool Create(WORD creationFlag, bool winNT);
	bool CloseChildSide(void);
	bool CloseParentSide(void);
	protected:

	private:
	bool Close(int side);
};

class CProcess : public CStack
{
	public:
	CProcess();
	~CProcess();

	bool CommandLine(char * cmdLine);
	bool isRunning(void);
	CTask * AddTask(char * cmdLine, WORD creationFlag, WORD outputFlag/* = STDOUT_NONE*/);
	bool CmdCat(char * cmdLine);
	void Run(void);
	void Run_Thread_Internal(void);

	CCommandDlg	CommandDlg;
	protected:

	private:
	PROCESS_INFORMATION pi;
	bool 			Running;
	DWORD 		exitCode;
	CTask * 		currTask;
	CPipes		Pipes;
	char * chr;
	char 	inBuf[1024];
	char 	outBuf[1024];
	char 	errBuf[1024];

	bool	RunProcess(CTask * task);

	void	WriteStdIn(HANDLE hPipe, WORD creationFlag);
	void	ReadStdErr(HANDLE hPipe, WORD creationFlag);
	long	ReadStdOut(CTask * task, HANDLE hPipe);
	int ReadOneChar(HANDLE hPipe, char * chrin);

	bool WriteFileAppend(char * fileName, char * line, int len=-1);
	bool	OutputLine(WORD outputFlag, char * line, int len=-1);
};

#endif
