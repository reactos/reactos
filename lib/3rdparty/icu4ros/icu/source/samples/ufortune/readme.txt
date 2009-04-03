Copyright (c) 2002-2005, International Business Machines Corporation and others. All Rights Reserved.
ufortune: a sample program demonstrating the use of ICU resource files by an application.

This sample demonstrates
         Defining resources for use by an application
         Compiling and packaging them into a dll
         Referencing the resource-containing dll from application code
         Loading resource data using ICU's API
         
Files:
    ./ufortune.c                 source code for the sample
    ./ufortune.sln               Windows MSVC workspace.  Double-click this to get started.
    ./ufortune.vcproj            Windows MSVC project file.
    ./Makefile                   Makefile for Unixes.  Needs gmake.
    resources/root.txt           Default resources  (text for messages in English)
    resources/es.txt             Spanish language resources source file..
    resources/res-file-list.txt  List of resource source files to be built
    resources/Makefile           Makefile for compiling resources, for Unixes.
    

To Build ufortune on Windows
    1.  Install and build ICU
    2.  In MSVC, open the workspace file icu\samples\ufortune\ufortune.sln
    3.  Choose a Debug or Release build.
    4.  Build.
	
To Run on Windows
    1.  Start a command shell window
    2.  Add ICU's bin directory to the path, e.g.
            set PATH=c:\icu\bin;%PATH%
        (Use the path to where ever ICU is on your system.)
    3.  cd into the ufortune directory, e.g.
            cd c:\icu\source\samples\ufortune\debug
    4.  Run it
            ufortune
     

To Build on Unixes
    1.  Build ICU.  Specify an ICU install directory when running configure,
        using the --prefix option.  The steps to build ICU will look something
        like this:
           cd <icu directory>/source
           runConfigureICU <platform-name> --prefix <icu install directory> [other options]
           gmake all
           
    2.  Install ICU, 
           gmake install
           
    3.  Build the sample
           cd <icu directory>/source/samples/ufortune
           export ICU_PREFIX= <icu install directory>
           gmake
           
 To Run on Unixes
           cd <icu directory>/source/samples/ufortune
           
           gmake check
              or
           export LD_LIBRARY_PATH=<icu install directory>/lib:.:$LD_LIBRARY_PATH
           ufortune
           
           
 Note:  The name of the LD_LIBRARY_PATH variable is different on some systems.
        If in doubt, run the sample using "gmake check", and note the name of
        the variable that is used there.  LD_LIBRARY_PATH is the correct name
        for Linux and Solaris.
