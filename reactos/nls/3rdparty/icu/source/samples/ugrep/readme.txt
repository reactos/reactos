Copyright (c) 2002-2005, International Business Machines Corporation and others. All Rights Reserved.
ugrep: a sample program demonstrating the use of ICU regular expression API.

usage:   ugrep [options] pattern [file ...]

   --help                 Output a brief help message
   -n,  --line-number     Prefix each line of output with the line number within its input file.
   -V,  --version         Output the program version number


The program searches for the specified regular expression in each of the
specified files, and outputs each matching line.

Input files are in the system default (locale dependent) encoding, unless they
begin with a BOM, in which case they are assumed to be in the UTF encoding
specified by the BOM.  Program output is always in the system's default
8 bit code page.

   
Files:
    ./ugrep.c                 source code for the sample
    ./ugrep.sln               Windows MSVC workspace.  Double-click this to get started.
    ./ugrep.vcproj            Windows MSVC project file.
    ./Makefile                Makefile for Unixes.  Needs gmake.
    

To Build ugrep on Windows
    1.  Install and build ICU
    2.  In MSVC, open the workspace file icu\samples\ugrep\ugrep.sln
    3.  Choose a Debug or Release build.
    4.  Build.
	
To Run on Windows
    1.  Start a command shell window
    2.  Add ICU's bin directory to the path, e.g.
            set PATH=c:\icu\bin;%PATH%
        (Use the path to where ever ICU is on your system.)
    3.  cd into the ugrep directory, e.g.
            cd c:\icu\source\samples\ugrep\debug
    4.  Run it
            ugrep ...
     

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
           Put the install directory containing icu-config on the $PATH.  
               This will generally be <icu install directory>/bin
           cd <icu directory>/source/samples/ugrep
           gmake
           
 To Run on Unixes
           cd <icu directory>/source/samples/ugrep
           
           export LD_LIBRARY_PATH=<icu install directory>/lib:.:$LD_LIBRARY_PATH
           ugrep ...
           
           
 Note:  The name of the LD_LIBRARY_PATH variable is different on some systems.
        If in doubt, run the sample using "gmake check", and note the name of
        the variable that is used there.  LD_LIBRARY_PATH is the correct name
        for Linux and Solaris.
