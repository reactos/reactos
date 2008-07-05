/* $Id$ */

#ifndef _REGISTRY_EXPLORER_H__INCLUDED
#define _REGISTRY_EXPLORER_H__INCLUDED

#define CURRENT_VERSION     _T("0.20+")
#define EMAIL               _T("nedko@users.sourceforge.net")

//#define __L(x)      L ## x
//#define _L(x)      __L(x)

#define PRODUCT_NAME        _T("Registry Explorer")

#define HELLO_MSG           _T("ReactOS ") PRODUCT_NAME _T(" version ") CURRENT_VERSION _T("\n")\
                            _T("License is GNU GPL. Type `help gpl` for details.\n\n")

#define NOWARANTY_MSG       _T("Registry Explorer comes with ABSOLUTELY NO WARRANTY\n")\
                            _T("This is free software, and you are welcome to redistribute it\n")\
                            _T("under certain conditions; type `help gpl' for details.\n")

#define COPYRIGHT_MSG       _T("Copyright (C) 2000-2005 Nedko Arnaudov\n")

#define CREDITS_MSG         _T("Registry Explorer is written by Nedko Arnaudov\n")\
                            _T("Special thanks to ReactOS team.\n")

#define BUGS_MSG            _T("Please report bugs to ") EMAIL _T("\n")

#define VER_MSG             COPYRIGHT_MSG\
                            _T("\n")\
                            NOWARANTY_MSG\
                            _T("\n")\
                            CREDITS_MSG\
                            _T("\n")\
                            BUGS_MSG

#define GOODBYE_MSG         _T("\nThank you for using Registry Explorer !!!\n")

//#define COMMAND_LENGTH(cmd)   (sizeof(DIR_CMD)-sizeof(TCHAR))/sizeof(TCHAR)
#define COMMAND_LENGTH(cmd) _tcslen(cmd)

#define PROMPT_BUFFER_SIZE  1024

#define COMMAND_NA_ON_ROOT  _T(" is not applicable to root key.\n")

#define SETTINGS_REGISTRY_KEY  _T("Software\\Registry Explorer\\")
#define NORMAL_TEXT_ATTRIBUTES_VALUE_NAME        _T("Normal Text Color")
#define COMMAND_TEXT_ATTRIBUTES_VALUE_NAME       _T("Command Text Color")
#define PROMPT_VALUE_NAME                        _T("Prompt")

#define DEFAULT_ESCAPE_CHAR                      _T('^')

#endif //#ifndef _REGISTRY_EXPLORER_H__INCLUDED
