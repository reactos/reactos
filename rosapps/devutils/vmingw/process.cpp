/********************************************************************
*	Module:	process.cpp. This is part of Visual-MinGW.
*
*	Purpose:	Procedures to invoke MinGW compiler.
*
*	Authors:	Manu B.
*
*	License:	Visual-MinGW is covered by GNU General Public License, 
*			Copyright (C) 2001  Manu B.
*			See license.htm for more details.
*
*	Note:		The following article from MSDN explanes how to handle Callback
*			procedures :
*						Calling All Members: Member Functions as Callbacks.
*						by Dale Rogerson.
*						Microsoft Developer Network Technology Group.
*						April 30, 1992.
*						http://msdn.microsoft.com/archive/default.asp
*
*	Revisions:	
*
********************************************************************/
#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <time.h>
#include <process.h>
#include "process.h"
#include "project.h"
#include "main.h"
#include "rsrc.h"

extern CCriticalSection CriticalSection;
extern CMessageBox MsgBox;
char errmsg[128];

// For winApp.isWinNT and winApp.Report.Append
extern CWinApp winApp;

/********************************************************************
*	Class:	CCommandDlg.
*
*	Purpose:
*
*	Revisions:	
*
********************************************************************/
CCommandDlg::CCommandDlg(){
	*cmdLine = '\0';
}

CCommandDlg::~CCommandDlg(){
}

HWND CCommandDlg::Create(void){
return CreateParam(&winApp, IDD_COMMAND, 0);
}

LRESULT CALLBACK CCommandDlg::CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam){
	switch(Message){
		case WM_INITDIALOG:
			return OnInitDialog((HWND) wParam, lParam);
		
		case WM_COMMAND:
			OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND) lParam);
			break;

		case WM_CLOSE:
			EndDlg(0); 
			break;
	}
return FALSE;
}

BOOL CCommandDlg::OnInitDialog(HWND, LPARAM){
	hCmdLine	= GetItem(IDC_CMDLINE);

	SetItemText(hCmdLine, cmdLine);
	// Show the dialog.
	Show();
return TRUE;
}

BOOL CCommandDlg::OnCommand(WORD, WORD wID, HWND){
	switch (wID){
		case IDOK:
			GetItemText(hCmdLine, cmdLine, sizeof(cmdLine));
			//MsgBox.DisplayString(cmdLine);
			winApp.Process.CommandLine(cmdLine);
		return TRUE;

		case IDCANCEL:
			EndDlg(IDCANCEL);
		return FALSE;
	}
return FALSE;
}


/********************************************************************
*	Class:	CTask.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CTask::CTask(){
	*cmdLine 		= '\0';
	*szFileName	= '\0';
	creationFlag 	= 0;
	outputFlag		= 0;
}

CTask::~CTask(){
}


/********************************************************************
*	Class:	CStack.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CStack::CStack(){
	retBuf = NULL;
}

CStack::~CStack(){
	DestroyList();
	if (retBuf)
		delete retBuf;
}

void CStack::DetachCurrent(void){
	// Empty list ?
	if (current != NULL){
		CNode * node = current;

		// Detach node from the list.
		if (node->next != NULL)
			node->next->prev = node->prev;
		if (node->prev != NULL)
			node->prev->next = node->next;
	
		// Set current node.
		if(node->next != NULL)
			current = node->next;
		else
			current = node->prev;

		if (current == NULL){
			// Now, the list is empty.
			first = last = NULL;

		}else if (first == node){
			// Detached node was first.
			first = current;

		}else if (last == node){
			// Detached node was last.
			last = current;
		}
		count--;
	}
}

/********************************************************************
*	Push/Pop/Flush.
********************************************************************/
int CStack::Push(CTask * newTask){
	InsertLast(newTask);
return Length();
}

CTask * CStack::Pop(void){
	// Delete return buffer.
	if (retBuf){
		delete retBuf;
		retBuf = NULL;
	}

	// Get first node. (FIFO stack)
	retBuf  = (CTask*) First();

	// The Stack is empty ?
	if (!retBuf)
		return NULL;

	// Detach current node from the list. Return a pointer to it.
	DetachCurrent();
return retBuf;
}

void CStack::Flush(void){
	DestroyList();
	if (retBuf)
		delete retBuf;
	retBuf = NULL;
}


