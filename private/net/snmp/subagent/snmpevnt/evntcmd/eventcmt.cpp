//****************************************************************************
//
//  Copyright (c) 1996, Microsoft Corporation
//
//  File:  EVENTCMT.CPP
//
//  Implementation of the EventConfigModifier class.
//
//  Author: Nadir Ahmed (nadira@microsoft.com)
//
//  History:
//
//      nadira   03/20/96  Created.
//
//****************************************************************************


//	Public Includes
//	===============


//	Private includes
//	================

#include "stdafx.h"

#include "resource.h"
#include <eventcmd.h>
#include <eventcmt.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


//	Defines
//	=======
#define EVENTCMT_LINE_LENGTH 500


//  Moved to eventcmt.h.
//  ====================

//  #define EVENTCMT_SYSTEM_MODE 0
//  #define EVENTCMT_CUSTOM_MODE 1
//  #define EVENTCMT_REQUST_MODE 2


//	Globals
//	=======



//============================================================================
//  EventConfigModifier::EventConfigModifier
//
//  This is the EventConfigModifier class's only constructor. This class checks
//	the command arguments, connects to and opens the target machines registry.
//	Handles writing to the eventcmt.log. Reads in the configuration file, builds
//	lists of configuration items to be added/deleted and processes these lists.
//	Starts and stops the SNMP service if necessary and generates a status mif.
//
//
//  Parameters:
//
//      LPCSTR CmdLine  	This is a space separated list of arguments.
//
//      LPCSTR machine      The machine name whose configuration is to be
//							modified.
//
//  Returns:
//
//      none
//
//============================================================================

EventConfigModifier::EventConfigModifier(LPCSTR CmdLine, LPCSTR machine)
{
	nl.LoadString(IDS_NL);
	tab.LoadString(IDS_TAB);
#ifdef EVENTCMT_OLD_LOG
	NL.LoadString(IDS_LFCR);
#else	//EVENTCMT_OLD_LOG
	NL = nl;
#endif	//EVENTCMT_OLD_LOG

	if(CmdLine)
		CommandLine = CmdLine;

	if (machine)
		Machine = machine;
	
//	This class variable's condition indicates whether an ERROR status mif has been written
//	======================================================================================

	PrevStatus = TRUE;


//	This class variable's condition indicates whether the SNMP config has been modified
//	===================================================================================

	SNMPModified = FALSE;


//	This class variable's condition indicates whether help needs to be printed
//	==========================================================================

	Printhelp = FALSE;
}


//============================================================================
//  EventConfigModifier::~EventConfigModifier
//
//  This is the EventConfigModifier class's only destructor.
//
//
//  Parameters:
//
//		none
//
//  Returns:
//
//      none
//
//============================================================================

EventConfigModifier::~EventConfigModifier()
{
}


//============================================================================
//  EventConfigModifier::SetError
//
//  This private method sets the error code that the Main() method returns.
//
//
//  Parameters:
//
//		DWORD val		The error code to be set. If this has already been set
//						return.
//
//  Returns:
//
//      none
//
//============================================================================

void EventConfigModifier::SetError(DWORD val)
{
	//	Check the current error state. Only add the code if it is new.
	//	==============================================================

	if(EvCmtReturnCode & val)
		return;
	else
		EvCmtReturnCode += val;
}


//============================================================================
//  EventConfigModifier::ProcessCommandLine
//
//  This private method processes the command line that was passed to the class
//	constructor as the first parameter. The command line may have several args
//	which define how this class will modify the configuration. The first argumemt
//	is compulsory and is always the name of the configuration file. If this is
//	missing this function will return false. Any following arguments are command
//	switches. Case is ignored for the switches and the those currently supported
//	are...
//	/NOMIF			- no mif file will be generated
//	/NOLOG 			- the log file will remain unchanged
//	/DEFAULT		- only run if the current config has default settings
//	/SETCUSTOM		- set this config to have custom settings
//	/NOSTOPSTART	- do not stop and start the snmp service
//
//
//  Parameters:
//
//		none
//
//  Returns:
//
//      BOOL		True if there were no errors, False otherwise
//
//============================================================================

BOOL EventConfigModifier::ProcessCommandLine()
{

	//	return FALSE if there is no command line
	//	========================================

	if (!CommandLine.GetLength())
	{
		SetError(EVCMT_BAD_ARGS);
		return FALSE;
	}

	LONG Result;
	
	if (Machine.GetLength())
	{
		Result = RegConnectRegistry(Machine.GetBuffer(1), HKEY_LOCAL_MACHINE, &hkey_machine);
		Machine.ReleaseBuffer();
	}
	else
		Result = RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hkey_machine);

	if (Result != ERROR_SUCCESS)
	{
		StatusMif(IDS_NO_REG_CNT, FALSE);
		SetError(EVCMT_REG_CNNCT_FAILED);
		return FALSE;
	}

	BOOL validArgs = TRUE;
	LogWanted = TRUE;
	StatusMifWritten = FALSE;
	MandatoryMode = TRUE;
	SetCustom = FALSE;
	SNMPStopStart = TRUE;
	CString tempbuff(CommandLine);
	CString * next = CStringStrtok(&tempbuff);
	CString help;
	help.LoadString(IDS_ARG_HELP);
	CString helph;
	helph.LoadString(IDS_ARG_HELPH);
	CString isHelp(*next);
	isHelp.MakeUpper();

	if ((isHelp == helph) || (isHelp == help))
	{
		Printhelp = TRUE;
	}
	else
	{
		//	return FALSE if there is no file specified
		//	==========================================

		if (next)
		{
			CommandFile = *next;	//set the config file.
			delete next;
		}
		else
		{
			RegCloseKey(hkey_machine);
			return FALSE;
		}

		next = CStringStrtok(&tempbuff);

		UniqueList commandargs; 
							

		//	get all the other arguments into a list for processing
		//	======================================================

		while (next)
		{
			//check miff
			ListItem * tempL = new ListItem(next);

			if(commandargs.Add(tempL))
				delete tempL;

			next = CStringStrtok(&tempbuff);
		}


		//	load all the valid command args
		//	===============================

		CString NoMif;
		CString NoLog;
		CString Mode;
		CString Custom;
		CString noSNMPstopstart;
		NoMif.LoadString(IDS_ARG_NOMIF);
		Mode.LoadString(IDS_ARG_MODE);
		NoLog.LoadString(IDS_ARG_NOLOG);
		Custom.LoadString(IDS_ARG_CUSTOM);
		noSNMPstopstart.LoadString(IDS_ARG_SNMP);


		//	process all the command args
		//	============================

		while (!commandargs.IsEmpty())
		{
			ListItem * item = commandargs.RemoveHead();
			CString * tmp = item->GetString();
			tmp->MakeUpper();

			if (*tmp == NoMif)
			{
				StatusMifWritten = TRUE;	//no status mif so indicate that one has been written
				PrevStatus = FALSE;			//an error mif so another will not be written
			}
			else if (*tmp == NoLog)
				LogWanted = FALSE; 			//no information to be added to the log file

			else if (*tmp == Mode)
				MandatoryMode = FALSE;		//only run if we have a DEFAULT config.
								
			else if (*tmp == Custom)		
				SetCustom = TRUE;			//set our config to be CUSTOM

			else if (*tmp == noSNMPstopstart)
				SNMPStopStart = FALSE;		//do not stop and start the SNMP service

			else if ((*tmp == help) || (*tmp == helph))
				Printhelp = TRUE;			//we're gonna print help!

			else
			{
				validArgs = FALSE;			//invalid arg specified
			}

			delete item;
		}

		if (Printhelp)
		{
			validArgs = TRUE;
			StatusMifWritten = TRUE;	//no staus mif if we print help
		}
		else if (!validArgs)
		{
			StatusMifWritten = FALSE;	//want a status mif generated if there was an error above!
			PrevStatus = TRUE;			//no error mif has been written yet.
			RegCloseKey(hkey_machine);
			LogWanted = TRUE;
			SetError(EVCMT_BAD_ARGS);
		}

#ifndef EVENTCMT_OLD_LOG
/*
        if (LogWanted)
		{
			CString logname;
			logname.LoadString(IDS_LOG_FILE);
			
			if (ERROR_SUCCESS != SMSCliLogInitialize(logname))
				LogWanted = FALSE;
		}
*/
#endif EVENTCMT_OLD_LOG

	}

	return validArgs;
}


//============================================================================
//  EventConfigModifier::ReadLineIn
//
//  This private method is used to read from a certain posn to the end of a
//	line from a file. 
//
//
//  Parameters:
//
//      HANDLE * h_file		a pointer to the handle of the file to read from
//
//		CString * Line		a pointer to the CString to accept the text read
//
//		DWORD * startpoint	a ponter to the starting point for the read.
//							when the procedure exits this will be the point
//							in the file where reading stopped i.e. start of a
//							new line.
//
//  Returns:
//
//      BOOL		True if there were no errors, False otherwise
//
//============================================================================

