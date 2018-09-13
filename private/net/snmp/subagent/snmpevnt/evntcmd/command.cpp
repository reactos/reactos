//****************************************************************************
//
//  Copyright (c) 1996, Microsoft Corporation
//
//  File:  COMMAND.CPP
//
//  Implementation of the translation command classes and their containers.
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
#include <eventcmd.h>
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif





//============================================================================
//  TrapCommandItem::TrapCommandItem
//
//  This is the TrapCommandItem class's only constructor. This class is derived
//	from the CommandItem class. It is used to store and process individual trap
//	destination configuration requests. The CommandItem constructor is called
//	count, time and eventid default values of 0. The eventlog gets the value of
//	the community name and the eventsource gets the value of the address.
//
//
//  Parameters:
//
//      CommandType comtype  	The requested command type. Currently either
//								AddTrap or DeleteTrap are only supported.
//
//      CString * comm			The community name for the trap destination.
//
//		CString * addr			The network address for the trap destination.
//
//		HKEY key				The registry key HKEY_LOCAL_MACHINE for the
//								machine that is beig configured.
//
//  Returns:
//
//      none
//
//============================================================================

TrapCommandItem::TrapCommandItem(CommandType comtype, CString * comm, CString * addr, HKEY key)
				:CommandItem(comtype, comm, addr, 0, 0, 0, key)
{
	//	Store the community name and address for the request
	//	====================================================	
	
	community = comm;
	address = addr;
}


//============================================================================
//  TrapCommandItem::Process
//
//  This public method is called to process the TrapCommandItem. It modifies
//	the image of the SNMP trap destination registry held in memory. The success
//	of the operation is indicated by setting the second parameter. It returns
//	a message contained in a CString which is to be written to the log.
//
//
//  Parameters:
//
//      UniqueList * CommList  	The image of the SNMP trap config held in
//								memory. This is a list of community names. Each
//								community name having a list of destination
//								addresses. (Therefore a list of lists!)
//
//      ReturnVal * success		An indication of the success of the operation.
//								Currently values reported are:
//								RET_OK	- success
//								RET_BAD	- failure
//								RET_NOT_FOUND	- Duplicate not added OR
//												- Item to be deleted not found
//
//
//  Returns:
//
//      CString * 				A pointer to a CString containing a message
//								to be written in the log. If this is non NULL
//								the CString object being pointed to should be
//								deleted by the calling function after use.
//
//============================================================================

CString * TrapCommandItem::Process(UniqueList * CommList, ReturnVal * success)
{
	CString * retchar = NULL;
	CString * commname = new CString(*community);
	CommListItem * tmp = new CommListItem(commname);
	CString * addr = new CString(*address);
	ListItem * addr_item = new ListItem(addr);
	*success = RET_OK;

	if (GetCommand() == AddTrap)
	{
		tmp->addresses.Add(addr_item);
		CommListItem * tmp2 = (CommListItem *)CommList->Add(tmp);
		
		if (tmp2) //duplicate commname just add the address to it
		{
			addr_item = tmp->addresses.Pop(); //get the address
			delete tmp;						  //delete the duplicate

			if(tmp2->addresses.Add(addr_item)) //add the address
			{
				retchar = WriteToBuff(IDS_MSG1);
				delete addr_item;			//duplicate address delete it
				*success = RET_NOTFOUND;	//indicate duplicate not added
			}

			if(!retchar)
				retchar = WriteToBuff(IDS_MSG2);
		}
 		
 		if(!retchar)
			retchar = WriteToBuff(IDS_MSG3);

	}
	else //DeleteTrap
	{
		
		//	Is the item in the current image?
		//	=================================

		CommListItem * tmp2 = (CommListItem *)CommList->FindItem(tmp, 0);

		if(tmp2)	//if it is in the image it will be removed.
		{
			POSITION p = tmp2->addresses.FindItem(addr_item);
			BOOL removed = FALSE;

			if (p)
			{
				ListItem * del = tmp2->addresses.FindItem(addr_item, 0);
				tmp2->addresses.RemoveAt(p);	//remove from the list
				delete del;						//delete the item from memory
				retchar = WriteToBuff(IDS_MSG4);
				removed = TRUE;
			}
			else
			{
				retchar = WriteToBuff(IDS_MSG5);
				*success = RET_NOTFOUND;
			}

			if (tmp2->addresses.IsEmpty())
			{
				POSITION p2 = CommList->FindItem(tmp);

				if (p2)
				{
					CommListItem * del2 = (CommListItem *)CommList->FindItem(tmp, 0);
					CommList->RemoveAt(p2);	//remove from the list
					delete del2;			//delete the item from memory

					if (retchar)
					{
						delete retchar;
						retchar = WriteToBuff(IDS_MSG6);
					}
					else
						retchar = WriteToBuff(IDS_MSG7);
				}
			}
				 
		}
		else	//not in the image, say so and indicate not found
		{
			retchar = WriteToBuff(IDS_MSG8);
			*success = RET_NOTFOUND;
		}
		
		delete tmp;
		delete addr_item;
	}
	
	return retchar;
}


