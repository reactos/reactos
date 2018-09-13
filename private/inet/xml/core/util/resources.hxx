/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _RESOURCES_HXX
#define _RESOURCES_HXX

typedef unsigned long ResourceID;

//------------------------------------------------------------------
// Encapsulates platform specific resource management.
//------------------------------------------------------------------
class Resources
{
public:
    // Load specified string from string table resource.
    static String* LoadString( ResourceID resId );

    // Format the specified message using the message table in the
    // current application's resources and inserting the variable
    // number of strings in place of the %1, %2, ... %n variables
    // as specified in the message resource.  
    // WARNING: List of args must be null terminated.
    static String* FormatMessageHelper( ResourceID resid, String* first, va_list args);
    static String* FormatMessage( ResourceID resid, String* first, ...);
    
    // Format the given system error into a message.
    static String* FormatSystemMessage( long errorid );

    // Return product version information, eg: "1.0.71.4"
    // False is returned if there is no version info.
    typedef int VerInfo[4];
    static bool GetVersion(String* szFileName, VerInfo& info);

    static void *GetUserResource(const char*  urID, const char*  urType, DWORD *pdwSize = NULL);
};

#endif