BOOL EventConfigModifier::ReadLineIn(HANDLE * h_file, CString * Line, DWORD * startpoint)
{
	Line->Empty();			//make sure the CString we're writing to is empty
	TCHAR buff[EVENTCMT_LINE_LENGTH +1];	//a buffer to read into
	DWORD bytesRead;					//the number of bytes read from the file
	OVERLAPPED overlapped;
	overlapped.OffsetHigh = 0;
	overlapped.hEvent = NULL;


	//	This next loop will run until we read a newline or an EOF
	//	=========================================================
		
	while(TRUE)
	{
		memset(buff, 0, sizeof(TCHAR)*(EVENTCMT_LINE_LENGTH +1));
		overlapped.Offset = *startpoint; //set the start point for the next read
		int x = 0;
		bytesRead = 0;
		

		//	Do the actual read from the file
		//	================================

		if (!ReadFile(*h_file, &buff, EVENTCMT_LINE_LENGTH, &bytesRead, &overlapped))
		{			
			DWORD e = GetLastError();
			return FALSE;
		}
		
		if (!bytesRead)
			return FALSE;

		CString tmp(buff);
		CString linefeed;
		linefeed.LoadString(IDS_LINEFEED);


		//	This loop will see if we have read a newline if yes, build the CString
		//	======================================================================

		while(x < tmp.GetLength())
		{
			if(tmp.GetAt(x) == nl.GetAt(0))	//we've found a newline
			{
				*startpoint = *startpoint + x + 1;	//adjust startpoint for the next read

				for(int i = 0; i < x - 1; i++)
					*Line += tmp.GetAt(i);
				
				if(tmp.GetAt(x-1) != linefeed.GetAt(0)) //don't want char before \n if it is \r
					*Line += tmp.GetAt(x-1);

				return TRUE;
			}
			x++;
		}


		//	We haven't got a newline so set things up for the next read from file
		//	=====================================================================
		
		*Line += tmp;							//add the text to our CString
		*startpoint = *startpoint + bytesRead;	//adjust the startpoint for the next read.

		if (bytesRead < EVENTCMT_LINE_LENGTH)	//we must have hit the end of file so return
			return TRUE;
	}
}


//============================================================================
//  EventConfigModifier::ReadConfigIn
//
//  This private method is used to read the configuration file and build the
//	the two (traps dests and translator config) lists of configuration updates
//	required. 
//
//
//  Parameters:
//
//      none
//
//  Returns:
//
//      none
//
//============================================================================