//============================================================================
//  CommandItem::CommandItem
//
//  This is the CommandItem class's only constructor. This class is derived
//	from the CObject class. This is so the MFC template storage classes can be
//	used. It is used to store and process event to SNMP trap translation
//	configuration requests. 
//
//
//  Parameters:
//
//      CommandType comtype  	The requested command type. Currently either
//								Add or Delete are only supported.
//
//      CString * log			The event log name.
//
//		CString * src			The event source name.
//
//		DWORD cnt				The number of times the event occurs before a
//								trap should be generated.
//
//		DWORD tm				The time interval for the count to be reached.
//
//		HKEY key				The registry key HKEY_LOCAL_MACHINE for the
//								machine that is being configured.
//
//  Returns:
//
//      none
//
//============================================================================

CommandItem::CommandItem(CommandType com, CString * log,
						CString * src, DWORD id,
						DWORD cnt, DWORD tm, HKEY key)
{
	//	Set the configuration values to be set in the registry
	//	======================================================

	command = com;
	eventLog = log;
	eventSource = src;
	eventID = id;
	hkey_machine = key;


	//	If the count is 0 make it 1, a valid value
	//	==========================================

	if(cnt)
		count = cnt;
	else
		count = 1;
	
	time = tm;
}


//============================================================================
//  CommandItem::ModifyRegistry
//
//  This public method is called to process the CommandItem. It modifies the
//	the registry either bt editing, deleting or creating a key. The success
//	of the operation is indicated by setting the second parameter. It returns
//	a message contained in a CString which is to be written to the log.
//
//
//  Parameters:
//
//      SourceItem * srcItem  	A pointer to a structure containing additional
//								information about this command. This needed to
//								process the item.
//
//
//      ReturnVal * success		An indication of the success of the operation.
//								Currently values reported are:
//								RET_OK			- success
//								RET_BAD			- failure
//								RET_NOT_FOUND	- Item to be deleted not found
//
//  Returns:
//
//      CString * 				A pointer to a CString containing a message
//								to be written in the log. If this is non NULL
//								the CString object being pointed to should be
//								deleted after use.
//
//============================================================================

CString * CommandItem::ModifyRegistry(SourceItem * srcItem, ReturnVal * success)
{
	CString * retchar = NULL;
	*success = RET_OK;


	//	Get the command type and process the config request accordingly
	//	===============================================================

	switch(command)
	{
		case Add:		//Do the add process
		{
			retchar = DoAdd(srcItem, success);		//modifies the success parameter
			break;
		}
		
		case Delete:	//Do the delete process
		{
			retchar = DoDelete(srcItem, success);	//modifies the success parameter
			break;
		}
		
		default:  		//Should never get here!
			break;	
	}

	return retchar;		//value returned by DoAdd or DoDelete
}


//============================================================================
//  CommandItem::DoDelete
//
//  This protected method is called to process the CommandItem of type "delete".
//	It modifies the registry by deleting a key. This method should only be
//	called by ModifyRegistry. The success of the operation is indicated by
//	setting the second parameter. It returnsa message contained in a CString
//	which is to be written to the log.
//
//
//  Parameters:
//
//      SourceItem * srcItem  	A pointer to a structure containing additional
//								information about this command. This needed to
//								process the item.
//
//
//      ReturnVal * success		An indication of the success of the operation.
//								Currently values reported are:
//								RET_OK			- success
//								RET_BAD			- failure
//								RET_NOT_FOUND	- Item to be deleted not found
//
//  Returns:
//
//      CString * 				A pointer to a CString containing a message
//								to be written in the log. This should never be
//								NULL and the CString object being pointed to
//								should be deleted after use.
//
//============================================================================

CString * CommandItem::DoDelete(SourceItem * srcItem, ReturnVal * success)
{
	
	CString * retchar = NULL;
	HKEY hkey;
 	CString keyname;
	char evidstr[34];
	_ultoa(eventID, evidstr, 10); 
	keyname.LoadString(IDS_SNMP_AGENT);
	CString DirSep;
	DirSep.LoadString(IDS_DIRSEP);
	keyname += DirSep;
	keyname += *eventSource;
	keyname += DirSep;
	keyname += evidstr;
	
	
	//	Open the key we need to delete so we can delete sub-keys
	//	========================================================

	LONG result = RegOpenKeyEx(hkey_machine, keyname,
								0, KEY_ALL_ACCESS, &hkey);

	if(result != ERROR_SUCCESS)
	{
	
		//	Failed to open the key either it's not there or we have an error
		//	================================================================

		if(result == ERROR_FILE_NOT_FOUND)
			*success = RET_NOTFOUND;
		else
			*success = RET_BAD;

		return(WriteToBuff(IDS_MSG9));
	}

	
	//	Enumerate and delete all subkeys, then close and delete this key
	//	================================================================

	retchar = DeleteSubKeys(&hkey);
	RegCloseKey(hkey);
	
	if(!retchar)	//Deleting all sub-keys worked.
	{
		result = RegDeleteKey(hkey_machine, keyname);
	
		if(result != ERROR_SUCCESS)
		{
			*success = RET_BAD;
			return(WriteToBuff(IDS_MSG10));
		}

		retchar = WriteToBuff(IDS_MSG11);

		
		//	If the last entry for this source is deleted, delete the source
		//	===============================================================

 		CString keyN;
		keyN.LoadString(IDS_SNMP_AGENT);
		keyN += DirSep;
		keyN += *eventSource;
		result = RegOpenKeyEx(hkey_machine, keyN,
								0, KEY_ALL_ACCESS, &hkey);

        if(result == ERROR_SUCCESS)	//this should always be true
		{
        	char Buffer[1024 + 1];
			DWORD dwLength = 1024 +1;
        	result = RegEnumKeyEx(hkey, 0, Buffer, &dwLength, NULL,
            					NULL, NULL, NULL);
			RegCloseKey(hkey);
			
			if (result == ERROR_NO_MORE_ITEMS) //we can delete it!
			{
				RegDeleteKey(hkey_machine, keyN);				
			}
		}
	}
	else		//Deleteing all sub-keys failed
	{
		*success = RET_BAD;
	}

	return retchar;
}


