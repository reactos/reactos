
#ifndef _REGISTRY_EXPLORER_H__INCLUDED
#define _REGISTRY_EXPLORER_H__INCLUDED

#define CURRENT_VERSION		_T("0.10")
#define EMAIL				_T("registryexplorer@yahoo.com")

//#define __L(x)      L ## x
//#define _L(x)      __L(x)
/*
#define HELLO_MSG(date)		_T("Registry Explorer for Microsoft Windows NT 4.0\n")\
							_T("Version ") CURRENT_VERSION _T(" Compiled on ") date _T("\n")\
							_T("Coded by Nedko Arnaoudov. This product is freeware.\n\n")
*/
#define HELLO_MSG			_T("Registry Explorer version ") CURRENT_VERSION _T(" for ReactOS\n")\
							_T("Coded by Nedko Arnaoudov. Licence is GNU GPL.\n\n")

#define VER_MSG				_T("Please report bugs to ") EMAIL _T("\n")

#define GOODBYE_MSG			_T("\nThank you for using Registry Explorer !!!\n")

#define COMMAND_NA_ON_ROOT	_T(" command is not applicable to root key.\n")

//#define COMMAND_LENGTH(cmd)	(sizeof(DIR_CMD)-sizeof(TCHAR))/sizeof(TCHAR)
#define COMMAND_LENGTH(cmd)	_tcslen(cmd)

#define PROMPT_BUFFER_SIZE	1024

#endif //#ifndef _REGISTRY_EXPLORER_H__INCLUDED
