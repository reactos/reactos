Copyright (c) 2001-2005 International Business Machines
Corporation and others. All Rights Reserved.
uresb: Resource Bundle

This sample demonstrates
         Building a resource bundle
         Using ICU to print data from a resource bundle

         
Files:
    uresb.c        Main source file in C
    uresb.sln      Windows MSVC workspace.  Double-click this to get started.
    uresb.vcproj   Windows MSVC project file
    resources.dsp  Windows project file for resources
    resources.mak  Windows makefile for resources

    root.txt       Root resource bundle
    en.txt         English translation
    sr.txt         Serbian translation (cp1251)

To Build uresb on Windows
    1.  Install and build ICU
    2.  In MSVC, open the workspace file icu\samples\uresb\uresb.sln
    3.  Choose a Debug or Release build.
    4.  Build.
	
To Run on Windows
    1.  Start a command shell window
    2.  Add ICU's bin directory to the path, e.g.
            set PATH=c:\icu\bin;%PATH%
        (Use the path to where ever ICU is on your system.)
    3.  cd into the uresb directory, e.g.
            cd c:\icu\source\samples\uresb\debug
    4.  Run it  (with a locale name, ex. english)
            uresb  en

To Build on Unixes
    1.  Build ICU.  
        Specify an ICU install directory when running configure,
        using the --prefix option.  The steps to build ICU will look something
        like this:
           cd <icu directory>/source
           runConfigureICU <platform-name> --prefix <icu install directory> [other options]
           gmake all
           
    2.  Install ICU, 
           gmake install
 
    3.  Compile
           cd <icu directory>/source/samples/uresb
           gmake ICU_PREFIX=<icu install directory) ICU_PATH=<icu source directory>
           
 To Run on Unixes
           cd <icu directory>/source/samples/uresb
           
           gmake ICU_PREFIX=<icu install directory>  check
               -or- 

           export LD_LIBRARY_PATH=<icu install directory>/lib:.:$LD_LIBRARY_PATH
           uresb
           
           
 Note:  The name of the LD_LIBRARY_PATH variable is different on some systems.
        If in doubt, run the sample using "gmake check", and note the name of
        the variable that is used there.  LD_LIBRARY_PATH is the correct name
        for Linux and Solaris.