//============================================================================
//  CommandItem::DeleteSubKeys
//
//  This protected method is called to delete all subkeys under the registry
//	key passed as the only parameter. It returns a apointer to a CString which
//	contains a message, if there was an error, which is to be written to the
//	log. This method is recursive...it calls  DeleteKey which in turn calls
//	this method.
//
//
//  Parameters:
//
//      HKEY * hKey  	A pointer to the registry key whose subkeys are to be
//						deleted.
//
//  Returns:
//
//      CString * 		A pointer to a CString containing an error message to
//						be written in the log. This should be NULL if there
//						were no errors. The CString object being pointed to
//						should be deleted after use.
//
//============================================================================

CString * CommandItem::DeleteSubKeys(HKEY * hKey)
{
    CString * retchar = NULL;
    char Buffer[1024 + 1];
    DWORD dwLength;
    int i = 0;

    LONG Result = ERROR_SUCCESS;

    
    //	Enumerate all subkeys and delete them
	//	=====================================

    while (TRUE)
    {
        dwLength = 1024 + 1;
        Result = RegEnumKeyEx(*hKey, i, Buffer, &dwLength, NULL,
            NULL, NULL, NULL);

        if (Result != ERROR_SUCCESS)
            break;

        if (dwLength > 0)
		{
            CString CBuffer(Buffer);
    

    		//	This will delete the key
			//	========================

            retchar = DeleteKey(hKey, &CBuffer);
			
			if (retchar)		//there was an error
				return retchar;
		}

        i++;
    }


    //	Did we find a normal end condition?
	//	===================================

    if (Result == ERROR_NO_MORE_ITEMS)
        return retchar;					//no errors

    return (WriteToBuff(IDS_MSG12));	//log an error
}


//============================================================================
//  CommandItem::DeleteKey
//
//  This protected method is called to delete the registry key indicateded by
//	the second parameter. The first parameter is an open registry key, the
//	second parameter is the keyname (full path relative to the first parameter).
//	This method returns a pointer to a CString which contains a message, if
//	there was an error, which is to be written to the log.
//
//
//  Parameters:
//
//      HKEY * hKey  	An open registry key.
//
//		CString * key	The name of the key to be deleted (full path relative
//						to the first parameter).
//
//  Returns:
//
//      CString * 		A pointer to a CString containing an error message to
//						be written in the log. This should be NULL if there
//						were no errors. The CString object being pointed to
//						should be deleted after use.
//
//============================================================================

CString * CommandItem::DeleteKey(HKEY * hKey, CString * key)
{
    CString * retchar = NULL;
	HKEY hk;
	

	//	Open the key to be deleted so we can tell if it has any subkeys
	//	===============================================================

	LONG result = RegOpenKeyEx(*hKey, *key,
								0, KEY_ALL_ACCESS, &hk);
	if(result != ERROR_SUCCESS)
	{
		return(WriteToBuff(IDS_MSG13));
	}


	//	Enumerate and delete all subkeys, then close and delete this key
	//	================================================================

	retchar = DeleteSubKeys(&hk);

	RegCloseKey(hk);

	if(!retchar)
	{
		result = RegDeleteKey(*hKey, *key);	//delete the key
	
		if(result != ERROR_SUCCESS)
		{
			return(WriteToBuff(IDS_MSG14));
		}
	}

	return retchar;
}


//============================================================================
//  CommandItem::DoDelete
//
//  This protected method is called to process the CommandItem of type "add".
//	It modifies the registry by editing or creating a key. This method should
//	only be called by ModifyRegistry. The success of the operation is indicated
//	by setting the second parameter. It returns a pointer to a CString which
//	contains a message to be written to the log.
//
//
//  Parameters:
//
//      SourceItem * srcItem  	A pointer to a structure containing additional
//								information about this command. This needed to
//								process the item.
//
//
//      ReturnVal * success		An indication of the success of the operation.
//								Currently values reported are:
//								RET_OK			- success
//								RET_BAD			- failure
//
//  Returns:
//
//      CString * 				A pointer to a CString containing a message
//								to be written in the log. This should never be
//								NULL and the CString object being pointed to
//								should be deleted after use.
//
//============================================================================

