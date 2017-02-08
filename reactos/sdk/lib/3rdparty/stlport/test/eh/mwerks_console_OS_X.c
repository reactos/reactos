/* Metrowerks Standard Library
 * Copyright © 1995-2002 Metrowerks Corporation.  All rights reserved.
 *
 * $Date$
 * $Revision$
 */

#include <ansi_parms.h>
#include <size_t.h>
#include <console.h>
#include <unistd.h>

#if __MACH__
  short InstallConsole(short fd)
  {
  #pragma unused (fd)

    return 0;
  }
#else
  #include <Carbon.h>

  typedef int (*ReadPtr)(int, void *, __std(size_t));
  typedef int (*WritePtr)(int, const void *, __std(size_t));

  static struct
  {
    Boolean isLoaded;
    CFBundleRef theBundle;
    ReadPtr theRead;
    WritePtr theWrite;
  } __msl_os_x;

  static OSErr __msl_CreateFrameworkBundleFromName(CFStringRef theFrameworkName,
    CFBundleRef *theBundle)
  {
    OSErr theErr;
    FSRef theRef;
    CFURLRef theFrameworkURL;
    CFURLRef theBundleURL;

    /* Find the folder containing all the frameworks */
    theErr = FSFindFolder(kOnAppropriateDisk, kFrameworksFolderType, false, &theRef);

    if (theErr == noErr)
    {
      /* Turn the framework folder FSRef into a CFURL */
      theFrameworkURL = CFURLCreateFromFSRef(kCFAllocatorSystemDefault, &theRef);

      if (theFrameworkURL != NULL)
      {
        /* Create a CFURL pointing to the desired framework */
        theBundleURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorSystemDefault,
          theFrameworkURL, theFrameworkName, false);

        CFRelease(theFrameworkURL);

        if (theBundleURL != NULL)
        {
          /* Turn the CFURL into a bundle reference */
          *theBundle = CFBundleCreate(kCFAllocatorSystemDefault, theBundleURL);

          CFRelease(theBundleURL);
        }
      }
    }

    return theErr;
  }

  short InstallConsole(short fd)
  {
  #pragma unused (fd)
    OSErr theErr;
    short theResult;

    theResult = -1;

    /* Start with no bundle */
    __msl_os_x.isLoaded = false;
    __msl_os_x.theBundle = NULL;
    __msl_os_x.theRead = NULL;
    __msl_os_x.theWrite = NULL;

    /* Create a bundle reference based on its name */
    theErr = __msl_CreateFrameworkBundleFromName(CFSTR("System.framework"),
      &__msl_os_x.theBundle);

    if ((theErr == noErr) && (__msl_os_x.theBundle != NULL))
    {
      theResult = 0;

      __msl_os_x.isLoaded = CFBundleLoadExecutable(__msl_os_x.theBundle);

      if (__msl_os_x.isLoaded)
      {
        /* Lookup the functions in the bundle by name */
        __msl_os_x.theRead = (ReadPtr)
          CFBundleGetFunctionPointerForName(__msl_os_x.theBundle, CFSTR("read"));
        __msl_os_x.theWrite = (WritePtr)
          CFBundleGetFunctionPointerForName(__msl_os_x.theBundle, CFSTR("write"));
      }
    }

    return theResult;
  }
#endif

void RemoveConsole(void)
{
#if !__MACH__
  if (__msl_os_x.theBundle != NULL)
  {
    if (__msl_os_x.isLoaded)
    {
      __msl_os_x.theRead = NULL;
      __msl_os_x.theWrite = NULL;

      CFBundleUnloadExecutable(__msl_os_x.theBundle);
      __msl_os_x.isLoaded = false;
    }

    CFRelease(__msl_os_x.theBundle);
    __msl_os_x.theBundle = NULL;
  }
#endif
}

long WriteCharsToConsole(char *buffer, long n)
{
#if __MACH__
  return write(1, buffer, n);
#else
  /* Call the function if it was found */
  if (__msl_os_x.theWrite == NULL)
    return -1;
  else
    return __msl_os_x.theWrite(1, buffer, n);
#endif
}

#if __MACH__
long WriteCharsToErrorConsole(char *buffer, long n)
{
  return write(2, buffer, n);
}
#endif

long ReadCharsFromConsole(char *buffer, long n)
{
#if __MACH__
  return read(0, buffer, n);
#else
  /* Call the function if it was found */
  if (__msl_os_x.theRead == NULL)
    return -1;
  else
    return __msl_os_x.theRead(0, buffer, n);
#endif
}

/* JWW - This code should never be reached, but it's needed for link purposes */
char *__ttyname(long fildes)
{
#pragma unused (fildes)
  /* all streams have the same name */
  static char *__devicename = "Terminal";

  if (fildes >= 0 && fildes <= 2)
    return (__devicename);

  return (0L);
}

int kbhit(void)
{
  return 0;
}

int getch(void)
{
  return 0;
}

void clrscr()
{
  return;
}

/* Change record:
 * JWW 010919 Created Mach-O console stubs file
 * JWW 020418 Use __std() for all size_t, and #include <size_t.h> to get proper C++ definitions
 */