/********************************************************************
*	Class:	CPipes.
*
*	Purpose:	Creates needed pipes, depending on creationFlag. 
*		Like GNU Make does, we use an Handle array for our pipes.
*		Parent Process Side is stdXXX[0] and Child Process Side is stdXXX[1].
*
*		Ex:	PARENT ->[0]IN_PIPE[1]-> 	CHILD_IO	->[1]OUT_PIPE[0]-> PARENT
*										->[1]ERR_PIPE[0]-> PARENT
*	Revisions:	
*
********************************************************************/
CPipes::CPipes(){
	hIn[0] 	= NULL;
	hIn[1] 	= NULL;
	hOut[0] 	= NULL;
	hOut[1] 	= NULL;
	hErr[0] 	= NULL;
	hErr[1] 	= NULL;
}

CPipes::~CPipes(){
}

bool CPipes::Create(WORD creationFlag, bool winNT){
	/* Create needed pipes according to creationFlag */
	/* Parent side of pipes is [0], child side is [1] */
	HANDLE hDup;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	
	if (winNT){
		/* Create a security descriptor for Windows NT */
		SECURITY_DESCRIPTOR sd;
		if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)){
			sprintf(errmsg, "vm error: Process.cpp InitializeSecurityDescriptor(winNT) failed (e=%d)", (int)GetLastError());
			winApp.Report.Append(errmsg, LVOUT_ERROR);
			return false;
		}
		if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE)){
			sprintf(errmsg, "vm error: Process.cpp SetSecurityDescriptorDacl(winNT) failed (e=%d)", (int)GetLastError());
			winApp.Report.Append(errmsg, LVOUT_ERROR);
			return false;
		}
		sa.lpSecurityDescriptor = &sd;
	}

	/* Input pipe */
	if (!CreatePipe(&hIn[1], &hIn[0], &sa, 0)){
		sprintf(errmsg, "vm error: Process.cpp CreatePipe(In) failed (e=%d)", (int)GetLastError());
		winApp.Report.Append(errmsg, LVOUT_ERROR);
		return false;
	}

	if (!DuplicateHandle(GetCurrentProcess(),
		      hIn[0],
		      GetCurrentProcess(),
		      &hDup,
		      0,
		      FALSE,
		      DUPLICATE_SAME_ACCESS)){
		sprintf(errmsg, "vm error: Process.cpp DuplicateHandle(In) failed (e=%d)", (int)GetLastError());
		winApp.Report.Append(errmsg, LVOUT_ERROR);
		return false;
	}
	CloseHandle(hIn[0]);
	hIn[0] = hDup;

	/* Output pipe */
	if (!CreatePipe(&hOut[0], &hOut[1], &sa, 0)){
		sprintf(errmsg, "vm error: Process.cpp CreatePipe(Out) failed (e=%d)", (int)GetLastError());
		winApp.Report.Append(errmsg, LVOUT_ERROR);
		return false;
	}

	if (!DuplicateHandle(GetCurrentProcess(),
		      hOut[0],
		      GetCurrentProcess(),
		      &hDup,
		      0,
		      FALSE,
		      DUPLICATE_SAME_ACCESS)){
		sprintf(errmsg, "vm error: Process.cpp DuplicateHandle(Out) failed (e=%d)", (int)GetLastError());
		winApp.Report.Append(errmsg, LVOUT_ERROR);
		return false;
	}
	CloseHandle(hOut[0]);
	hOut[0] = hDup;

	/* Error pipe */
	if (!(creationFlag & OUTERR_PIPE) && (creationFlag & ERR_PIPE)){
		if (!CreatePipe(&hErr[0], &hErr[1], &sa, 0)){
			sprintf(errmsg, "vm error: Process.cpp CreatePipe(Err) failed (e=%d)", (int)GetLastError());
			winApp.Report.Append(errmsg, LVOUT_ERROR);
			return false;
		}

		if (!DuplicateHandle(GetCurrentProcess(),
			      hErr[0],
			      GetCurrentProcess(),
			      &hDup,
			      0,
			      FALSE,
			      DUPLICATE_SAME_ACCESS)){
			sprintf(errmsg, "vm error: Process.cpp DuplicateHandle(Err) failed (e=%d)", (int)GetLastError());
			winApp.Report.Append(errmsg, LVOUT_ERROR);
			return false;
		}
		CloseHandle(hErr[0]);
		hErr[0] = hDup;
	}
return true;
}

bool CPipes::CloseChildSide(void){
return Close(1);
}

bool CPipes::CloseParentSide(void){
return Close(0);
}

bool CPipes::Close(int side){

	if (side < 0 || side > 1)
		return false;

	if (hIn[side]){
		CloseHandle(hIn[side]);
		hIn[side] = NULL;
	}

	if (hOut[side]){
		CloseHandle(hOut[side]);
		hOut[side] = NULL;
	}

	if (hErr[side]){
		CloseHandle(hErr[side]);
		hErr[side] = NULL;
	}
return true;
}


