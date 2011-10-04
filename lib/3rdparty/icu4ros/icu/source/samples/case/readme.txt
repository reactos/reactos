Copyright (c) 2003-2005, International Business Machines Corporation and others. All Rights Reserved.
case: case mapping

This sample demonstrates
         Using ICU to convert between different cases

         
Files:
    case.cpp      Main source file in C++
    ucase.c       Main source file in C
    case.sln      Windows MSVC workspace.  Double-click this to get started.
    case.vcproj   Windows MSVC project file

To Build case on Windows
    1.  Install and build ICU
    2.  In MSVC, open the solution file icu\samples\case\case.sln
        (or, use the workspace All, in icu\samples\all\all.sln )
    3.  Choose a Debug or Release build.
    4.  Build.
	
To Run on Windows
    1.  Start a command shell window
    2.  Add ICU's bin directory to the path, e.g.
            set PATH=c:\icu\bin;%PATH%
        (Use the path to where ever ICU is on your system.)
    3.  cd into the case directory, e.g.
            cd c:\icu\source\samples\case\debug
    4.  Run it
            case

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
           cd <icu directory>/source/samples/case
           gmake ICU_PREFIX=<icu install directory)
           
 To Run on Unixes
           cd <icu directory>/source/samples/case
           
           gmake ICU_PREFIX=<icu install directory>  check
               -or- 

           export LD_LIBRARY_PATH=<icu install directory>/lib:.:$LD_LIBRARY_PATH
           case
           
           
 Note:  The name of the LD_LIBRARY_PATH variable is different on some systems.
        If in doubt, run the sample using "gmake check", and note the name of
        the variable that is used there.  LD_LIBRARY_PATH is the correct name
        for Linux and Solaris.