CString * CommandItem::DoAdd(SourceItem * srcItem, ReturnVal * success)
{
	CString * retchar = NULL;
	HKEY hkey;
	DWORD createtype = REG_OPENED_EXISTING_KEY;
 	CString keyname;
	keyname.LoadString(IDS_SNMP_AGENT);
	CString DirSep;
	DirSep.LoadString(IDS_DIRSEP);
	keyname += DirSep;
	keyname += *eventSource;


	//	Open the registry at the place where event config is
	//	====================================================

	LONG result = RegOpenKeyEx(hkey_machine, keyname,
								0, KEY_ALL_ACCESS, &hkey);

    if (result != ERROR_SUCCESS)
	{
	
		//	Open or create the event source key to be modified
		//	==================================================

		result = RegCreateKeyEx(hkey_machine, keyname, 0,
								NULL, REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS, NULL, &hkey, &createtype);
		
		if(result != ERROR_SUCCESS)
		{
			retchar = WriteToBuff(IDS_MSG15);
			*success = RET_BAD;
			return retchar;
		}


		//	Make sure this has the correct settings
		//	=======================================

		CString ValueApp;
		DWORD app = 1;
		ValueApp.LoadString(IDS_APPEND);
		result = RegSetValueEx(hkey, ValueApp, 0, REG_DWORD,
								(CONST BYTE *)&app, 4);

		if(result != ERROR_SUCCESS)
		{
			retchar = WriteToBuff(IDS_MSG15);
		}

		CString ValueEntOID;
		ValueEntOID.LoadString(IDS_ENTOID);
		CMyString * entoid = (CMyString *)srcItem->GetEntOID();
		LPTSTR buff2 = entoid->GetBuffer(1);
		DWORD buff2sz = entoid->GetBufferSize();
		result = RegSetValueEx(hkey, ValueEntOID, 0, REG_SZ,
								(CONST BYTE *)buff2, buff2sz);
		entoid->ReleaseBuffer();

		if(result != ERROR_SUCCESS)
		{
			retchar = WriteToBuff(IDS_MSG15);
		}

		if(retchar)
		{
			RegCloseKey(hkey);

			if(createtype == REG_CREATED_NEW_KEY)
				RegDeleteKey(hkey_machine, keyname);

			*success = RET_BAD;
			return retchar;
		}
	}

	DWORD createtype2 = REG_OPENED_EXISTING_KEY;
	char evidstr[34];
	_ultoa(eventID, evidstr, 10); 
	HKEY hkey2;


	//	Open or create the event ID key to be modified
	//	==============================================

	result = RegCreateKeyEx(hkey, evidstr, 0,
							NULL, REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hkey2, &createtype2);
		
	if(result != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);

		if(createtype == REG_CREATED_NEW_KEY)
			RegDeleteKey(hkey_machine, keyname);
	
		*success = RET_BAD;
		return(WriteToBuff(IDS_MSG15));
	}

	CString ValueCnt;
	ValueCnt.LoadString(IDS_COUNT);

	
	//	Set the count
	//	=============
	
	result = RegSetValueEx(hkey2, ValueCnt, 0, REG_DWORD,
							(CONST BYTE *)&count, 4);

	if(result != ERROR_SUCCESS)
	{
		RegCloseKey(hkey2);

		if(createtype2 == REG_CREATED_NEW_KEY)
			RegDeleteKey(hkey, evidstr);

		RegCloseKey(hkey);

		if(createtype == REG_CREATED_NEW_KEY)
			RegDeleteKey(hkey_machine, keyname);

		*success = RET_BAD;
		return(WriteToBuff(IDS_MSG15));
	}

	CString ValueT;
	ValueT.LoadString(IDS_TIME);


	//	If necessary, set the time
	//	==========================

	if ((count > 1) && time)
	{
		result = RegSetValueEx(hkey2, ValueT, 0, REG_DWORD,
								(CONST BYTE *)&time, 4);
		if(result != ERROR_SUCCESS)
		{
			RegCloseKey(hkey2);
		
			if(createtype2 == REG_CREATED_NEW_KEY)
				RegDeleteKey(hkey, evidstr);

			RegCloseKey(hkey);

			if(createtype == REG_CREATED_NEW_KEY)
				RegDeleteKey(hkey_machine, keyname);

			*success = RET_BAD;
			return(WriteToBuff(IDS_MSG15));
		}
	}
	else
		RegDeleteValue(hkey2, ValueT);

	CString ValueID;
	ValueID.LoadString(IDS_FULLID);

	
	//	Set the fullid value
	//	====================
	
	result = RegSetValueEx(hkey2, ValueID, 0, REG_DWORD,
							(CONST BYTE *)&eventID, 4);

	if(result != ERROR_SUCCESS)
	{
		RegCloseKey(hkey2);

		if(createtype2 == REG_CREATED_NEW_KEY)
			RegDeleteKey(hkey, evidstr);

		RegCloseKey(hkey);

		if(createtype == REG_CREATED_NEW_KEY)
			RegDeleteKey(hkey_machine, keyname);

		*success = RET_BAD;
		return(WriteToBuff(IDS_MSG15));
	}
	
	RegCloseKey(hkey);
	RegCloseKey(hkey2);

	return (WriteToBuff(IDS_MSG16));
}