/********************************************************************
*	Class:	CProcess.
*
*	Purpose:	
*
*	Revisions:	
*
********************************************************************/
CProcess::CProcess(){
	Running = false;
	exitCode = 0;

	pi.hProcess		= 0; 
	pi.hThread		= 0; 
	pi.dwProcessId	= 0; 
	pi.dwThreadId	= 0; 
}

CProcess::~CProcess(){
}

/********************************************************************
*	Manage Tasks.
********************************************************************/
bool CProcess::isRunning(void){
	if (Running){
		MsgBox.DisplayWarning("A process is already running !");
		return true;
	}
return false;
}

CTask * CProcess::AddTask(char * cmdLine, WORD creationFlag, WORD outputFlag){
	CTask * newTask = new CTask;

	strcpy(newTask->cmdLine, cmdLine);
	newTask->creationFlag = creationFlag;
	newTask->outputFlag = outputFlag;
	Push(newTask);
return newTask;
}

bool CProcess::CmdCat(char * cmdLine){
	CTask * task = (CTask*) GetCurrent();
	if (!task)
		return false;

	strcat(task->cmdLine, cmdLine);
return true;
}

/********************************************************************
*	RunNext/Run/RunProcess.
********************************************************************/
void __cdecl call_thread(void * ptr){
	/* C++ adapter */
	((CProcess *) ptr)->Run_Thread_Internal();
}

void CProcess::Run(void){
	// Check if something is already running before creating a thread.
	if (!Running){
		// Call Run_Thread_Internal()
		_beginthread(call_thread, 1024 * 1024, (void *) this);
	}
}

void CProcess::Run_Thread_Internal(void){
	exitCode = 0;
	/* Execute each task */
	for ( ; ; ){
		/* If previous task returns an error code, abort */
		if (exitCode != 0)
			break;
	
		// Get one task to execute.
		currTask = Pop();
	
		// Nothing to run.
		if (!currTask)
			break;

		/* Show command lines ?*/
		winApp.Report.Append(currTask->cmdLine, LVOUT_NORMAL);

		if (RunProcess(currTask)){
			winApp.Report.Append("Abort !", LVOUT_NORMAL);
			exitCode = 1;
			break;
		}
	}

	// Successful ?
	if (exitCode == 0)
		winApp.Report.Append("Performed successfully.", LVOUT_NORMAL);

	Flush();
	Running = false;
return;
}

bool CProcess::RunProcess(CTask * task){
	if (!task)
		return false;

	bool usePipes = task->creationFlag;
	STARTUPINFO si	= {sizeof(STARTUPINFO), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
					0, 0, 0, 0, 0};

	/* PROCESS_INFORMATION */
	pi.hProcess		= 0; 
	pi.hThread		= 0; 
	pi.dwProcessId	= 0; 
	pi.dwThreadId	= 0; 

	/* Process creation with pipes */
	if (usePipes){
		/* Create needed pipes according to creationFlag */
		if(!Pipes.Create(task->creationFlag, winApp.isWinNT)){
			Pipes.CloseChildSide();
			Pipes.CloseParentSide();
			return false;
		}
	
		si.dwFlags 		= STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		si.wShowWindow	= SW_HIDE;
		//si.wShowWindow	= SW_SHOWNORMAL;

		/* Set pipe handles */
		if (Pipes.hIn[1] != NULL && Pipes.hOut[1] != NULL){
			si.hStdInput = Pipes.hIn[1];
			si.hStdOutput = Pipes.hOut[1];
			if (Pipes.hErr[1] == NULL)
				si.hStdError = Pipes.hOut[1];
			else
				si.hStdError = Pipes.hErr[1];
		}else{
			sprintf(errmsg, "vm error: Process.cpp Invalid pipe handle");
			winApp.Report.Append(errmsg, LVOUT_ERROR);
			Pipes.CloseChildSide();
			Pipes.CloseParentSide();
			return false;
		}
	}
		
	/* Create the child process */
	Running = CreateProcess(NULL, 
			task->cmdLine, 
			NULL, 
			NULL, 
			usePipes, 
			0, 
			NULL,
			/*startDir[0] ? startDir :*/ NULL, 
			&si, 
			&pi);

	if (!Running){
		/* CreateProcess failed. Close handles and return */
		Pipes.CloseChildSide();
		Pipes.CloseParentSide();
		sprintf(errmsg, "vm error: Process.cpp CreateProcess failed (e=%d)", (int)GetLastError());
		winApp.Report.Append(errmsg, LVOUT_ERROR);
		return false;
	}else{
		/* Close child process handles */
		Pipes.CloseChildSide();

		if (!(usePipes & IN_PIPE)){
			/* Don't use the Input pipe */
			::CloseHandle(Pipes.hIn[0]);
			Pipes.hIn[0] = NULL;
		}
	}

	//sprintf(errmsg, "vm debug: enter io loop");
	//winApp.Report.Append(errmsg, LVOUT_ERROR);
	if (usePipes){
		/* Initialize buffers */
		*outBuf = 0;
		chr = outBuf;
		bool bResult;
		for ( ; ; ){
			Sleep(100L);
	
			bResult = ReadStdOut(task, Pipes.hOut[0]);
			if (bResult != NO_ERROR)
				break;

			::GetExitCodeProcess(pi.hProcess, &exitCode);
			if (exitCode != STILL_ACTIVE){
				break;
			}
		}
	}
	//sprintf(errmsg, "vm debug: exit io loop");
	//winApp.Report.Append(errmsg, LVOUT_ERROR);

	/* The child process is running. Perform I/O until terminated */
	::WaitForSingleObject(pi.hProcess, INFINITE);
	/* Process terminated. Get exit code. */
	::GetExitCodeProcess(pi.hProcess, &exitCode);
		if (exitCode == NO_ERROR){
			return NO_ERROR;
		}
	/* Close handles */
	Pipes.CloseParentSide();
	::CloseHandle(pi.hProcess);
	if (pi.hThread){
		::CloseHandle(pi.hThread);
		pi.hThread = NULL;
	}
return exitCode;
}

