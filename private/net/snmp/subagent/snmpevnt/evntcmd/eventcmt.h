//****************************************************************************
//
//  Copyright (c) 1996, Microsoft Corporation
//
//  File:  eventcmt.h
//
//  NT Event to SNMP trap translator config. modification code. Shared by
//	CLICFG and the EVNTTRAP\CONFIG projects.
//
//  History:
//
//      nadira		03/29/96	Created.
//
//****************************************************************************


//	Return codes from EventConfigModifier::Main
//	===========================================

#define EVCMT_SUCCESS 			0		//success
#define EVCMT_BAD_INPUT_FILE	2		//could not open input file
#define EVCMT_INVALID_COMMAND	4		//at least one command failed
#define EVCMT_BAD_ARGS			8		//at least one invalid argument
#define EVCMT_NO_SNMP			16		//SNMP is not installed (correctly?)
#define EVCMT_NO_SNMP_XN		32		//The extension to SNMP is not installed (correctly)
#define EVCMT_LOCK_FAIL			64		//failed to create volatile regkey to lock reg
#define EVCMT_LOCK_OUT			128		//volatile regkey is present already 
#define EVCMT_REG_FAIL			256		//Failed to read from or write to the registry
#define EVCMT_CUSTOM_SET		512		//Custom and /DEFAULT was specified so not overidden
#define EVCMT_SNMP_STOPSTART	1024	//failed to stop the SNMP service
#define EVCMT_SNMP_START		2048	//failed to re-start the SNMP service after stopping it
#define EVCMT_LOGOPEN_FAILED	4096	//failed to create log file.
#define EVCMT_REG_CNNCT_FAILED	8192	//failed to connect to the registry.


//  Event source configuration modes - used by eventcmt.exe, eventrap.exe, and clicfg.exe.
//  ======================================================================================

#define EVENTCMT_SYSTEM_MODE 0          //  Client machine is using the site-wide default event sources
#define EVENTCMT_CUSTOM_MODE 1          //  Client machine is using a custom set of event sources
#define EVENTCMT_REQUST_MODE 2          //  Client machine was previously using a custom set of event
                                        //  sources but is now waiting for clicfg.exe to reset
                                        //  the event sources to the site-wide default.


//=================================================================================
//
//	class EventConfigModifier
//
//	This class is central to the modification of the trap translation config. It
//	checks the command arguments, connects to and opens the target machine's
//	registry. Handles writing to the eventcmt.log. Reads in the configuration file,
//	builds lists of configuration items to be added/deleted and processes these
//	lists. It starts and stops the SNMP service if necessary and generates a status
//	mif. All that needs to be done is to create an EventConfigModifier object and
//	call the Main method. For method descriptions see the source file EVENTCMT.CPP
//	in project EVNTTRAP\CONFIG.
//
//	NOTE: To create an EventConfigModifier object two parameters are passed to the
//	constructor. The first is a cmmandline, the second an UNC machine name (e.g.
//	\\nadir2).
//															
//	The command line may have several space separated args (as in DOS prompt cmds)
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
//=================================================================================
class EventConfigModifier
{

private:


	//	Private Members
	//	===============

	CString				CommandLine;		//The command line as passed to the constructor
	CString				CommandFile;		//The config file name as taken from CommandLine
	CString				Machine;			//The UNC name of the target machineas passed to
											//the constructor e.g. \\nadir2

#ifdef EVENTCMT_OLD_LOG
	HANDLE				hFile;				//The handle to the log file
	BOOL				closeHandle;		//Indicates whether we need to close the log file
#endif	//EVENTCMT_OLD_LOG

	CommandItemQueue	CommandQ;			//The list of transaltion requests
	CommandItemQueue	TrapQ;				//The list of trap destination requests
	SourceList			SourceL;			//The list of event sources
	UniqueList			CommNames;			//The list of community names and dest. addresses
	BOOL				StatusMifWritten;	//Indicates whethera Status Mif has been written
	BOOL				LogWanted;			//Indicates whether the log file is to be updated
	BOOL				MandatoryMode;		//Indicates the mode to run in
	BOOL				SetCustom;			//Indicates whether to set the mode in the registry to custom
	DWORD				EvCmtReturnCode;	//The return code from Main()
	HKEY				hkey_machine;		//HKEY_LOCAL_MACHINE for the target machine
	BOOL				SNMPinstalled;		//Indicates whether SNMP service is installed
	BOOL				SNMPStopStart;		//Indicates whether we want to Stop/Start the SNMP service
	BOOL				PrevStatus; 		//Indicates whether an ERROR status mif has been written
	BOOL 				SNMPModified;		//Indicates whether the SNMP trap registry has been changed
	BOOL				Printhelp;			//Indicates whether help is to be printed to the console
	
	//	Private Constants
	//	=================

	CString nl;			//newline character
	CString NL;			//either linefeed newline OR newline depending on the WriteToLog method
	CString tab;		//tab character

	
	//	Private Methods
	//	===============

	void				ProcessCommandQ();
	void				ProcessTrapQ();
	void				Load();
	void				ReadConfigIn();
	void				WriteToLog(CString * inbuf);
	void				PrintHelp();

#ifdef EVENTCMT_OLD_LOG
	BOOL				OpenLogFile();
	CString *			GetRegStr(HKEY openhkey, UINT id, CString name);
#endif	//EVENTCMT_OLD_LOG

	CommandItem *		GetCommandArgs(CString * buffer, CString * comline);
	TrapCommandItem *	GetTrapCommandArgs(CString * buffer, CString * comline);
	BOOL				StrToDword(CString * str, DWORD * num);
	CString *			MyStrtok(CString * comline);
	CString *			CStringStrtok(CString * comline);
	BOOL				CheckInstallations();
	BOOL				ReadSNMPRegistry();
	BOOL				WriteSNMPRegistry();
	BOOL				DeleteSNMPRegistry(HKEY * hkey);
	void				StartSNMP();
	void				StopSNMP();
	void 				StatusMif(UINT mess_id, BOOL status);
	BOOL				ProcessCommandLine();
	BOOL				ReadLineIn(HANDLE * h_file, CString * Line, DWORD * startpoint);
	void				SetError(DWORD val);

public:

	//	Public Constructors
	//	===================

			EventConfigModifier(LPCSTR CmdLine, LPCSTR machine = NULL);


	//	Public Methods
	//	==============

	DWORD	Main();

	
	//	Public Destructors
	//	===================

			~EventConfigModifier();

};