//============================================================================
//  CommandItem::WriteToBuff
//
//  This public method is called to format a message to be written to the log.
//	It's only parameter is the resource id of a string to be appended to the
//	command line from which this CommandItem was built.
//
//
//  Parameters:
//
//      UINT mssgid  	A pointer to a structure containing additional
//								information about this command. This needed to
//								process the item.
//
//  Returns:
//
//      CString * 		A pointer to a CString containing a message
//						to be written in the log. This should never be
//						NULL and the CString object being pointed to
//						should be deleted after use.
//
//============================================================================

CString * CommandItem::WriteToBuff(UINT mssgid)
{
	CString * s = new CString;
	CString temp;
	temp.LoadString(IDS_PRAGMA);
	CString tab;
	tab.LoadString(IDS_TAB);
	CString quote;
	quote.LoadString(IDS_QUOTE);


	//	First build the command line that is CommandItem was made from
	//	==============================================================
	
	*s += temp;
	*s += tab;
	temp.Empty();
	BOOL trap = FALSE;
	
	switch (command)
	{
		case Add:
		{
			temp.LoadString(IDS_ADD);
			break;
		}

		case Delete:
		{
			temp.LoadString(IDS_DEL);
			break;
		}

		case AddTrap:
		{
			trap = TRUE;
			temp.LoadString(IDS_ADDTRAP);
			break;
		}

		case DeleteTrap:
		{
			trap = TRUE;
			temp.LoadString(IDS_DELTRAP);
			break;
		}

		default:  //huh!?!
			break;
	}
	
	*s += temp;
	*s += tab;
	temp.Empty();

	*s += quote;
	*s += *eventLog;
	*s += quote;
	*s += tab;
	*s += quote;
	*s += *eventSource;
	*s += quote;

	if (!trap)	//not a trap command, has eventid, count and time
	{
		*s += tab;
		char tmp[34];
		_ultoa(eventID, tmp, 10); 
		*s += tmp;

		if (count)
		{
			*s += tab;
			char tmp[34];
			_ultoa(count, tmp, 10); 
			*s += tmp;

			if (time)
			{
				*s += tab;
				char tmp[34];
				_ultoa(time, tmp, 10); 
				*s += tmp;
			}
		}
	}
	

	//	Add the message from the resource specified by this method's parameter
	//	======================================================================

	if (mssgid)
	{
		CString messg;
		messg.LoadString(mssgid);
		*s += tab;
		*s += messg;
	}

	CString NL;

#ifdef EVENTCMT_OLD_LOG

	NL.LoadString(IDS_LFCR);

#else	//EVENTCMT_OLD_LOG

	NL.LoadString(IDS_NL);

#endif	//EVENTCMT_OLD_LOG

	*s += NL;

	
	//	Return the CString object that we've built
	//	==========================================
	
	return s;
}


//============================================================================
//  CommandItem::~CommandItem
//
//  This is the CommandItem class's only desstructor. It frees the memory used
//	to the two CString members	eventLog and eventSource. (These are the same
//	as commname and address for the  TrapCommandItem so they are not deleted in
//	the TrapCommandItem destructor.)
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

CommandItem::~CommandItem()
{
	delete eventLog;
	delete eventSource;	
}


//============================================================================
//  CommandItemQueue::~CommandItemQueue
//
//  This is the CommandItemQueue class's only desstructor. If there are any
//	items in the queue they are deleted.
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

CommandItemQueue::~CommandItemQueue()
{
	while(!IsEmpty())
	{
		CommandItem * com = RemoveHead();
		delete com;
	}
}


//============================================================================
//  SourceItem::SourceItem
//
//  This is the SourceItem class's only constructor. This class is derived
//	from the CObject class. This is so the MFC template storage classes can be
//	used. It is used to store and obtain event source information so that SNMP
//	trap translation configuration requests may be processed.
//
//
//  Parameters:
//
//		CString * src	The event source name.
//
//		HKEY key		The registry key HKEY_LOCAL_MACHINE for the	machine
//						that is being configured.
//
//		BOOL E			A boolean indicating whether this Source is valid.
//
//
//  Returns:
//
//      none
//
//============================================================================

SourceItem::SourceItem(CString * src, HKEY key, BOOL E)
{
	EntOID = NULL;
	hkey_machine = key;

	if(src)	
	{
		source = new CString(*src);
	}
	else
		source = NULL;

	
	//	This sets the member Exists variable and if necessary creates an OID for the source
	//	===================================================================================
	
	SetExists(E);
}


//============================================================================
//  SourceItem::SetExists
//
//  This public method is called to set the Exist member variable. It takes
//	one parameter, the value to be set. If needed, it creates an OID string
//	for the event source.
//
//
//  Parameters:
//
//		BOOL E			A boolean indicating whether this Source is valid.
//
//
//  Returns:
//
//      none
//
//============================================================================

