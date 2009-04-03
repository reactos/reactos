Copyright (c) 2003-2005, International Business Machines Corporation and others. All Rights Reserved.
uciter8: Lenient reading of 8-bit Unicode with a UCharIterator

This sample demonstrates reading
8-bit Unicode text leniently, accepting a mix of UTF-8 and CESU-8
and also accepting single surrogates.
UTF-8-style macros are defined as well as a UCharIterator.
The macros are incomplete (do not assemble code points from pairs of surrogates)
but sufficient for the iterator.

If you wish to use the lenient-UTF/CESU-8 UCharIterator in a context outside of
this sample, then copy the uit_len8.c file,
as well as either the uit_len8.h header or just the prototype that it contains.

*** Warning: ***
This UCharIterator reads an arbitrary mix of UTF-8 and CESU-8 text.
It does not conform to any one Unicode charset specification,
and its use may lead to security risks.


Files:
    uciter8.c        Main source file in C
    uit_len8.c       Lenient-UTF/CESU-8 UCharIterator implementation
    uit_len8.h       Header file with the prototoype for the lenient-UTF/CESU-8 UCharIterator
    uciter8.sln      Windows MSVC workspace.  Double-click this to get started.
    uciter8.vcproj   Windows MSVC project file

To Build uciter8 on Windows
    1.  Install and build ICU
    2.  In MSVC, open the workspace file icu\samples\uciter8\uciter8.sln
    3.  Choose a Debug or Release build.
    4.  Build.

To Run on Windows
    1.  Start a command shell window
    2.  Add ICU's bin directory to the path, e.g.
            set PATH=c:\icu\bin;%PATH%
        (Use the path to where ever ICU is on your system.)
    3.  cd into the uciter8 directory, e.g.
            cd c:\icu\source\samples\uciter8\debug
    4.  Run it
            uciter8

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
           cd <icu directory>/source/samples/uciter8
           gmake ICU_PREFIX=<icu install directory)

To Run on Unixes
           cd <icu directory>/source/samples/uciter8
           
           gmake ICU_PREFIX=<icu install directory>  check
               -or- 

           export LD_LIBRARY_PATH=<icu install directory>/lib:.:$LD_LIBRARY_PATH
           uciter8


 Note:  The name of the LD_LIBRARY_PATH variable is different on some systems.
        If in doubt, run the sample using "gmake check", and note the name of
        the variable that is used there.  LD_LIBRARY_PATH is the correct name
        for Linux and Solaris.
