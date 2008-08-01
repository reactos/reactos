Copyright (c) 2002-2005, International Business Machines Corporation and others. All Rights Reserved.
udata: Low level ICU data

This sample demonstrates
    Using the low level ICU data handling interfaces (udata) to create 
 and later access user data.
         
Files:
    writer.c       C source for Writer application, will generate data file to be read by Reader.
    reader.c       C source for Reader application, will read file created by Writer
    udata.sln      Windows MSVC workspace.  Double-click this to get started.
    udata.vcproj   Windows MSVC project file

To Build udata on Windows
    1.  Install and build ICU
    2.  In MSVC, open the workspace file icu\samples\udata\udata.sln
    3.  Choose a Debug or Release build.
    4.  Build.

To Run on Windows
    1.  Start a command shell window
    2.  Add ICU's bin directory to the path, e.g.
            set PATH=c:\icu\bin;%PATH%
        (Use the path to where ever ICU is on your system.)
    3.  cd into the udata directory, e.g.
            cd c:\icu\source\samples\udata\debug
    4.  Run it
            writer
            reader

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
           You will need to set ICU_PATH to the location of your ICU source tree, for example ICU_PATH=/home/srl/icu   (containing source, etc.)
           cd <icu directory>/source/samples/udata
           gmake ICU_PATH=<icu source directory> ICU_PREFIX=<icu install directory)
           
 To Run on Unixes
           cd <icu directory>/source/samples/udata
           gmake ICU_PREFIX=<icu install directory>  check
               -or- 

           export LD_LIBRARY_PATH=<icu install directory>/lib:.:$LD_LIBRARY_PATH
           writer
           reader
           
 Note:  The name of the LD_LIBRARY_PATH variable is different on some systems.
        If in doubt, run the sample using "gmake check", and note the name of
        the variable that is used there.  LD_LIBRARY_PATH is the correct name
        for Linux and Solaris.