void SourceItem::SetExists(BOOL E)
{

	//	delete the old OID if there was one
	//	===================================

	if (EntOID)
		delete EntOID;

	
	//	if needed, create a new OID
	//	===========================

	if (E)
	{
		CString OID;
		_ultoa(source->GetLength(), OID.GetBuffer(20), 10);
		OID.ReleaseBuffer();
		OID += TEXT(".");
		char * c = source->GetBuffer(1);

		while (c && (*c != '\0'))
		{
			int a = *c;
			char str[34];
			_ultoa(a, str, 10); 
			OID += str;
			OID += TEXT(".");
			c++;
		}

		source->ReleaseBuffer();
		EntOID = new CString(OID);
	}
	else
		EntOID = NULL;

	
	//	set the Exists member
	//	=====================

	Exists = E;
}

#ifdef EVENTCMT_VALIDATE_ID


//============================================================================
//  SourceItem::GetDllName
//
//  This public method is called to get the name of the message dll for this
//	event source.
//
//
//  Parameters:
//
//		CommandItem* Cmnd	A pointer to a CommandItem which has this event
//							source
//
//
//  Returns:
//
//      CString *			A pointer to a CString containing the dll name
//
//============================================================================

CString * SourceItem::GetDllName(CommandItem* Cmnd)
{

	//	Open the registry and get the key for the event source specified
	//	================================================================

	CString * Dll = NULL;
	HKEY hkeyOpen;
	CString keyName;
	CString keyValue;
	CString DirSep;
	DirSep.LoadString(IDS_DIRSEP);
	DWORD valType;
	char * buff;
	char * buff2;
	DWORD buffsz = 0;

	keyName.LoadString(IDS_KEYNAME);
	CString * evlog = Cmnd->GetEventLog(); 
	keyName += *evlog;
	keyName += DirSep; 
	CString * evsrc = Cmnd->GetEventSource(); 
	keyName += *evsrc;


	//	Get the key to the source from the registry
	//	===========================================

	LONG Result = RegOpenKeyEx(hkey_machine, keyName,
								0, KEY_ALL_ACCESS, &hkeyOpen);

    if (Result != ERROR_SUCCESS)
        return Dll;

	
	//	Get the name and path to the message dll
	//	========================================

	keyValue.LoadString(IDS_VALUENAME);
	Result = RegQueryValueEx(hkeyOpen, keyValue, NULL, &valType, (unsigned char*)buff, &buffsz);

    if (!buffsz)//(Result != ERROR_MORE_DATA)
	{
		RegCloseKey(hkeyOpen);        
        return Dll;
	}

	
	//	first we found the size of the buffer needed, now get the name of the dll
	//	=========================================================================

	buff = new char[buffsz + 1];
	Result = RegQueryValueEx(hkeyOpen, keyValue, NULL, &valType, (unsigned char*)buff, &buffsz);
	RegCloseKey(hkeyOpen);        
    
    if (Result != ERROR_SUCCESS)
	{
		delete buff;
		return Dll;
	}
	
	buff2 = new char[1];
	
	
	//	Remove any environment strings (e.g. %SystemRoot%)
	
	DWORD length = ExpandEnvironmentStrings(buff, buff2, 1);
	
	if (!length)
	{
		delete buff;
		return Dll;
	}

	delete buff2;
	buff2 = new char[length + 1];
	buffsz = ExpandEnvironmentStrings(buff, buff2, length);

	delete buff;

	if(!buffsz)
	{
		delete buff2;
		return Dll;
	}
	
	
	//	Create the CString for the return
	//	=================================
	
	Dll = new CString(buff2);
	delete buff2;
	return Dll;
}


//============================================================================
//  SourceItem::EnumerateEventIDMap
//
//  This public method is called to get read and enumerate the message dll.
//	This is so EventId values may be validated.
//
//
//  Parameters:
//
//		CommandItem* Cmnd	A pointer to a CommandItem which has this event
//							source
//
//
//  Returns:
//
//      BOOL				An indication of the success of this method. FALSE
//							if there was an error.
//
//============================================================================

BOOL SourceItem::EnumerateEventIDMap(CommandItem* Cmnd)
{
	CString * dllname;
	dllname = GetDllName(Cmnd);

	if (!dllname)
		return FALSE;

    
	//	Load the message dll
	//	====================

    HINSTANCE hInstMsgFile = LoadLibraryEx(*dllname, NULL, 
        LOAD_LIBRARY_AS_DATAFILE);
	delete dllname;

    if (hInstMsgFile == NULL)
        return FALSE;
  
    
	//	Enumerate the message dll
	//	=========================
    
    if (EnumResourceNames(hInstMsgFile, RT_MESSAGETABLE, 
        (ENUMRESNAMEPROC)ProcessMsgTable, (LONG)this) == FALSE)
    {
        FreeLibrary(hInstMsgFile);
        return FALSE;
    }

    FreeLibrary(hInstMsgFile);
    return TRUE;
}


//============================================================================
//  BOOL CALLBACK ProcessMsgTable
//
//  This callback function is used to read and process an open message dll for
//	event IDs. See WINDOWS API for EnumResourceNames and EnumResNameProc.
//
//============================================================================

