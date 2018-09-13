/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef unix
#define main prog_main
 
int main(int argc, char **argv);
 
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCmdLine, int
nCmdShow)  {
    extern int __argc;
    extern char **__argv;
    return main(__argc, __argv);
}
#endif


void BuildCharTable()
{
    const short TABLE_SIZE = 128;
    int charType[TABLE_SIZE];
    enum CharType
    {
        FWHITESPACE    = 1,
        FDIGIT         = 2,
        FLETTER        = 4,
        FMISCNAME      = 8,
        FSTARTNAME     = 16,
        FCHARDATA      = 32
    };

    //-------------------------------------------------------------------------
    // The rules for this are in the XML Language Spec.
    int i;
    for ( i = 0; i < TABLE_SIZE; i++) {
        WCHAR c = (WCHAR)i;
        charType[i] = 0;            
        if (c >= 0x20)
        {
            // section 2.2
            charType[i] |= FCHARDATA;
            if (IsCharAlpha(c)) 
                charType[i] |= FLETTER;
            else if (IsCharAlphaNumeric(c))
                charType[i] |= FDIGIT;
        }
//        g_chCharUpper[i] = (TCHAR)::CharUpper((TCHAR *)i);
    }

    charType[0x9] |= FWHITESPACE;   // section 2.3
    charType[0xa] |= FWHITESPACE;
    charType[0xd] |= FWHITESPACE;
    charType[0x20] |= FWHITESPACE;

    charType['.'] |= FMISCNAME;
    charType['-'] |= FMISCNAME;
    charType['_'] |= FMISCNAME | FSTARTNAME;
    charType[':'] |= FSTARTNAME;
    charType[0x9] |= FCHARDATA;
    charType[0xa] |= FCHARDATA;
    charType[0xd] |= FCHARDATA;
    //-------------------------------------------------------------------------

    printf("CharType g_anCharType[TABLE_SIZE] = { \n");
    for ( i = 0; i < TABLE_SIZE; i++) 
    {
        printf("\t0");
        if (charType[i] & FWHITESPACE) printf(" | FWHITESPACE");
        if (charType[i] & FDIGIT) printf(" | FDIGIT");
        if (charType[i] & FLETTER) printf(" | FLETTER");
        if (charType[i] & FMISCNAME) printf(" | FMISCNAME");
        if (charType[i] & FSTARTNAME) printf(" | FSTARTNAME");
        if (charType[i] & FCHARDATA) printf(" | FCHARDATA");
        printf(",\n");
    }
    printf("};\n");
}

//====================================================================================
int __cdecl main(int argc, char* argv[])
{
    BuildCharTable();
	return 0;	
}


#ifdef UNIX
#else
void *
MemAllocNe(size_t cb)
{
    void* pv = LocalAlloc(LMEM_FIXED, cb);
    return pv;
}

void *
MemAlloc(size_t cb)
{
    void* pv = LocalAlloc(LMEM_FIXED, cb);
    return pv;
}

void
MemFree(void *pv)
{

    pv = ::LocalFree(pv);
}
#endif
