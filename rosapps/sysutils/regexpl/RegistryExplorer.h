
#ifndef _REGISTRY_EXPLORER_H__INCLUDED
#define _REGISTRY_EXPLORER_H__INCLUDED

#define CURRENT_VERSION		_T("0.20")
#define EMAIL				_T("registryexplorer@yahoo.com")

//#define __L(x)      L ## x
//#define _L(x)      __L(x)
/*
#define HELLO_MSG(date)		_T("Registry Explorer for Microsoft Windows NT 4.0\n")\
							_T("Version ") CURRENT_VERSION _T(" Compiled on ") date _T("\n")\
							_T("Coded by Nedko Arnaoudov. This product is freeware.\n\n")
*/

#define PRODUCT_NAME		_T("Registry Explorer")

#define HELLO_MSG			_T("ReactOS ") PRODUCT_NAME _T(" version ") CURRENT_VERSION _T("\n")\
							_T("License is GNU GPL. Type `help gpl` for details.\n\n")

#define NOWARANTY_MSG		_T("Registry Explorer comes with ABSOLUTELY NO WARRANTY\n")\
							_T("This is free software, and you are welcome to redistribute it\n")\
							_T("under certain conditions; type `help gpl' for details.\n")

#define COPYRIGHT_MSG		_T("Copyright (C) 2000-2001 Nedko Arnaoudov\n")

#define CREDITS_MSG			_T("Registry Explorer is written by Nedko Arnaoudov\n")\
							_T("Special thanks to Emanuele Aliberti\n")

#define BUGS_MSG			_T("Please report bugs to ") EMAIL _T("\n")\
							_T("Updates are available at http://www.geocities.com/registryexplorer/\n")\
							_T("and as part of ReactOS http://www.reactos.com\n")

#define VER_MSG				COPYRIGHT_MSG\
							_T("\n")\
							NOWARANTY_MSG\
							_T("\n")\
							CREDITS_MSG\
							_T("\n")\
							BUGS_MSG

#define GOODBYE_MSG			_T("\nThank you for using Registry Explorer !!!\n")

//#define COMMAND_LENGTH(cmd)	(sizeof(DIR_CMD)-sizeof(TCHAR))/sizeof(TCHAR)
#define COMMAND_LENGTH(cmd)	_tcslen(cmd)

#define PROMPT_BUFFER_SIZE	1024

#define COMMAND_NA_ON_ROOT	_T(" is not applicable to root key.\n")

#endif //#ifndef _REGISTRY_EXPLORER_H__INCLUDED