BOOL CALLBACK ProcessMsgTable(HANDLE hModule, LPCTSTR lpszType,
    LPTSTR lpszName, LONG lParam)
{

    SourceItem* src = (SourceItem*)(LPVOID)lParam;


    //	Found a message table.  Process it!
	//	===================================

    HRSRC hResource = FindResource((HINSTANCE)hModule, lpszName,
        RT_MESSAGETABLE);
    if (hResource == NULL)
        return TRUE;

    HGLOBAL hMem = LoadResource((HINSTANCE)hModule, hResource);
    if (hMem == NULL)
        return TRUE;

    PMESSAGE_RESOURCE_DATA pMsgTable =
        (PMESSAGE_RESOURCE_DATA)::LockResource(hMem);
    if (pMsgTable == NULL)
        return TRUE;

    ULONG ulBlock, ulId, ulOffset;

    for (ulBlock=0; ulBlock<pMsgTable->NumberOfBlocks; ulBlock++)
    {
        ulOffset = pMsgTable->Blocks[ulBlock].OffsetToEntries;
        for (ulId = pMsgTable->Blocks[ulBlock].LowId;
            ulId <= pMsgTable->Blocks[ulBlock].HighId; ulId++)
                
        {
            src->eventIDs.SetAt((DWORD)ulId, 0);
        }
    }

    return TRUE;
}

#endif //EVENTCMT_VALIDATE_ID


//============================================================================
//  SourceItem::~SourceItem
//
//  This is the SourceItem class's only desstructor. It frees any memory
//	to the source name, the OID and the map of valid eventids.
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

SourceItem::~SourceItem()
{
	if (source)
		delete source;

	if (EntOID)
		delete EntOID;

#ifdef EVENTCMT_VALIDATE_ID
	eventIDs.RemoveAll();
#endif //EVENTCMT_VALIDATE_ID

}


//============================================================================
//  SourceList::~SourceList
//
//  This is the SourceList class's only desstructor. If there are any items in
//	the queue they are deleted.
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

SourceList::~SourceList()
{
	while(!IsEmpty())
	{
		SourceItem * src = RemoveHead();
		delete src;
	}
}


//============================================================================
//  SourceList::FindItem
//
//  This public method is called to find a particular SourceItem in this
//	SourceList. The search is done by name.
//
//
//  Parameters:
//
//		CString * src	A pointer to a CString which has the event source
//						name being sought.
//
//		SourceItem * i	A pointer to the source item if it was found. This is
//						NULL if the item wasn't found.
//
//
//  Returns:
//
//      int				An indication of the success of this method. This is
//						one of three values:
//						RET_OK			- The item is a valid event source
//						RET_BAD			- The item is an invalid event source
//						RET_NOT_FOUND	- The item was not found
//
//============================================================================

int SourceList::FindItem(CString * src, SourceItem *& i)
{
	POSITION p = GetHeadPosition();
	ReturnVal ret = RET_NOTFOUND;
	i = NULL;


	//	Step through the list searching for the event source
	//	====================================================
		
	while (src && p) 
	{
		SourceItem * item = GetNext(p);
		CString * temp = item->GetSource();

		if(*temp == *src)
		{
			
			//	The item has been found!
			//	========================
			
			i = item;

			if(item->GetExists())
				ret = RET_OK;	//valid event source
			else
				ret = RET_BAD;	//invalid event source

			break;
		}
	}

	return ret;
}


//============================================================================
//  UniqueList::~UniqueList
//
//  This is the UniqueList class's only desstructor. If there are any items in
//	the queue they are deleted.
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

UniqueList::~UniqueList()
{
	while(!IsEmpty())
	{
		ListItem * item = RemoveHead();
		delete item;
	}
}


//============================================================================
//  UniqueList::FindItem
//
//  This public method is called to find a particular ListItem in this
//	UniqueList. The search is done by comparing the contents of the ListItem
//	pointed to by the first parameter (using ListItem::CompareFunc).
//
//
//  Parameters:
//
//		ListItem * item		A pointer to a ListItem to be found. 
//
//		int dummy			A dummy variable to distinguish this overloaded
//							function.
//
//  Returns:
//
//      ListItem *			A pointer to the item found in the list. NULL if
//							the item is not found.
//
//============================================================================

ListItem * UniqueList::FindItem(ListItem * item, int dummy)
{
	
	//	Reset the list and step through it trying to find the item
	//	==========================================================	
	
	POSITION p = GetHeadPosition();
	ListItem * ret = NULL;

	while (item && p) 
	{
		ListItem * i = GetNext(p);

		if(i->CompareFunc(item) == 0)
		{
			ret = i;	//found it!
			break;
		}
	}

	return ret;
}


//============================================================================
//  UniqueList::FindItem
//
//  This public method is called to find a particular ListItem in this
//	UniqueList. The search is done by comparing the contents of the ListItem
//	pointed to by the first parameter (using ListItem::CompareFunc).
//
//
//  Parameters:
//
//		ListItem * item		A pointer to a ListItem to be found. 
//
//  Returns:
//
//      POSITION			The POSITION of the item in the list. NULL if the
//							item is not found.
//
//============================================================================