/********************************************************************
*	Pipes input/output.
********************************************************************/
void CProcess::WriteStdIn(HANDLE hPipe, WORD){
	if (!hPipe)
		return;

return;
}

void CProcess::ReadStdErr(HANDLE hPipe, WORD){
	if (!hPipe)
		return;

return;
}

long CProcess::ReadStdOut(CTask * task, HANDLE hPipe){
	if (!task || !hPipe)
		return ERROR_INVALID_FUNCTION;

	/* Copy each char and output lines while there is something to read */
	for ( ; ; ){
		// Copy one char, return if nothing available.
		if (!ReadOneChar(hPipe, chr))
			break;

		// Ignore CR.
		if (*chr == '\r')
			continue;

		if (*chr != '\n'){
			chr++;
			/* @@TODO Overflow 
			if ((chr - outBuf) >= max_len)
				realloc(buffer);*/
		// End of line
		}else if (*chr =='\n'){
			*chr = '\0';
			// Output error lines to List View.
			if (task->outputFlag == STDOUT_FILE_APPEND){
				WriteFileAppend(task->szFileName, outBuf, (chr - outBuf));
			}else{
				OutputLine(task->outputFlag, outBuf, (chr - outBuf));
			}
			*outBuf = '\0';
			chr = outBuf;
		}
	}
return NO_ERROR;
}	

int CProcess::ReadOneChar(HANDLE hPipe, char * chrin){
	DWORD bytesRead = 0;
	DWORD bytesAvail = 0;

	if (!PeekNamedPipe(hPipe, chrin, (DWORD)1, &bytesRead, &bytesAvail, NULL))
		return 0;

	if (bytesAvail == 0)
		return 0;

	if (!ReadFile(hPipe, chrin, (DWORD)1, &bytesRead, NULL))
		return 0;

return bytesRead;
}

bool CProcess::CommandLine(char * cmdLine){
	if (!Pipes.hIn[0])
		return false;
	if (!Running || !currTask || !currTask->creationFlag)
		return false;
	int len = strlen(cmdLine);
	if (len){
		strcpy(inBuf, cmdLine);
		char * s = inBuf;
		s+=len;
		*s = '\r';
		s++;
		*s = '\n';
		s++;
		*s = '\0';
	}
	DWORD written;
	
	if (!WriteFile(Pipes.hIn[0], inBuf, strlen(inBuf), &written, 0))
		return false;
	
return true;
}

bool CProcess::WriteFileAppend(char * fileName, char * line, int /*len*/){
	if (!*fileName)
		return false;

	/* Append one line of text to a file */
	FILE * file = fopen(fileName, "a");
	if (file){
		fprintf(file, line);
		fprintf(file, "\n");
		fclose(file);
		return true;
	}
return false;
}

bool CProcess::OutputLine(WORD outputFlag, char * line, int /*len*/){
	/* Output error lines to List View */

	CriticalSection.Enter();
	winApp.Report.Append(line, outputFlag);
	CriticalSection.Leave();
return true;
}