void EventConfigModifier::ReadConfigIn()
{
	CString buff;
	CString CBuff;

	
	//	Open the configuration file for read
	//	====================================

	HANDLE hfile = CreateFile(CommandFile,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
	
	if (hfile == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
		StatusMif(IDS_IFILE, FALSE);
		SetError(EVCMT_BAD_INPUT_FILE);
		return;
	}
	else // we have an open file
	{
		CString * next;
		CString msg1str;
		msg1str += NL;
		msg1str += NL;
		CString addmsg;
		addmsg.LoadString(IDS_MSG24);
		msg1str += addmsg;
		msg1str += NL;
		msg1str += NL;
		WriteToLog(&msg1str);
		int badlines = 0;
		int goodlines = 0;
		DWORD Offset = 0;

		
		//	this loop reads and processes the file line by line
		//	===================================================

		while (ReadLineIn(&hfile, &buff, &Offset))
		{
			CString comment;
			comment.LoadString(IDS_COMMENT);
			CBuff = buff;
			CString wholebuff(buff);

			if (!CBuff.GetLength())
				continue; //blank line

			// Is it a comment, if it is just skip it. 
			if (CBuff.GetAt(0) == comment.GetAt(0))
			{
				CString msg;
				msg.LoadString(IDS_MSG17);
				wholebuff += msg;
				wholebuff += NL;
				WriteToLog(&wholebuff);
				continue;
			}

			next = CStringStrtok (&CBuff);	//get the first token from the line
			CString prag;
			prag.LoadString(IDS_PRAGMA);
			if (next && (*next != prag))	//first token not #pragma - log an error
			{ 
				if (next->GetAt(0) != comment.GetAt(0))	//check it isn't a comment.
				{
					CString msg;
					msg.LoadString(IDS_MSG18);
					wholebuff += msg;
					wholebuff += NL;
					badlines++;
					SetError(EVCMT_INVALID_COMMAND);
					StatusMif(IDS_SYNTAX_ERROR, FALSE);
				}
				else		//it is a comment, say so in the log
				{
					CString msg;
					msg.LoadString(IDS_MSG17);
					wholebuff += msg;
					wholebuff += NL;
				}

				WriteToLog(&wholebuff);
				delete next;
				continue;	//skip to the next line in the file	
			}
			
			delete next;
			next = CStringStrtok (&CBuff);	//get the next token from the string
			
			if (next)						//this should be the command type
			{
				next->MakeUpper();
				CString AddCom;
				AddCom.LoadString(IDS_ADD);			//add
				CString DelCom;
				DelCom.LoadString(IDS_DEL);			//delete
				CString AddTCom;
				AddTCom.LoadString(IDS_ADDTRAP);	//add_trap_dest
				CString DelTCom;
				DelTCom.LoadString(IDS_DELTRAP);	//delete_trap_dest
				CommandItem * com;
				TrapCommandItem * tcom;

				if (*next == AddCom)							//add translation config
				{
					com = GetCommandArgs(&wholebuff, &CBuff);	//get the rest of the command args.
					
					if (com)	//GetCommandArgs succeded, we have a command to process
					{
						com->SetCommand(Add);	//set the command type
						CommandQ.Add(com);		//add it to the list
						goodlines++;			//increment the numer of good commands found
					}
					else		//GetCommandArgs failed, bad args - set the error and mif
					{
						badlines++;			//increment the numer of bad commands found
						SetError(EVCMT_INVALID_COMMAND);
						StatusMif(IDS_SYNTAX_ERROR, FALSE);
						delete next;
						continue;
					}
				}
				else if (*next == DelCom)	//delete translation config 
				{
					com = GetCommandArgs(&wholebuff, &CBuff); 
					
					if (com)	//GetCommandArgs succeded, we have a command to process
					{
						com->SetCommand(Delete);	//set the command type
						CommandQ.Add(com);			//add it to the list
						goodlines++;				//increment the numer of good commands found
					}
					else		//GetCommandArgs failed, bad args - set the error and mif
					{
						badlines++;			//increment the numer of bad commands found
						SetError(EVCMT_INVALID_COMMAND);
						StatusMif(IDS_SYNTAX_ERROR, FALSE);
						delete next;
						continue;
					}
				}
				else if (*next == AddTCom)	//add trap config
				{
					if(!SNMPinstalled)		//no snmp, set error and mif
					{
						CString msg;
						msg.LoadString(IDS_MSG50);
						wholebuff += msg;
						wholebuff += NL;
						WriteToLog(&wholebuff);
						badlines++;
						StatusMif(IDS_INVALID_SNMP, FALSE);
						delete next;
						continue;
					}

					tcom = GetTrapCommandArgs(&wholebuff, &CBuff); 	//get the rest of the arguments
					
					if (tcom)		//GetTrapCommandArgs succeded, something to process
					{
						tcom->SetCommand(AddTrap);	//set the command type
						TrapQ.Add(tcom);			//add it to the list
						goodlines++;
					}
					else			//GetTrapCommandArgs failed set error and mif
					{
						badlines++;
						SetError(EVCMT_INVALID_COMMAND);
						StatusMif(IDS_SYNTAX_ERROR, FALSE);
						delete next;
						continue;
					}
				}
				else if (*next == DelTCom)	//add trap config
				{
					if(!SNMPinstalled)		//no snmp, set error and mif
					{
						CString msg;
						msg.LoadString(IDS_MSG50);
						wholebuff += msg;
						wholebuff += NL;
						WriteToLog(&wholebuff);
						badlines++;
						StatusMif(IDS_INVALID_SNMP, FALSE);
						delete next;
						continue;
					}

					tcom = GetTrapCommandArgs(&wholebuff, &CBuff);	//get the rest of the arguments 
					
					if (tcom)		//GetTrapCommandArgs succeded, something to process
					{
						tcom->SetCommand(DeleteTrap);	//set the command type
						TrapQ.Add(tcom);				//add it to the list
						goodlines++;
					}
					else 			//GetTrapCommandArgs failed set error and mif
					{
						badlines++;
						SetError(EVCMT_INVALID_COMMAND);
						StatusMif(IDS_SYNTAX_ERROR, FALSE);
						delete next;
						continue;
					}
				}
				else	//we don't have a valid command type set error and mif
				{
					CString msg;
					msg.LoadString(IDS_MSG19);
					wholebuff += msg;
					wholebuff += NL;
					WriteToLog(&wholebuff);
					badlines++;
					SetError(EVCMT_INVALID_COMMAND);
					StatusMif(IDS_SYNTAX_ERROR, FALSE);
					delete next;
					continue;	
				}
			
				delete next;

			} 
		}


		//	Finished reading the file report this to the log
		//	================================================

		CString msg2;
		msg2 += NL;
		msg2 += NL;
		CString read;
		read.LoadString(IDS_MSG20);
		msg2 +=	read;
		msg2 += NL;
		char num[34];
		_ultoa(badlines, num, 10); 
		msg2 += num;
		read.Empty();
		read.LoadString(IDS_MSG21);
		msg2 += read;
		msg2 += NL;
		_ultoa(goodlines, num, 10); 
		msg2 += num;
		read.Empty();
		read.LoadString(IDS_MSG22);
		msg2 += read;
		msg2 += NL;
		msg2 += NL;
		WriteToLog(&msg2);
		CloseHandle(hfile);
	}
}


//============================================================================
//  EventConfigModifier::GetTrapCommandArgs
//
//  This private method is used to read the arguments specified in a line of
//	text supplied as a parameter. If there is an error it returns NULL and
//	writes to the log. If it succedes it returns a TrapCommandItem.
//
//
//  Parameters:
//
//      CString * buffer		A pointer to the CString which is the complete
//								command line used to write to the log.
//
//		CString * comline		A pointer to the CString which is the partial
//								command line containing the arguments to be
//								read.
//
//  Returns:
//
//      TrapCommandItem *		A pointer to a TrapCommandItem. The arguments
//								stripped from the input line and inserted into
//								this class. This is NULL if there is an error. 
//
//============================================================================

TrapCommandItem * EventConfigModifier::GetTrapCommandArgs(CString * buffer, CString * comline)
{
	TrapCommandItem * newCom = NULL;
	CString * comm;
	CString * addr;
	comm = MyStrtok(comline);


	if(!comm)	//error no community name
	{
		CString add;
		add.LoadString(IDS_MSG23);
		*buffer += add;
		*buffer += NL;
		WriteToLog(buffer);
		return newCom;
	}
				
	addr = MyStrtok(comline);
	
	if(!addr)	//error no address specified
	{
		delete comm;

		CString add;
		add.LoadString(IDS_MSG25);
		*buffer += add;
		*buffer += NL;
		WriteToLog(buffer);
		return newCom;
	}

	CString * extra = CStringStrtok(comline);

	if(extra)	//error too many arguments
	{
		delete comm;
		delete addr;
		delete extra;

		CString add;
		add.LoadString(IDS_MSG26);
		*buffer += add;
		*buffer += NL;
		WriteToLog(buffer);
		return newCom;
	}
	

	//	If we got this far we've got something to process create the TrapCommandItem
	//	============================================================================

	newCom = new TrapCommandItem(AddTrap, //just for a default value
								comm, addr, hkey_machine);

	CString add;
	add.LoadString(IDS_MSG27);
	*buffer += add;
	*buffer += NL;
	WriteToLog(buffer);

 	return newCom;
}


//============================================================================
//  EventConfigModifier::CStringStrtok
//
//  This private method is used to read a space separated token in a line of
//	text supplied as a parameter. If there is an error it returns NULL. If it
//	succedes it returns a pointer to the CString token. (A bit like strtok).
//
//
//  Parameters:
//
//      CString * In		A pointer to the CString which is to be used to
//							be read from. This CString is MODIFIED by this
//							function. This CString has the leading token from
//							it and any leading whitespace removed from it.
//
//
//  Returns:
//
//      CString *			A pointer to the CString containing the token. This
//							is NULL if there is an error or an empty string input.
//
//============================================================================

CString * EventConfigModifier::CStringStrtok(CString * In)
{
	CString * ret = NULL;
    BOOL    more;
    int     i;

	if (!In)		//no input specified
		return ret;

	In->TrimLeft();	//remove leading whitespace

	if (!In->GetLength())	//if after removing whitespace there is no length return
		return ret;

	CString wSpace;
	wSpace.LoadString(IDS_SPACE);


	//	The outer for loop steps through the input string character by character
	//	The inner for loop steps through all possible whitespace characters
	//	========================================================================

    more = TRUE;
	for (i = 0; i < In->GetLength(); i++)
	{
		for (int j = 0; more && j < wSpace.GetLength(); j++)
		{
            if (In->GetAt(i) == wSpace.GetAt(j))
                more = FALSE;
		}
        if (!more)
            break;
	}

	ret = new CString;
	
	//	If we get here we have a valid token within the input string
	//	Copy the string upto this point to the string that we return
	//	============================================================

	for (int k = 0; k < i; k++)
	{
		*ret += In->GetAt(k);
		In->SetAt(k,tab.GetAt(0));
	}

	In->TrimLeft();	//remove leading whitespace from the input string
	return ret;

}


//============================================================================
//  EventConfigModifier::MyStrtok
//
//  This private method is used to read a space/quote separated token in a line
//	of text supplied as a parameter. If there is an error it returns NULL. If
//	it succedes it returns a pointer to the CString token. (A bit like strtok).
//
//
//  Parameters:
//
//      CString * In		A pointer to the CString which is to be used to
//							be read from. This CString is MODIFIED by this
//							function. This CString has the leading token from
//							it and any leading whitespace removed from it.
//
//
//  Returns:
//
//      CString *			A pointer to the CString containing the token. This
//							is NULL if there is an error or an empty string input.
//
//============================================================================

CString * EventConfigModifier::MyStrtok(CString * In)
{
	CString * ret = CStringStrtok(In);

	if (!ret)	//nothing (maybe whitespace) in input string
		return ret;

	CString temp(*ret);
	CString space;
	space.LoadString(IDS_ASPACE);
	CString quote;
	quote.LoadString(IDS_QUOTE);
	delete ret;
	ret = NULL;
	int length = temp.GetLength();


	//	This loop steps keeps getting tokens until start and end quotes match
	//	i.e. for multiple worded quoted strings e.g. "Print Manager"
	//	=====================================================================

	while ( (temp.GetAt(0) == quote.GetAt(0)) &&
				(temp.GetAt(length - 1) != quote.GetAt(0)) )
	{
		CString * extra = CStringStrtok(In); //Get the next token

		if (!extra)
		{
			ret = new CString(temp);
			return ret; //return the string with the quote!
		}

		temp += space; //the space was stripped by strtok.
		temp += *extra;
		delete extra;
		length = temp.GetLength();
	}


	//	Get rid of the quotes only if the length is greater than one - could be a single "
	//	==================================================================================

	if ((length > 1) && 
			(temp.GetAt(0) == quote.GetAt(0)) && 
			(temp.GetAt(length - 1) == quote.GetAt(0)))
	{
		temp.SetAt(0, tab.GetAt(0));
		temp.TrimLeft();
		length = temp.GetLength();
		temp.SetAt(length - 1, tab.GetAt(0));
		temp.TrimRight();
		length = temp.GetLength();
	}

	if (length)
	{
		ret = new CString(temp);	//create the return string from what we have built
	}

	return ret;
}


//============================================================================
//  EventConfigModifier::GetCommandArgs
//
//  This private method is used to read the arguments specified in a line of
//	text supplied as a parameter. If there is an error it returns NULL and
//	writes to the log. If it succedes it returns a TrapCommandItem.
//
//
//  Parameters:
//
//      CString * buffer		A pointer to the CString which is the complete
//								command line used to write to the log.
//
//		CString * comline		A pointer to the CString which is the partial
//								command line containing the arguments to be
//								read.
//
//  Returns:
//
//      CommandItem *			A pointer to a CommandItem. The arguments
//								stripped from the input line and inserted into
//								this class. This is NULL if there is an error. 
//
//============================================================================

CommandItem * EventConfigModifier::GetCommandArgs(CString * buffer, CString * comline)
{
	CommandItem * newCom = NULL;
	CString * evlog;
	CString * evsrc;
	CString * evid;
	CString * evcnt;
	CString * evtm;
	DWORD evtid = 0;
	DWORD evtcount = 0;
	DWORD evttime = 0;
	evlog = MyStrtok(comline);
	
	if(!evlog)	//no eventlog argument
	{
		CString add;
		add.LoadString(IDS_MSG28);
		*buffer += add;
		*buffer += NL;
		WriteToLog(buffer);
		return newCom;
	}
				
	evsrc = MyStrtok(comline);
	
	if(!evsrc)	//no event source argument
	{
		delete evlog;

		CString add;
		add.LoadString(IDS_MSG29);
		*buffer += add;
		*buffer += NL;
		WriteToLog(buffer);
		return newCom;
	}

	evid = MyStrtok(comline);

	if(evid)	//eventid argument has to be converted into a DWORD
	{
		BOOL badstr = StrToDword(evid, &evtid);	//convert the id into a DWORD
		delete evid;

		if (badstr)	//invalid eventid
		{
			delete evlog;
			delete evsrc;

			//Add it to the list of bad lines - invalid or missing command

			CString add;
			add.LoadString(IDS_MSG30);
			*buffer += add;
			*buffer += NL;
			WriteToLog(buffer);
			return newCom;
		}

	}
	else	//no eventid argument was specified
	{
		delete evlog;
		delete evsrc;
		
		CString add;
		add.LoadString(IDS_MSG30);
		*buffer += add;
		*buffer += NL;
		WriteToLog(buffer);
		return newCom;
	}

	//now the count if there is one
	evcnt = MyStrtok(comline);
	
	if(evcnt)
	{
		BOOL badstr = StrToDword(evcnt, &evtcount);

		if (badstr || !evtcount)
		{
			delete evlog;
			delete evsrc;
			delete evcnt;

			//Add it to the list of bad lines - invalid or missing command
			CString add;
			add.LoadString(IDS_MSG31);
			*buffer += add;
			*buffer += NL;
			WriteToLog(buffer);
			return newCom;
		}

		//now get the time if there is one specified...
		evtm = MyStrtok(comline);
	
		if (evtm)
		{
			badstr = StrToDword(evtm, &evttime);

			if (badstr || (evttime && (evtcount < 2)))
			{
				delete evlog;
				delete evsrc;
				delete evcnt;
 				delete evtm;

				//Add it to the list of bad lines - invalid or missing command
				CString add;
				add.LoadString(IDS_MSG32);
				*buffer += add;
				*buffer += NL;
				WriteToLog(buffer);
				return newCom;
			}

			// if there are more arguments, that's too many...
			CString * extra = CStringStrtok(comline);

			if(extra)
			{
				delete evlog;
				delete evsrc;
				delete evcnt;
				delete evtm;
				delete extra;

				//Add it to the list of bad lines - invalid or missing command
				CString add;
				add.LoadString(IDS_MSG26);
				*buffer += add;
				*buffer += NL;
				WriteToLog(buffer);
				return newCom;
			}
		}
		else
		{
		 	evtm = NULL;
		}
	}
	else
	{
		// if there are more arguments, that's too many...
		CString * extra = CStringStrtok(comline);

		if(extra)
		{
			delete evlog;
			delete evsrc;
			delete extra;

			//Add it to the list of bad lines - invalid or missing command
			CString add;
			add.LoadString(IDS_MSG26);
			*buffer += add;
			*buffer += NL;
			WriteToLog(buffer);
			return newCom;
		}

	 	evcnt = NULL;
	 	evtm = NULL;
	}


	//	If we get here we have a valid set of aguments so create a CommandItem
	//	======================================================================

	newCom = new CommandItem(Add, //just for a default value
								evlog, evsrc, evtid,
								evtcount, evttime, hkey_machine);


 	//	Delete the temporary storage for the count and time
	//	===================================================
 	
 	if(evcnt)
 	{
 		delete evcnt;

		if (evtm)
			delete evtm;
	}

	CString add;
	add.LoadString(IDS_MSG27);
	*buffer += add;
	*buffer += NL;
	WriteToLog(buffer);

 	return newCom;
}


//============================================================================
//  EventConfigModifier::StrToDword
//
//  This private method is used to convert a CString into a DWORD.
//
//
//  Parameters:
//
//      CString * str		A pointer to the CString which to be converted
//
//		DWORD * num			A pointer to the DWORD which will contain the
//							result.
//
//  Returns:
//
//      BOOL				A boolean indicating whether the string converted
//							had any non numeric characters. TRUE if there are.
//							
//
//============================================================================

BOOL EventConfigModifier::StrToDword(CString * str, DWORD * num) 
{
	char * str1 = str->GetBuffer(1);
	char * tmp = str1;
	BOOL badstr = FALSE;

	
	//	The following loop steps through the CString checking for non numerics
	//	======================================================================
	
	while (tmp && (*tmp != '\0'))
	{
		if ((*tmp < '0') || (*tmp > '9'))
		{
			badstr = TRUE;
			break;
		}
		tmp++;
	}
	
	if (badstr)
		*num = 0;
	else
	{
		tmp = str1;
		*num = strtoul(str1, &tmp, 10);
	}
	
	str->ReleaseBuffer();

	return badstr;
}


//============================================================================
//  EventConfigModifier::WriteToLog
//
//  This private method is used to write a CString to the log file.
//
//
//  Parameters:
//
//      CString * inbuf		A pointer to the CString which to be converted
//
//
//  Returns:
//
//      none
//							
//
//============================================================================

void EventConfigModifier::WriteToLog(CString * inbuf)
{
	if(!LogWanted)	//command switch stated no log wanted.
		return;

#ifdef EVENTCMT_OLD_LOG

	DWORD errnum = 0;
	LPTSTR buff = inbuf->GetBuffer(1);
	CMyString * inbuf2 = (CMyString *)inbuf;
	DWORD buffsz = inbuf2->GetBufferSize() - 1;
	DWORD justwritten = 0;

	if (!WriteFile(hFile,
				buff,
				buffsz,
				&justwritten,
				NULL))
	{
	 //	It didn't work get the error condition
	 //	======================================

		errnum = GetLastError();
		 
	}
		inbuf->ReleaseBuffer();

#else	//EVENTCMT_OLD_LOG

//	SMSCliLog(*inbuf);

#endif	//EVENTCMT_OLD_LOG

}


//============================================================================
//  EventConfigModifier::Main
//
//  This public method is called after constructing a EventConfigModifier.
//	This method drives the process of changing the trap destinations and the
//	configuration of which events are translated into traps.
//
//
//  Parameters:
//
//      none
//
//
//  Returns:
//
//      DWORD		An error code which can report multiple errors at once.
//					See EVENTCMT.H for the meanings.
//							
//
//============================================================================

DWORD EventConfigModifier::Main()
{
	//	Set the error code to success to start
	//	======================================

	EvCmtReturnCode = EVCMT_SUCCESS;


	//	Get the config file name and any command switches
	//	=================================================

	if (!ProcessCommandLine())
	{
		StatusMif(IDS_INVALIDARGS, FALSE);
		return EvCmtReturnCode;
	}

	if (Printhelp)
	{
		PrintHelp();
		return EvCmtReturnCode;
	}


	//	Checks to see if SNMPELEA.DLL and the SNMP service are installed
	//	(This also LOCKS our bit of the registry so nobody else may edit)
	//	================================================================= 

	if (CheckInstallations())	//locks our bit of the registry if TRUE
	{

		//	Processes the config file producing the list of trap destinations
		//	and the list of translation events to be modified in the registry
		//	=================================================================

		Load();					
		
		
		//	Process the list of translation events
		//	======================================
		
		if(!CommandQ.IsEmpty())
			ProcessCommandQ();

		//	Unlock our bit of the registry so others may edit it
		//	====================================================

		CString keyName;
	 	keyName.LoadString(IDS_LOCK_REG);
		RegDeleteKey(hkey_machine, keyName);

		
		//	Process the list of trap destinations
		//	=====================================

		if(!TrapQ.IsEmpty())
			ProcessTrapQ();

#ifdef EVENTCMT_OLD_LOG
		if(closeHandle) //to our log file
			CloseHandle(hFile);
#endif	//EVENTCMT_OLD_LOG

		StatusMif(IDS_SUCCESS, TRUE);
	}
	else	//CheckInstallations failed
		StatusMif(IDS_INSTALL, FALSE);

	
	//	Disconnect from the registry and return the error code
	//	======================================================

	RegCloseKey(hkey_machine);
	return EvCmtReturnCode;
}

void EventConfigModifier::PrintHelp()
{
	CString msg;
	msg.LoadString(IDS_HELP_MSG);
	CString title;
	title.LoadString(IDS_HELP_BOX);
	MessageBox(NULL, msg, title, MB_ICONINFORMATION|MB_OK|MB_SETFOREGROUND
				|MB_DEFAULT_DESKTOP_ONLY|MB_SYSTEMMODAL);
}

//============================================================================
//  EventConfigModifier::StatusMif
//
//  This private method is called to write a no ID status mif indicating the
//	success of the process. This method may be called many times during the
//	course of this application but, the mif written will be the FIRST mif
//	reporting a failure or if there is no failure the FIRST mif reporting an
//	event (not a serious error) which reports success.
//
//
//  Parameters:
//
//      UINT mess_id		The resource ID for the message to be written into
//							the mif.
//
//		BOOL status			This boolean indicates whether there were any
//							problems. FALSE if there were, TRUE if there were no
//							(serious) problems.
//
//
//  Returns:
//
//      none
//							
//
//============================================================================

void EventConfigModifier::StatusMif(UINT mess_id, BOOL status)
{
	//	If a status mif has been written, return if the the previous
	//	type was an error or the current call is not for an error
	//	===============================================================
	
	if(StatusMifWritten)
	{
		if(status || !PrevStatus)
			return;
	}
	else	//no previous mif, indicate there is.
	{
		StatusMifWritten = TRUE;
	}

	PrevStatus = status;	//set the status for the 'previous' mif


	//	Load up all the arguments that will be written to the mif
	//	=========================================================

	CString filename;
	CString manufacturer;
	CString product;
	CString version;
	CString locale;
	CString serno;
	CString message;

	filename.LoadString(IDS_FILENAME);
	manufacturer.LoadString(IDS_MANUFAC);
	product.LoadString(IDS_PROD);
	version.LoadString(IDS_VER);
	locale.LoadString(IDS_LOC);
	serno.LoadString(IDS_SERNO);
	message.LoadString(mess_id);
	CString statmifdll;
	statmifdll.LoadString(IDS_STATMIFDLL);

	HINSTANCE hInstLibrary;
	

	//	Declare the function we will use to write the mif
	//	=================================================

	DWORD (WINAPI *InstallStatusMIF) (const char *, const char*, const char*,
										const char*, const char *, const char *,
										const char *, BOOL);


	//	Load the dll which will write the mif for us
	//	============================================
	
	if (hInstLibrary = LoadLibrary(statmifdll))
	{
		InstallStatusMIF = (DWORD (WINAPI *)(const char *, const char*, const char*,
										const char*, const char *, const char *,
										const char *, BOOL))
										GetProcAddress(hInstLibrary, "InstallStatusMIF");
		
		if (InstallStatusMIF) //we loaded the dll and have the function we need!
		{
			
			//	Write the mif
			//	=============

			if (!InstallStatusMIF(filename, manufacturer, product, version,
									locale, serno, message, status ))
			{
#ifdef EVENTCMT_OLD_LOG
				if(closeHandle)
#endif	//EVENTCMT_OLD_LOG

				{
					//we failed to write the mif, log it.
					CString add;
					add.LoadString(IDS_MSG33);
					add += NL;
					WriteToLog(&add);
				}
			}
			else	//we wrote the mif, log it.
			{
#ifdef EVENTCMT_OLD_LOG
				if(closeHandle)
#endif	//EVENTCMT_OLD_LOG

				{
					CString add;
					add.LoadString(IDS_MSG53);
					add += NL;
					add += message;
					add += NL;
					WriteToLog(&add);
				}
			}
		}
		else	//failed to load the function we need, log it.
		{
#ifdef EVENTCMT_OLD_LOG
			if(closeHandle)
#endif	//EVENTCMT_OLD_LOG

			{
				CString add;
				add.LoadString(IDS_MSG34);
				add += NL;
				WriteToLog(&add);
			}
		}

		FreeLibrary(hInstLibrary);
	}
	else 	//failed to load the dll, log it.
	{
#ifdef EVENTCMT_OLD_LOG
		if(closeHandle)
#endif	//EVENTCMT_OLD_LOG

		{
			CString add;
			add.LoadString(IDS_MSG35);
			add += statmifdll;
			add += NL;
			WriteToLog(&add);
		}
	}								
}


//============================================================================
//  EventConfigModifier::CheckInstallations
//
//  This private method is called to check whether the SNMP service and the
//	SNMPELEA.DLL are installed. If the SNMP service is not installed the
//	application will still run. If the SNMPELEA.DLL is not installed, this will
//	return FALSE. This function also creates a volatile reg key to indicate it
//	is editing our bit of the registry, this program will return FALSE if this
//	already present or it cannot be created. This function also check and if
//	necessary changes the config mode. the current supported modes are:
//	
//	EVENTCMT_SYSTEM_MODE	Client machine is using the site-wide event sources
//	EVENTCMT_CUSTOM_MODE	Client machine is using a custom set of event sources
//	EVENTCMT_REQUST_MODE	Client machine was previously using a custom set of
//  						event sources but is now waiting for clicfg.exe to
//  						reset the event sources to the site-wide default.
//
//
//  Parameters:
//
//      none
//
//
//  Returns:
//
//      BOOL		TRUE if there were no errors, FALSE if there were!
//							
//
//============================================================================

BOOL EventConfigModifier::CheckInstallations()
{
	HKEY hkeyOpen;
	CString keyName;
	DWORD createtype;

	keyName.LoadString(IDS_SNMP);


	//	Is SNMP installed?
	//	==================

	LONG Result = RegOpenKeyEx(hkey_machine, keyName,
								0, KEY_EXECUTE, &hkeyOpen);

    if (Result != ERROR_SUCCESS)
	{
		SetError(EVCMT_NO_SNMP);
        SNMPinstalled = FALSE;
		StatusMif(IDS_NO_SNMP_INSTALLED, FALSE);
	}
	else
		SNMPinstalled = TRUE;

	RegCloseKey(hkeyOpen);
	keyName.Empty();        
 	keyName.LoadString(IDS_SNMP_AGENT);


	//	Is the SNMP sub-agent SNMPELEA.DLL installed?
	//	=============================================

	Result = RegOpenKeyEx(hkey_machine, keyName,
								0, KEY_EXECUTE, &hkeyOpen);

    if (Result != ERROR_SUCCESS)
	{
		SetError(EVCMT_NO_SNMP_XN);
        return FALSE;
	}

	RegCloseKey(hkeyOpen);        
	

	//	Is somebody else editing our piece of the registry
	//	==================================================

	keyName.Empty();        
 	keyName.LoadString(IDS_LOCK_REG);
	Result = RegCreateKeyEx(hkey_machine, keyName, 0,
								NULL, REG_OPTION_VOLATILE,
								KEY_ALL_ACCESS, NULL, &hkeyOpen, &createtype);

    if (Result != ERROR_SUCCESS)
	{
		StatusMif(IDS_NOT_LOCKED, FALSE);
		SetError(EVCMT_LOCK_FAIL);
        return FALSE;
	}
	
	RegCloseKey(hkeyOpen);

    if (createtype == REG_OPENED_EXISTING_KEY)
	{
		StatusMif(IDS_REG_LOCKED, FALSE);
		SetError(EVCMT_LOCK_OUT);
        return FALSE;
	}
    
	keyName.Empty();        
 	keyName.LoadString(IDS_BASE_KEY);
	Result = RegOpenKeyEx(hkey_machine, keyName,
								0, KEY_ALL_ACCESS, &hkeyOpen);
    
    if (Result != ERROR_SUCCESS)
	{
		SetError(EVCMT_REG_FAIL);
        return FALSE;
	}

	
	//	Check and if necessary SET the configuration mode
	//	=================================================
	
	DWORD mode;
	
	if (SetCustom)
		mode = EVENTCMT_CUSTOM_MODE;
	else
		mode = EVENTCMT_SYSTEM_MODE;

	
	//	/DEFAULT was NOT a command line switch
	//	======================================
	
	if (MandatoryMode)
	{
		//	Set the mode required
		//	=====================

		keyName.Empty();        
	 	keyName.LoadString(IDS_CONF_TYPE);
		Result = RegSetValueEx(hkeyOpen, keyName, 0, REG_DWORD,
							(CONST BYTE *)&mode, 4);

		if (Result != ERROR_SUCCESS)
		{
			RegCloseKey(hkeyOpen);
			keyName.Empty();        
 			keyName.LoadString(IDS_LOCK_REG);
			RegDeleteKey(hkey_machine, keyName);
			SetError(EVCMT_REG_FAIL);
			StatusMif(IDS_MODE_FAIL, FALSE);
			return FALSE;
		}

	}
	else	// /DEFAULT specified, only run if allowed
	{
		TCHAR val[1024 + 1];
		DWORD valsz;
		DWORD type;
		CString Conftype;
		Conftype.LoadString(IDS_CONF_TYPE);
		BOOL gotmode = FALSE;

		
		//	Get the current mode
		//	====================

		valsz = 1024 + 1;
		Result = RegQueryValueEx(hkeyOpen, Conftype, NULL,
									&type, (unsigned char*)val, &valsz);

		if (Result != ERROR_SUCCESS)	//didn't find the key for the mode
		{
			if (Result == ERROR_FILE_NOT_FOUND)	//it's not there!
			{
				gotmode = FALSE;
			}
			else
			{
				RegCloseKey(hkeyOpen);
				keyName.Empty();        
	 			keyName.LoadString(IDS_LOCK_REG);
				RegDeleteKey(hkey_machine, keyName);
				SetError(EVCMT_REG_FAIL);
				return FALSE;
			}
		}
		else						//got the key now what's the current mode
		{
			if (valsz > 0)
			{
				if (type == REG_DWORD)
				{
					mode =  * ((DWORD *)val);
					gotmode = TRUE;
				}
			}
		}

		if (!gotmode || (mode == EVENTCMT_REQUST_MODE))
		{
		 //	No current mode or default requested
		 //	====================================

			if (SetCustom)	//Set to Custom mode requested
			{
				mode = EVENTCMT_CUSTOM_MODE;
			}
			else			//Set to default mode
			{
				mode = EVENTCMT_SYSTEM_MODE;
			}

			keyName.Empty();        
		 	keyName.LoadString(IDS_CONF_TYPE);
			Result = RegSetValueEx(hkeyOpen, keyName, 0, REG_DWORD,
								(CONST BYTE *)&mode, 4);

			if (Result != ERROR_SUCCESS)
			{
				RegCloseKey(hkeyOpen);
				keyName.Empty();        
	 			keyName.LoadString(IDS_LOCK_REG);
				RegDeleteKey(hkey_machine, keyName);
				SetError(EVCMT_REG_FAIL);
				StatusMif(IDS_MODE_FAIL, FALSE);
				return FALSE;
			}

		}
		
		else if (gotmode && (mode == EVENTCMT_CUSTOM_MODE))	
		{
		 //	The current mode is Custom
		 //	==========================

			RegCloseKey(hkeyOpen);
			keyName.Empty();        
 			keyName.LoadString(IDS_LOCK_REG);
			RegDeleteKey(hkey_machine, keyName);
			SetError(EVCMT_CUSTOM_SET);
			StatusMif(IDS_DFLT_CONFLICT, FALSE);
			return FALSE;
		}

		else if (gotmode && (mode == EVENTCMT_SYSTEM_MODE) && SetCustom)
		{
		 //	The current mode is the default and /SETCUSTOM specified
		 //	========================================================

			mode = EVENTCMT_CUSTOM_MODE;
			keyName.Empty();        
		 	keyName.LoadString(IDS_CONF_TYPE);
			Result = RegSetValueEx(hkeyOpen, keyName, 0, REG_DWORD,
								(CONST BYTE *)&mode, 4);

			if (Result != ERROR_SUCCESS)
			{
				RegCloseKey(hkeyOpen);
				keyName.Empty();        
	 			keyName.LoadString(IDS_LOCK_REG);
				RegDeleteKey(hkey_machine, keyName);
				SetError(EVCMT_REG_FAIL);
				StatusMif(IDS_MODE_FAIL, FALSE);
				return FALSE;
			}
		}
	}

	RegCloseKey(hkeyOpen);
    
	return TRUE;	
}


//============================================================================
//  EventConfigModifier::ProcessTrapQ
//
//  This private method is called to process the list of trap destinations.
//
//
//  Parameters:
//
//      none
//
//
//  Returns:
//
//      none
//							
//
//============================================================================

void EventConfigModifier::ProcessTrapQ()
{
	//	Get the first item from the TrapQ
	//	=================================

	TrapCommandItem* TCmnd = (TrapCommandItem*)TrapQ.Pop();
	
	if (!TCmnd)
	{
		CString sstr;
		sstr += NL;
		sstr += NL;
		CString msg;
		msg.LoadString(IDS_MSG51);
		sstr += msg;
		sstr += NL;
		sstr += NL;
		WriteToLog(&sstr);
		return;
	}

	
	//	Get the current SNMP trap destination settings, we'll edit this
	//	in memory and then write the changes back once we have finished
	//	===============================================================

	if (!ReadSNMPRegistry())
	{
		CString sstr;
		sstr += NL;
		sstr += NL;
		CString msg;
		msg.LoadString(IDS_MSG36);
		sstr += msg;
		sstr += NL;
		sstr += NL;
		WriteToLog(&sstr);
		StatusMif(IDS_REGREAD,FALSE);
		SetError(EVCMT_REG_FAIL);
		return;
	}


	//	Loop through the TrapQ adding to and deleting from
	//	CommNames as you go to build a new registry image.
	//	==================================================

	while (TCmnd)
	{
		//	Process the current configuration request
		//	=========================================
		ReturnVal retval;
		CString * msg = TCmnd->Process(&CommNames, &retval);

		if(retval != RET_OK)					//No change was made...
		{
			if (retval == RET_BAD)						//An error occurred
			{
				StatusMif(IDS_SYNTAX_ERROR, FALSE);
				SetError(EVCMT_INVALID_COMMAND);
			}
			else  if (TCmnd->GetCommand() != AddTrap)	//RET_NOT_FOUND && DeleteTrap
			{
				StatusMif(IDS_DEL_MISS_ENTRY, TRUE);
			}
				
		}
		else									//Successful modification
		{
			SNMPModified = TRUE;
		}

		if (msg)
		{
			WriteToLog(msg);
			delete msg;
		}
			
		
		//	Delete this configuration request nd get the next one...
		//	========================================================
		
		delete TCmnd;
		TCmnd = (TrapCommandItem*)TrapQ.Pop();
	}


	//	Finished processing the TrapQ. If we made changes, write
	//	all changes to the registry and stop the SNMP service
	//	========================================================

	if (SNMPModified)
	{
		if (WriteSNMPRegistry())
		{
			StopSNMP();
		}
	}
	else
	{
		CString sstr;
		sstr += NL;
		sstr += NL;
		CString messg;
		messg.LoadString(IDS_MSG51);
		sstr += messg;
		sstr += NL;
		sstr += NL;
		WriteToLog(&sstr);
	}

	
	//	Start the SNMP service regardless of if we stopped it
	//	=====================================================

	StartSNMP();
}


//============================================================================
//  EventConfigModifier::StopSNMP
//
//  This private method is called to stop the SNMP service.
//
//
//  Parameters:
//
//      none
//
//
//  Returns:
//
//      none
//							
//
//============================================================================

void EventConfigModifier::StopSNMP()
{
	//	Check that /NOSTOPSTART was NOT specified
	//	=========================================
	
	if (!SNMPStopStart)
	{
		SetError(EVCMT_SNMP_STOPSTART);
		return;	
	}

	
	//	Get a handle to the service manager so we can get the SNMP service
	//	==================================================================
	
	SC_HANDLE services;

	if (Machine.GetLength())
	{
		services = OpenSCManager(Machine,			//the remote machine
								"ServicesActive",	//active services
								GENERIC_EXECUTE);	//want to interrogate, start and stop a service
	}
	else
	{
		services = OpenSCManager(NULL,				//NULL for local machine
								"ServicesActive",	//active services
								GENERIC_EXECUTE);	//want to interrogate, start and stop a service
	}
	
	if (!services)	//didn't connect to the service control manager, log it
	{
		DWORD its_broke = GetLastError();
		CString sstr;
		sstr += NL;
		sstr += NL;
		CString msg;
		msg.LoadString(IDS_MSG37);
		sstr += msg;
		sstr += NL;
		msg.Empty();
		msg.LoadString(IDS_MSG38);
		sstr += msg;
		sstr += NL;
		sstr += NL;
		WriteToLog(&sstr);
		StatusMif(IDS_STARTSTOPSNMP, FALSE);
		SetError(EVCMT_SNMP_STOPSTART);
		SNMPStopStart = FALSE; //don't try start.
		return;
	}

	
	//	Get a handle to the SNMP service
	//	================================
	
	SC_HANDLE snmp_service = OpenService(services, "SNMP", SERVICE_CONTROL_INTERROGATE |
											SERVICE_START | SERVICE_STOP);

	if (!snmp_service)	//didn't get a handle to the SNMP service, log it
	{
		DWORD its_broke = GetLastError();
		CloseServiceHandle(services);
		CString sstr;
		sstr += NL;
		sstr += NL;
		CString msg;
		msg.LoadString(IDS_MSG37);
		sstr += msg;
		sstr += NL;
		msg.Empty();
		msg.LoadString(IDS_MSG38);
		sstr += msg;
		sstr += NL;
		sstr += NL;
		WriteToLog(&sstr);
		StatusMif(IDS_STARTSTOPSNMP, FALSE);
		SetError(EVCMT_SNMP_STOPSTART);
		SNMPStopStart = FALSE; //don't try start.
		return;
	}
	
	SERVICE_STATUS stat;

	
	//	Send the message to stop the service
	//	====================================
	
	if (!ControlService(snmp_service, SERVICE_CONTROL_STOP, &stat))
	{
		DWORD its_broke = GetLastError();
		BOOL flag_error = FALSE;

		if (stat.dwCurrentState == SERVICE_STOP_PENDING)
		{
			Sleep(10000);	//ten second wait
			
			if (ControlService(snmp_service, SERVICE_CONTROL_INTERROGATE, &stat))
			{
				if (stat.dwCurrentState != SERVICE_STOPPED)	//has it stopped?
				{
					flag_error = TRUE; //raise an error
				}
			}
			else
			{
				flag_error = TRUE;	//couldn't contact service? raise error!
			}
		}		
		else if (stat.dwCurrentState != SERVICE_STOPPED && its_broke != ERROR_SERVICE_NOT_ACTIVE)
		{
			flag_error = TRUE;	//raise an error if the service is not stopped.
		}

		if (flag_error)
		{
			CloseServiceHandle(snmp_service);
			CloseServiceHandle(services);
			CString sstr;
			sstr += NL;
			sstr += NL;
			CString msg;
			msg.LoadString(IDS_MSG37);
			sstr += msg;
			sstr += NL;
			msg.Empty();
			msg.LoadString(IDS_MSG38);
			sstr += msg;
			sstr += NL;
			sstr += NL;
			WriteToLog(&sstr);
			StatusMif(IDS_STARTSTOPSNMP, FALSE);
			SetError(EVCMT_SNMP_STOPSTART);
			SNMPStopStart = FALSE; //don't try start.
			return;
		}
	}

	CloseServiceHandle(snmp_service);
	CloseServiceHandle(services);	
}


//============================================================================
//  EventConfigModifier::StartSNMP
//
//  This private method is called to start the SNMP service.
//
//
//  Parameters:
//
//      none
//
//
//  Returns:
//
//      none
//							
//
//============================================================================

void EventConfigModifier::StartSNMP()
{
	//	Check that /NOSTOPSTART was NOT specified and there
	//	were no errors in stopping the SNMP service earlier
	//	===================================================
	
	if (!SNMPStopStart)
	{
		return;	
	}

	//	Get a handle to the service manager so we can get the SNMP service
	//	==================================================================
	
	SC_HANDLE services;
	
	if (Machine.GetLength())
	{
		services = OpenSCManager(Machine,	//the remote machine
								"ServicesActive",	//active services
								GENERIC_EXECUTE);	//want to interrogate, start and stop a service
	}
	else
	{
		services = OpenSCManager(NULL,		//NULL for local machine
								"ServicesActive",	//active services
								GENERIC_EXECUTE);	//want to interrogate, start and stop a service
	}
	
	if (!services)	//didn't connect to the service control manager, log it
	{
		DWORD its_broke = GetLastError();
		CString sstr;
		sstr += NL;
		sstr += NL;
		CString msg;
		msg.LoadString(IDS_MSG37);
		sstr += msg;
		sstr += NL;
		msg.Empty();
		msg.LoadString(IDS_MSG38);
		sstr += msg;
		sstr += NL;
		sstr += NL;
		WriteToLog(&sstr);
		
		if (SNMPModified)		//Only log raise an error if we need a restart for changes.
		{
			StatusMif(IDS_STARTSTOPSNMP, FALSE);
			SetError(EVCMT_SNMP_STOPSTART);
		}

		return;
	}

	
	//	Get a handle to the SNMP service
	//	================================
	
	SC_HANDLE snmp_service = OpenService(services, "SNMP", SERVICE_CONTROL_INTERROGATE |
											SERVICE_START | SERVICE_STOP);

	if (!snmp_service)	//failed to get the handle, log it
	{
		DWORD its_broke = GetLastError();
		CloseServiceHandle(services);
		CString sstr;
		sstr += NL;
		sstr += NL;
		CString msg;
		msg.LoadString(IDS_MSG37);
		sstr += msg;
		sstr += NL;
		msg.Empty();
		msg.LoadString(IDS_MSG38);
		sstr += msg;
		sstr += NL;
		sstr += NL;
		WriteToLog(&sstr);

		if (SNMPModified)		//Only log raise an error if we need a restart for changes
		{
			StatusMif(IDS_STARTSTOPSNMP, FALSE);
			SetError(EVCMT_SNMP_STOPSTART);
		}
		return;
	}

	BOOL flag_error = FALSE;

	
	//	Try to start the SNMP service
	//	=============================
	
	if (!StartService(snmp_service, 0, NULL))
	{
		DWORD its_broke = GetLastError();
		
		if (its_broke == ERROR_SERVICE_ALREADY_RUNNING)
		{
			if (!SNMPModified) //we didn't stop the service so don't report an error or try again.
			{
				CloseServiceHandle(snmp_service);
				CloseServiceHandle(services);
				return;
			}

			Sleep(10000);	//ten second wait for it to stop
		
			if (!StartService(snmp_service, 0, NULL)) //one last try.
			{
				flag_error = TRUE;	//we made changes and can't stop/start snmp, error.
			}
		}
		else		//it didn't start and it's not running, raise an error
		{
			flag_error = TRUE;
		}
	}

	if (!flag_error)	// no error, log it.
	{
		CString sstr;
		sstr += NL;
		sstr += NL;
		CString msg;
		msg.LoadString(IDS_MSG52);
		sstr += msg;
		sstr += NL;
		sstr += NL;
		WriteToLog(&sstr);
	}
	else			//there was an error, log it
	{
		CString sstr;
		sstr += NL;
		sstr += NL;
		CString msg;
		msg.LoadString(IDS_MSG39);
		sstr += msg;
		sstr += NL;
		msg.Empty();
		msg.LoadString(IDS_MSG40);
		sstr += msg;
		sstr += NL;
		sstr += NL;
		WriteToLog(&sstr);
		StatusMif(IDS_STARTSNMP, FALSE);
		SetError(EVCMT_SNMP_START);
	}

	CloseServiceHandle(snmp_service);
	CloseServiceHandle(services);
}


//============================================================================
//  EventConfigModifier::WriteSNMPRegistry
//
//  This private method is called to write the new image of the trap destination
//	configuration. First the current config is deleted from the registry and
//	then the new configuration (old config cached and modified in memory) is
//	written into the registry.
//
//
//  Parameters:
//
//      none
//
//
//  Returns:
//
//      BOOL		TRUE if this function is successful, FALSE otherwise.
//					NOTE:	There may have been problems but this function
//							will still return TRUE if they were not serious.
//							
//
//============================================================================

BOOL EventConfigModifier::WriteSNMPRegistry()
{
	HKEY hkeyOpen;
	CString keyName;

	keyName.LoadString(IDS_TRAP_CONF);

	
	//	Open the registry at the point were the trap dests are kept
	//	===========================================================
	
	LONG Result = RegOpenKeyEx(hkey_machine, keyName,
								0, KEY_ALL_ACCESS, &hkeyOpen);

    if (Result != ERROR_SUCCESS)
	{
		CString sstr;
		sstr += NL;
		sstr += NL;
		CString amsg;
		amsg.LoadString(IDS_MSG45);
		sstr += amsg;
		sstr += NL;
		sstr += NL;
		WriteToLog(&sstr);
		StatusMif(IDS_REGWRITE, FALSE);
		SetError(EVCMT_REG_FAIL);
        return FALSE;
	}

	
	//	Delete all trap destinations
	//	============================

	if(!DeleteSNMPRegistry(&hkeyOpen))
	{
		CString sstr;
		sstr += NL;
		sstr += NL;
		CString msg;
		msg.LoadString(IDS_MSG41);
		sstr += msg;
		sstr += NL;
		msg.Empty();
		msg.LoadString(IDS_MSG42);
		sstr += msg;
		sstr += NL;
		sstr += NL;
		WriteToLog(&sstr);
		StatusMif(IDS_REGWRITE, FALSE);
		SetError(EVCMT_REG_FAIL);
        return FALSE;

	}

	CString sstr;
	sstr += NL;
	sstr += NL;
	CString amsg;
	amsg.LoadString(IDS_MSG44);
	sstr += amsg;
	sstr += NL;
	sstr += NL;
	WriteToLog(&sstr);

	
	//	Now loop through our image of the trap destinations. For
	//	each community name create a new registry key and for each
	//	destination address under that community name create a value
	//	============================================================

	CommListItem * Comm = (CommListItem *)CommNames.Pop();

	while (Comm)
	{
		HKEY hkey;
		DWORD createtype;
		CString * commkey = Comm->GetString();
		Result = RegCreateKeyEx(hkeyOpen, *commkey, 0,
							NULL, REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hkey, &createtype);

	    if (Result != ERROR_SUCCESS)	//failed to create this key, log it and continue
		{
			CString sstr;
			sstr += NL;
			CString msg;
			msg.LoadString(IDS_MSG46);
			sstr += msg;
			sstr += *commkey;
			sstr += NL;
			msg.Empty();
			msg.LoadString(IDS_MSG47);
			sstr += msg;
			sstr += NL;
			sstr += NL;
			WriteToLog(&sstr);
    	 	delete Comm;
			CommListItem * Comm = (CommListItem *)CommNames.Pop();
			StatusMif(IDS_REGWRITE, FALSE);
			SetError(EVCMT_REG_FAIL);
    	    continue;
		}
		

		//	Add the addresses for this community name
		//	=========================================

		ListItem * addr = Comm->addresses.Pop();
		int x = 1;

		while (addr)
		{
			char str[34];
			_ultoa(x, str, 10);
			CMyString * addrstr = (CMyString *)addr->GetString();
			LPTSTR val = addrstr->GetBuffer(1); 
			DWORD valsz = addrstr->GetBufferSize();

			Result = RegSetValueEx(hkey, str, 0, REG_SZ,
							(const unsigned char*) val, valsz);
			
		    if (Result != ERROR_SUCCESS)	//failed to create this value, log it and continue.
			{
				CString sstr;
				sstr += NL;
				CString msg;
				msg.LoadString(IDS_MSG48);
				sstr += msg;
				sstr += val;
				sstr += NL;
				msg.Empty();
				msg.LoadString(IDS_MSG49);
				sstr += msg;
				sstr += *commkey;
				sstr += NL;
				sstr += NL;
				WriteToLog(&sstr);
				StatusMif(IDS_REGWRITE, FALSE);
				SetError(EVCMT_REG_FAIL);
			}
			
			addrstr->ReleaseBuffer();
			x++;

			delete addr;
			addr = Comm->addresses.Pop();
		}

		RegCloseKey(hkey);
		delete Comm;	
		Comm = (CommListItem *)CommNames.Pop();
	}
	

	//	Finished writing to the (trap destination's part of the) registry
	//	=================================================================

	RegCloseKey(hkeyOpen);
	CString astr;
	astr += NL;
	astr += NL;
	CString done;
	done.LoadString(IDS_MSG43);
	astr += done;
	astr += NL;
	astr += NL;
	WriteToLog(&astr);

	return TRUE;
}


//============================================================================
//  EventConfigModifier::DeleteSNMPRegistry
//
//  This private method is called to delete the current SNMP trap destinations
//	from the registry. The only parameter is the starting point in the registry.
//
//	Note:	This method can be used to delete any section of the registry
//			given a "root" key to start from. All keys below this "root"
//			will be deleted. This function is recursive.
//
//
//  Parameters:
//
//      none
//
//
//  Returns:
//
//      BOOL	TRUE if this function is a complete success, FALSE if any
//				part of the trap dest config remains in the registry.
//							
//
//============================================================================

BOOL EventConfigModifier::DeleteSNMPRegistry(HKEY * hkey)
{
	HKEY hkeyOpen;
	char Buffer[1024 + 1];
	DWORD dwLength;
	int i = 0;
	LONG Result;

    
	//	Enumerate all keys below the "root" (the parameter)
	//	and delete them by recursively calling this method.
	//	===================================================

    while (TRUE)
    {
        
        //	Get a key below the root
		//	========================
        dwLength = 1024 + 1;
        Result = RegEnumKeyEx(*hkey, i, Buffer, &dwLength, NULL,
       					     NULL, NULL, NULL);
       
        if (Result != ERROR_SUCCESS)
            break;
       
        if (dwLength > 0)
		{
			Result = RegOpenKeyEx(*hkey, Buffer, 0,
										KEY_ALL_ACCESS, &hkeyOpen);
			
			if (Result != ERROR_SUCCESS)
				break;

			
			//	Delete any the keys below this one,
			//	if there are none then delete this key
			//	======================================
			
			if (!DeleteSNMPRegistry(&hkeyOpen))
			{
				RegCloseKey(hkeyOpen);
				return FALSE;
			}
			else
			{
			 	RegCloseKey(hkeyOpen);
				Result = RegDeleteKey(*hkey, Buffer);

			}

        }
       
    }


    //	Did we find a normal end condition?
	//	===================================

    if (Result == ERROR_NO_MORE_ITEMS)
        return TRUE;

    return FALSE;

}


//============================================================================
//  EventConfigModifier::ReadSNMPRegistry
//
//  This private method is called to read the registry and construct an image
//	of the trap destination configuration so they may be manipulated in memory.
//
//
//  Parameters:
//
//      none
//
//
//  Returns:
//
//      BOOL
//							
//
//============================================================================

BOOL EventConfigModifier::ReadSNMPRegistry()
{
	HKEY hkeyOpen;
	CString keyName;

	keyName.LoadString(IDS_TRAP_CONF);

	
	//	Get the root key of all the community names
	//	===========================================
	
	LONG Result = RegOpenKeyEx(hkey_machine, keyName,
								0, KEY_READ, &hkeyOpen);
    if (Result != ERROR_SUCCESS)
        return FALSE;

    char Buffer[1024 + 1];
    DWORD dwLength;
    int i = 0;
	BOOL NoError = TRUE;


	//	Now enumerate this key and get all the community names
	//	For each community name construct a list of addresses.
	//	======================================================

    while (NoError)
    {
        dwLength = 1024 + 1;
        Result = RegEnumKeyEx(hkeyOpen, i, Buffer, &dwLength, NULL,
       					     NULL, NULL, NULL);
       
        if (Result != ERROR_SUCCESS)
            break;
       
        if (dwLength > 0)
		{
			CString * str = new CString(Buffer);
			CommListItem * item = new CommListItem(str);
			CommNames.Add(item); //There can't be duplicates (it's a regkey)
            NoError = item->GetAddresses(&hkeyOpen);
        }
       
        i++;
    }

	RegCloseKey(hkeyOpen);

    
    //	Did we find a normal end condition?
	//	===================================

    if ((Result == ERROR_NO_MORE_ITEMS) && NoError)
        return TRUE;

    return FALSE;
}


//============================================================================
//  EventConfigModifier::ProcessCommandQ
//
//  This private method is called to process the list of translation requests.
//	
//	Note:	If the compile time flag EVENTCMT_VALIDATE_ID is TRUE then the
//			eventIds are validated on the machine that the tool is running on.
//			If this is remote then the validation may wrongly fail to update
//			the configuration.
//
//
//  Parameters:
//
//      none
//
//
//  Returns:
//
//      none
//							
//
//============================================================================

void EventConfigModifier::ProcessCommandQ()
{
	CommandItem* Cmnd = CommandQ.Pop();


	//	Loop through the queue of commands
	//	==================================

	while (Cmnd)
	{
		SourceItem * src = (SourceItem *) NULL;

		switch (SourceL.FindItem(Cmnd->GetEventSource(), src))
		{
			case RET_BAD: //write to log - invalid event source
			{
				CString * msg = Cmnd->WriteToBuff(IDS_MSG29);
				WriteToLog(msg);
				delete msg;
				StatusMif(IDS_SOURCE_ERROR, FALSE);
				SetError(EVCMT_INVALID_COMMAND);
				break;
			}

			case RET_NOTFOUND: //get the msg dll and enumerate the eventids
			{
				src = new SourceItem(Cmnd->GetEventSource(), hkey_machine);
#ifdef EVENTCMT_VALIDATE_ID
				BOOL Is_OK = src->EnumerateEventIDMap(Cmnd);
#else //EVENTCMT_VALIDATE_ID
				BOOL Is_OK = TRUE;
#endif //EVENTCMT_VALIDATE_ID
				src->SetExists(Is_OK);

				
				//	We have a valid event source (always TRUE if EVENTCMT_VALIDATE_ID==FALSE)
				//	=========================================================================

				if (Is_OK)				
				{					
#ifdef EVENTCMT_VALIDATE_ID
					//validate eventid
					DWORD val;

					if (src->eventIDs.Lookup(Cmnd->GetEventID(), val))
#endif //EVENTCMT_VALIDATE_ID
					{
						ReturnVal retval;
						
						
						//	Valid evntID, make the necessary change to the registry
						//	=======================================================

						CString * msg = Cmnd->ModifyRegistry(src, &retval);

						if (retval != RET_OK)
						{
							if(retval == RET_BAD)
							{
								StatusMif(IDS_REGERROR, FALSE);
								SetError(EVCMT_REG_FAIL);
							}
							else
								StatusMif(IDS_DEL_MISS_ENTRY, TRUE);
							
						}

						if (msg) //this should always be true!
						{
							WriteToLog(msg);
							delete msg;
						}
					}
#ifdef EVENTCMT_VALIDATE_ID
					else	//The event id was not in the message dll
					{
						CString * msg = Cmnd->WriteToBuff(IDS_MSG30);
						WriteToLog(msg);
						StatusMif(IDS_EVENTID_ERROR, FALSE);
						SetError(EVCMT_INVALID_COMMAND);
						delete msg;
					}
#endif //EVENTCMT_VALIDATE_ID
				}
				else
				{
					CString * msg = Cmnd->WriteToBuff(IDS_MSG29);
					WriteToLog(msg);
					StatusMif(IDS_SOURCE_ERROR, FALSE);
					SetError(EVCMT_INVALID_COMMAND);
					delete msg;
				}

				SourceL.Add(src);
				break;
			}			
			
			case RET_OK:	//valid event source
			{
#ifdef EVENTCMT_VALIDATE_ID
				DWORD val;

				if (src->eventIDs.Lookup(Cmnd->GetEventID(), val))
#endif //EVENTCMT_VALIDATE_ID
				{
					ReturnVal retval;
						
					
					//	Valid evntID, make the necessary change to the registry
					//	=======================================================

					CString * log = Cmnd->ModifyRegistry(src, &retval);

					if (retval != RET_OK)
					{
						if(retval == RET_BAD)
						{
							StatusMif(IDS_REGERROR, FALSE);
							SetError(EVCMT_REG_FAIL);
						}
						else
							StatusMif(IDS_DEL_MISS_ENTRY, TRUE);
						
					}

					if (log) //this should always be true!
					{
						WriteToLog(log);
						delete log;
					}
				}
#ifdef EVENTCMT_VALIDATE_ID
				else	//invalid event source (never true if EVENTCMT_VALIDATE_ID==FALSE)
				{
					CString * log = Cmnd->WriteToBuff(IDS_MSG30);
					WriteToLog(log);
					StatusMif(IDS_SYNTAX_ERROR, FALSE);
					SetError(EVCMT_INVALID_COMMAND);
					delete log;
				}
#endif //EVENTCMT_VALIDATE_ID

				break;
			}
			
			default: //huh?!
				break;
		}

		//	Delete the current command (just processed) and get the next one
		//	================================================================

		delete Cmnd;
		Cmnd = CommandQ.Pop();
	}
}


//============================================================================
//  EventConfigModifier::Load
//
//  This private method is called to read in the config file and open the log
//	file. This method is virtually redundant if EVENTCMT_OLD_LOG is false, and
//	the SMS logging system is in use.
//
//
//  Parameters:
//
//      none
//
//
//  Returns:
//
//      none
//							
//
//============================================================================

void EventConfigModifier::Load()
{
	BOOL goodresult = TRUE;

#ifdef EVENTCMT_OLD_LOG

	//	Check that a log file is wanted, if it is open the log
	//	======================================================

	if(LogWanted)
	{
		goodresult = OpenLogFile();
		closeHandle = goodresult;
	}
	else
		closeHandle = FALSE;
#endif	//EVENTCMT_OLD_LOG


	//	Read the config file
	//	====================

    if (goodresult)
		ReadConfigIn();
	else					//failed to open the log file
	{
		StatusMif(IDS_NOLOGFILE, FALSE);
		SetError(EVCMT_LOGOPEN_FAILED);
	}
}

#ifdef EVENTCMT_OLD_LOG


//============================================================================
//  EventConfigModifier::OpenLogFile
//
//  This private method is called to open the log file. The logfile name is
//	in the string resource for this application. The directory that it is
//	created in (new file each time this function is called) is determined in
//	this order:
//	1.	THE LOGPATH FOUND BY GetSMSPath (Currently kept in sms.ini)
//	2.	THE DIRECTORY WHERE THE SMS ADMIN UI LOG IS KEPT (found by reading the
//		registry key - SOFTWARE\Microsoft\SMS\TRACING\SMS_USER_INTERFACE -
//		the value of TraceFileName.
//	3.	THE CURRENT USER'S TEMP PATH 
//
//
//  Parameters:
//
//      none
//
//
//  Returns:
//
//      BOOL		TRUE if a log file was opened, FALSE otherwise.
//							
//
//============================================================================

BOOL EventConfigModifier::OpenLogFile()
{
	CString logfile;
	CString logname;
	logname.LoadString(IDS_LOG_FILE);
	CString DirSep;
	DirSep.LoadString(IDS_DIRSEP);
	TCHAR logfilebuff[1024 + 1];
	DWORD BufferSize = 1024 + 1;
	CString * path = NULL;


	//	First try and find the client logs dir
	//	======================================

	DWORD Result = GetSMSPath(GETPATH_CLIENT_LOG_DIR,
								logfilebuff, BufferSize);

	if (Result != GETPATH_NO_ERROR)
	{
		//	Now try and find the SMS ADMIN UI log
		//	=====================================
	
		path = GetRegStr(hkey_machine, IDS_SMS_TRACE, CString("TraceFilename"));

		if (path)
		{
			for(int j = path->GetLength() - 1; j > 1; j--)
			{
				if (path->GetAt(j) == DirSep.GetAt(0))
				{
					for (int k = j+1; k < path->GetLength(); k++) 
						path->SetAt(k, tab.GetAt(0));
					
					path->TrimRight();
					break;
				}
			}

			if (!(path->GetAt(1) == DirSep.GetAt(0)) || !(path->GetAt(2) == DirSep.GetAt(0)))
			{			
				CString * drive = GetRegStr(hkey_machine, IDS_SMS_INSTALL,
										CString("Installation Directory"));
				
				for (int l = 2; l < drive->GetLength(); l++)
					drive->SetAt(l, tab.GetAt(0));
				
				drive->TrimRight();
				logfile += *drive;
				delete drive; 
			}

			logfile += *path;
		}
	}
	else
		logfile = logfilebuff;

	if (path)
		delete path;

	int loglength = logfile.GetLength();


	//	Still no path! Try $TEMP
	//	========================

	if (loglength == 0)
	{
		CString * temp = GetRegStr(HKEY_CURRENT_USER, IDS_TEMP, CString("temp"));
		
		if (temp)
		{
			logfile += *temp;
			delete temp;
		}
	}

	
	//	Now add the log file name
	//	=========================
	
	loglength = logfile.GetLength() - 1;

	if(logfile.GetAt(loglength) != DirSep.GetAt(0))
		logfile += DirSep;

	logfile += logname;

	
	//	Create the file, delete any old versions
	//	========================================
	
	hFile = CreateFile(logfile,
						GENERIC_WRITE,
						0,
						NULL,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL |
						//FILE_FLAG_OVERLAPPED |
						FILE_FLAG_WRITE_THROUGH,
						NULL);
	
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();	// didn't work
		return FALSE;
	}
	
	return TRUE;

}