POSITION UniqueList::FindItem(ListItem * item)
{
	POSITION p = GetHeadPosition();
	POSITION ret = p;

	if (!item)
		return NULL;

	
	//	Reset the list and step through it trying to find the item
	//	==========================================================	
	
	while (item && p) 
	{
		ret = p;  //the current position
		ListItem * i = GetNext(p);

		if(i->CompareFunc(item) == 0)
		{
			//the current position will be returned
			break;
		}
		
		ret = p; //if not found, p will be null and be returned.
	}

	return ret;
}


//============================================================================
//  UniqueList::Add
//
//  This public method is called to add a ListItem into this UniqueList. The
//	ListItem is not added if it is already present in the list. If this is the
//	case a pointer to the matching item in the list is returned.
//
//
//  Parameters:
//
//		ListItem * newItem	A pointer to the ListItem to be added. 
//
//  Returns:
//
//      ListItem *			A pointer to the item if is in the list. NULL if
//							the item was not found and was added.
//
//============================================================================

ListItem * UniqueList::Add(ListItem* newItem)
{
	ListItem * ret = NULL;
	ret = FindItem(newItem, 0);

	if(ret)
		return ret;
	else
		AddTail(newItem);

	return ret;
}


//============================================================================
//  ListItem::ListItem
//
//  This is the ListItem class's only constructor. This class is derived from
//	the CObject class. This is so the MFC template storage classes can be
//	used. It is used to store a CString value in a list.
//
//
//  Parameters:
//
//      CString * str  	A pointer to the CString object to be stored.
//
//  Returns:
//
//      none
//
//============================================================================

ListItem::ListItem(CString * str)
{
	string = str;
}


//============================================================================
//  ListItem::~ListItem
//
//  This is the ListItem class's only destructor. It frees the memory used
//	to store the CString members.
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

ListItem::~ListItem()
{
	delete string;
}


//============================================================================
//  ListItem::CompareFunc
//
//  This public method is used to compare the contents of the ListItem pointed
//	to by the first parameter to the contents of this ListItem.
//
//
//  Parameters:
//
//      ListItem * item		A pointer to the ListItem whose contents will be
//							compared to this ListItem's contents.
//
//  Returns:
//
//      int					An indication of the outcome of the comparison. 0
//							if there is a match, 1 otherwise.
//
//============================================================================

int ListItem::CompareFunc(ListItem * item)
{
	CString * temp = item->GetString();

	if(*temp == *string)
		return 0;
	else
		return 1;
}


//============================================================================
//  CommListItem::CommListItem
//
//  This is the CommListItem class's only constructor. This class is derived
//	from the ListItem class. It is basically a ListItem with a ListItem member
//	as well as a CString. It is used to store a CString value and a list of
//	associated CStrings in a list (a list of lists). This class is used to
//	store the SNMP trap destination registry image. The CString being a
//	community name and the list of CStrings being the destination addresses. A
//	special destructor is not needed as the ListItem destructor will be used
//	once for each member in the list and is adequate. 
//
//
//  Parameters:
//
//      CString * str  	A pointer to the CString object to be stored.
//
//  Returns:
//
//      none
//
//============================================================================

CommListItem::CommListItem(CString * str)
			:ListItem(str)
{
//nothing to do
}


//============================================================================
//  CommListItem::GetAddresses
//
//  This public method is called to get the trap destination addresses for the
//	community name that is stored in this object's CString member.
//
//
//  Parameters:
//
//      HKEY * hkey  	A pointer to the open registry key of the SNMP trap
//						destination root key.
//
//  Returns:
//
//      BOOL 			A boolean indicating the success of the method. TRUE
//						if there were no errors, FALSE otherwise.
//
//============================================================================

BOOL CommListItem::GetAddresses(HKEY * hkey)
{
	HKEY hkeyOpen;

	CString *temp = GetString();

	//	Open the registry key for this community name
	//	=============================================
	LONG Result = RegOpenKeyEx(*hkey, *temp,
								0, KEY_READ, &hkeyOpen);
    
    if (Result != ERROR_SUCCESS)
        return FALSE;

    char Name[1024 + 1];
    DWORD NameLength;
    char Data[1024 + 1];
    DWORD DataLength;
    DWORD type;

    int i = 0;


	//	Enumerate all the values (addresses) under this key
	//	===================================================

    while (TRUE)
    {
        NameLength = 1024 + 1;
        DataLength = 1024 + 1;
        Result = RegEnumValue(hkeyOpen, i, Name, &NameLength, NULL,
            					&type, (unsigned char*)Data, &DataLength);
        
        if (Result != ERROR_SUCCESS)
            break;
        
        if ((NameLength > 0) && (DataLength > 0) && (type == REG_SZ)) 
		{
			CString * str = new CString(Data); 
			ListItem * item = new ListItem(str);
			
			if(addresses.Add(item)) //shouldn't be duplicates but check anyway
				delete item;
        }
        
        i++;
    }

	RegCloseKey(hkeyOpen);


    //	Did we find a normal end condition?
	//	===================================
    if (Result == ERROR_NO_MORE_ITEMS)
        return TRUE;

    return FALSE;

}
