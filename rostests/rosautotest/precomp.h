/* General includes */
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/rtlfuncs.h>
#include <reason.h>
#include <shlobj.h>
#include <wininet.h>

#include <ndk/rtlfuncs.h>
#include <reactos/buildno.h>

/* Class includes */
#include "auto_array_ptr.h"
#include "CConfiguration.h"
#include "CFatalException.h"
#include "CInvalidParameterException.h"
#include "CProcess.h"
#include "CSimpleException.h"
#include "CTestInfo.h"
#include "CTest.h"
#include "CTestList.h"
#include "CJournaledTestList.h"
#include "CVirtualTestList.h"
#include "CWebService.h"
#include "CWineTest.h"

/* Useful macros */
#define EXCEPTION(Message)   throw CSimpleException(Message)
#define FATAL(Message)       throw CFatalException(__FILE__, __LINE__, Message)
#define SSEXCEPTION          throw CSimpleException(ss.str().c_str())

/* main.c */
extern CConfiguration Configuration;

/* shutdown.c */
bool ShutdownSystem();

/* tools.c */
wstring AsciiToUnicode(const char* AsciiString);
wstring AsciiToUnicode(const string& AsciiString);
string EscapeString(const char* Input);
string EscapeString(const string& Input);
string GetINIValue(PCWCH AppName, PCWCH KeyName, PCWCH FileName);
bool IsNumber(const char* Input);
void StringOut(const string& String);
string UnicodeToAscii(PCWSTR UnicodeString);
string UnicodeToAscii(const wstring& UnicodeString);


/* Lazy HACK to allow compiling/debugging with MSVC while we lack support
   for linking against "debugsup_ntdll" in MSVC */
#ifdef _MSC_VER
    #define DbgPrint
#endif
