Copyright (c) 2002-2005, International Business Machines Corporation and others. All Rights Reserved.
icucal: a sample program which displays the calendar.

This sample demonstrates
         Formatting a calendar
         Outputting text in the default codepage to the console

         
Files:
    cal.c                      Main source file
    uprint.h                   codepage output convenience header
    uprint.h                   codepage output convenience implementation
    cal.sln                    Windows MSVC workspace.  Double-click this to get started.
    cal.vcproj                 Windows MSVC project file

To Build icucal on Windows
    1.  Install and build ICU
    2.  In MSVC, open the workspace file icu\samples\cal\cal.sln
    3.  Choose a Debug or Release build.
    4.  Build.
	
To Run on Windows
    1.  Start a command shell window
    2.  Add ICU's bin directory to the path, e.g.
            set PATH=c:\icu\bin;%PATH%
        (Use the path to where ever ICU is on your system.)
    3.  cd into the cal directory, e.g.
            cd c:\icu\source\samples\cal\debug
    4.  Run it
            cal

To Build on Unixes
    1.  Build ICU.  icucal is built automatically by default unless samples are turned off.
        Specify an ICU install directory when running configure,
        using the --prefix option.  The steps to build ICU will look something
        like this:
           cd <icu directory>/source
           runConfigureICU <platform-name> --prefix <icu install directory> [other options]
           gmake all
           
    2.  Install ICU, 
           gmake install
           
 To Run on Unixes
           cd <icu directory>/source/samples/cal
           
           gmake check
               -or- 

           export LD_LIBRARY_PATH=<icu install directory>/lib:.:$LD_LIBRARY_PATH
           cal
           
           
 Note:  The name of the LD_LIBRARY_PATH variable is different on some systems.
        If in doubt, run the sample using "gmake check", and note the name of
        the variable that is used there.  LD_LIBRARY_PATH is the correct name
        for Linux and Solaris.