//============================================================================
//  EventConfigModifier::GetRegStr
//
//  This private method is called to get the value of a string from the
//	registry. This method takes three parameters. A open key to the registry,
//	the resource id of the string which is the key name to be opened to find
//	value and the name of the value whose value is being sought.(!?!)
//	(Read the code it's real simple!!)
//
//
//  Parameters:
//
//      HKEY openhkey	HKEY_LOCAL_MACHINE for the target registry.
//
//		UINT id			The resource ID for the full path to the registry key
//						whose value is to be read
//
//		CString name	The name of the value to be read.
//
//
//  Returns:
//
//      CString *		A pointer to the CString object containing the string
//						read from the registry. This is NULL if the valuename
//						was not found.
//							
//
//============================================================================

CString *  EventConfigModifier::GetRegStr(HKEY openhkey, UINT id, CString name)
{
	CString * ret = NULL;
	HKEY hkey;
	CString key;
	

	//	Load the registry key name
	//	==========================

	key.LoadString(id);

	
	//	Open the registry key that is to be read
	//	========================================
	
	LONG Result = RegOpenKeyEx(openhkey, key,
							0, KEY_EXECUTE, &hkey);

	if (Result != ERROR_SUCCESS)
		return ret;

	TCHAR val[1024 + 1]	;
	DWORD valsz;
	DWORD type;


	//	Read the registry key for the value requested
	//	=============================================

	Result = RegQueryValueEx(hkey, name, NULL, &type,
					 		(unsigned char*)val, &valsz);

	if (Result != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		return FALSE;
	}

	if (valsz > 0)
	{
		ret = new CString(val);
	}

	RegCloseKey(hkey);
	return ret;
}

#endif	//EVENTCMT_OLD_LOG
