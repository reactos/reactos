/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "resources.hxx"

#include <stdio.h>

extern HINSTANCE g_hInstance;

//--------------------------------------------------------------------
String*
Resources::LoadString( ResourceID resId )
{
    int size = 4096;
    char* buffer = new char[size];

    int rc = ::LoadStringA( g_hInstance, resId, buffer, size);
    long error = 0;
    String * result = null;
    if (rc > 0)
    {
        buffer[rc] = '\0';
        result =  String::newString(buffer);
    } 
    delete [] buffer;
    if (rc == 0)
    {
        Exception::throwE(E_FAIL); // String::newString(_T("StringTable resource not found")));
    }
    return result;
}

//--------------------------------------------------------------------
String*
Resources::FormatMessageHelper( ResourceID resid, String* first, va_list args)
{
    // Count the number of arguments and build a TCHAR* va_list.
    va_list marker;
    int count = 0;
    int l = 0;

    char ** list = null;
    char * buffer = null;
	Exception * ex = null;
    String* result = null;

    marker = args;

    String * s = first;

    while (s != null)
    {
        Assert(String::_getClass()->isInstance(s));        
        l += s->length();
        count++;
        s = va_arg(marker, String *);
    }

    TRY
    {
        if (count > 0)
        {
            list = new char * [count];
            marker = args;

            s = first;
            count = 0;
            while (s != null)
            {
                AsciiText ascii(s);
                list[count] = new char [ascii._uLen + 1];
                memcpy(list[count], (char *)ascii, ascii._uLen + 1);
                count++;
                s = va_arg(marker, String *);
            }
        }

        // Now call system FormatMessage function to do the work.
        DWORD size = 4096 + l;

        buffer = new char[size];

        // STRINGTABLE
        DWORD rc = ::FormatMessageA( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY , 
            g_hInstance, resid, 
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            buffer, size, (va_list*)list);

        if (rc > 0)
        {
            buffer[rc] = '\0';
            result = String::newString(buffer);
        }
        else
        {
            delete[] buffer;
            buffer = null; // in case there is no last error to throw.
            Exception::throwLastError();
        }
    }
    CATCH
    {
		ex = GETEXCEPTION();
    }
    ENDTRY

    delete[] buffer;

    if (list != null)
    {
        for (int i = 0; i < count; i++)
        {
            delete[] list[i];
        }
        delete[] list;
    }

    if (ex)
        Exception::throwAgain();

    return result;
}
 
//------------------------------------------------------------------------------------------
String* 
Resources::FormatSystemMessage( long errorid )
{
    // Now call system FormatMessage function to do the work.
    DWORD size = 4096;
    char* buffer = new char[size];

    DWORD rc = ::FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM, 
        NULL, errorid, 
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        buffer, size, NULL);

    if (rc <= 0)
    {
        // maybe it's URLMON...in which case URLMON.DLL should be loaded already.
        HINSTANCE h = ::GetModuleHandleA("URLMON.DLL");
        rc = ::FormatMessageA( FORMAT_MESSAGE_FROM_HMODULE, 
            h, errorid, 
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            buffer, size, NULL);   
    }
    if (rc <= 0)
    {
        // maybe it's WININET...in which case WININET.DLL should be loaded already.
        HINSTANCE h = ::GetModuleHandleA("WININET.DLL");
        rc = ::FormatMessageA( FORMAT_MESSAGE_FROM_HMODULE, 
            h, errorid, 
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            buffer, size, NULL);   
    }

    String * result;
    TRY
    {
        long error = 0;
        if (rc > 0)
        {
            buffer[rc] = '\0';
            result = String::newString(buffer);
        }
        else
        {
#if defined(UNIX)
            String * num = String::newString((int)errorid);
#else
            String * num = String::newString(errorid);
#endif
            result = FormatMessage(MSG_E_SYSTEM_ERROR, num, null);
        }
    }
    CATCH
    {
        result = String::emptyString();
    }
    ENDTRY
    delete[] buffer;
    return result;
}

//------------------------------------------------------------------------------------------
bool
Resources::GetVersion(String* fileName, VerInfo& auiVersion)
{
    DWORD dwInfo;
    DWORD dwJunk;
    VS_FIXEDFILEINFO *info;
    char* szBuffer = new char[MAX_PATH];
    char* szFileBuffer = new char[MAX_PATH];
    unsigned int goo;
    bool result = false;
    if (0<(GetFullPathNameA((char *)AsciiText(fileName), MAX_PATH, szFileBuffer, NULL))) {
        dwInfo = GetFileVersionInfoSizeA(szFileBuffer,&dwJunk);

        if (dwInfo>0) {
            if (TRUE==GetFileVersionInfoA(szFileBuffer,0,MAX_PATH,szBuffer)) {
                goo = dwInfo;
                if (TRUE == VerQueryValueA(szBuffer, "\\", (void **)&info,&goo)) {
                    auiVersion[0] = HIWORD(info->dwFileVersionMS);
                    auiVersion[1]  = LOWORD(info->dwFileVersionMS);
                    auiVersion[2] = HIWORD(info->dwFileVersionLS);
                    auiVersion[3]  = LOWORD(info->dwFileVersionLS);
                    result = true;
                }
            }
        }
    }
    delete [] szBuffer;
    delete[] szFileBuffer;
    return result;
}

//------------------------------------------------------------------------------------------
void *
Resources::GetUserResource(const char* urID, const char* urType, DWORD *pdwSize)
{
    void *pRet = NULL;
    HRSRC hrsrc;
    HGLOBAL hgbl;

    if (pdwSize)
        *pdwSize = 0;

    if ((hrsrc = ::FindResourceA(g_hInstance, urID, urType)) != NULL)
    {
        if ((hgbl = ::LoadResource(g_hInstance, hrsrc)) != NULL)
        {
            pRet = ::LockResource(hgbl);
            if (NULL != pdwSize)
            {
                *pdwSize = ::SizeofResource(g_hInstance, hrsrc);
            }
        }
    }
    return pRet;
}


//------------------------------------------------------------------------------------------
String* 
Resources::FormatMessage( ResourceID resid, String* first, ...)
{
    va_list va;
    va_start(va, first);

    String * s = FormatMessageHelper( resid, first, va);
    
    va_end(va);
    return s;
}
