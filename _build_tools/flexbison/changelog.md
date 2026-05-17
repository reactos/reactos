## Changelog

**NOTE**:
  * 2.4.x versions include bison version 2.7
  * 2.5.x versions include bison version 3.x
  
### version 2.5.25
  * upgrade win_bison to version 3.8.2
  * upgrade m4 to version 1.4.19

### version 2.5.24
  * upgrade win_bison to version 3.7.4
  * upgrade m4 to version 1.4.18
  * upgrade gnulib
  * removed VS2015 support
  * fixed win_bison --update option (renaming opened file)

### version 2.5.23
  * upgrade win_bison to version 3.7.1

### version 2.5.22
  * upgrade win_bison to version 3.5.0

### version 2.5.21
  * avoid _m4eof lines in generated bison code while printing warnings

### version 2.5.20
  * recovered invoking win_bison from different folders

### version 2.5.19
  * upgrade win_bison to version 3.4.1
  
### version 2.5.18
  * upgrade win_bison to version 3.3.2

### version 2.5.17
  * upgrade win_bison to version 3.3.1

### version 2.5.16
  * upgrade win_bison to version 3.1
  * write output flex/bison files in binary mode "wb" that means use '\n' EOL not '\r\n'
  * documentation about how to use the custom build-rules is now included

### versions 2.4.12/2.5.15
  * upgrade win_bison to version 3.0.5

### versions 2.4.12/2.5.14
  * revert to Visual Studio 2015 due to false positive virus alarms for win_flex.exe

### versions 2.4.11/2.5.13
  * fixed VS 2017 compilation errors in location.cc

### versions 2.4.11/2.5.12
  * migrate to Visual Studio 2017

### versions 2.4.10/2.5.11
  * upgrade win_flex to version 2.6.4
  * fixed compilation warnings

### versions 2.4.9/2.5.10
  * data folder was up to dated for bison 3.0.4

### versions 2.4.9/2.5.9
  * recovered --header-file win_flex option

### versions 2.4.8/2.5.8
  * fixed outdated FlexLexer.h file

### versions 2.4.7/2.5.7
  * upgrade win_flex to version 2.6.3
  * fixed compilation warnings

### versions 2.4.6/2.5.6
  * upgrade win_bison to version 3.0.4
  * win_bison v2.7 is unchanged
  * add separate custom build rules
    * for win_bison `custom_build_rules\win_bison_only`
    * and win_flex `custom_build_rules\win_flex_only`

### versions 2.4.5/2.5.5
  * fix missing Additional Options in custom build rules
  * fix incorrect "----header-file" option in flex custom build rules
  * add some extra flex options to Visual Studio property pages:
     1. Prefix (--prefix="...")
     2. C++ Class Name (--yyclass="...")

###versions 2.4.4/2.5.4
  * fix silent errors in custom build rules
  * add some flex/bison options to Visual Studio property pages:
  * Bison:
     1. Output File Name (--output="...")
     2. Defines File Name (--defines="...")
     3. Debug (--debug)
     4. Verbose (--verbose)
     5. No lines (--no-lines)
     6. File Prefix (--file-prefix="...")
     7. Graph File (--graph="...")
     8. Warnings (--warnings="...")
     9. Report (--report="...")
     10. Report File Name (--report-file="...")

  * Flex:
     1. Output File Name (--outfile="...")
     2. Header File Name (--header-file="...")
     3. Windows compatibility mode (--wincompat)
     4. Case-insensitive mode (--case-insensitive)
     5. Lex-compatibility mode (--lex-compat)
     6. Start Condition Stacks (--stack)
     7. Bison Bridge Mode (--bison-bridge)
     8. No #line Directives (--noline)
     9. Generate Reentrant Scanner (--reentrant)
     10. Generate C++ Scanner (--c++)
     11. Debug Mode (--debug)

### versions 2.4.3/2.5.3
  * fix incorrect #line directives in win_flex.exe
see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=542482

### versions 2.4.2/2.5.2
  * backport parallel invocations of win_bison version 2.7
  * win_bison of version 3.0 is unchanged

### versions 2.4.1/2.5.1
  * remove XSI extention syntax for fprintf function (not implemented in windows)
  * this fixes Graphviz files generation for bison

**NOTE**:
  * 2.4.x versions will include bison version 2.7
  * 2.5.x versions will include bison version 3.0

### version 2.5
  * upgrade win_bison to version 3.0 and make temporary win_bison's files process unique (so parallel invocations of win_bison are possible)

**NOTE**: Several deprecated features were removed in bison 3.0 so this version can break your projects.
Please see http://savannah.gnu.org/forum/forum.php?forum_id=7663
For the reason of compatibility I don't change win_flex_bison-latest.zip to refer to win_flex_bison-2.5.zip file.
It still refer to win_flex_bison-2.4.zip

### version 2.4
  * fix problem with "m4_syscmd is not implemented" message.
  * Now win_bison should output correct diagnostic and error messages.

### version 2.3
  * hide __attribute__ construction for non GCC compilers

### version 2.2
  * added --wincompat option to win_flex (this option changes `<unistd.h>` unix include with `<io.h>` windows analog
  also `isatty/fileno` functions changed to `_isatty/_fileno`)
fixed two "'<' : signed/unsigned mismatch" warnings in win_flex generated file

### version 2.1
  * fixed crash when execute win_bison.exe under WindowsXP (argv[0] don't have full application path)
  * added win_flex_bison-latest.zip package to freeze download link

### version 2.0
  * upgrade win_bison to version 2.7 and win_flex to version 2.5.37

### version 1.2
  * fixed win_flex.exe #line directives (some #line directives in output file were with unescaped backslashes)

### version 1.1
  * fixed win_flex.exe parallel invocations (now all temporary files are process specific)
  * added FLEX_TMP_DIR environment variable support to redirect temporary files folder
  * added '.exe' to program name in win_flex.exe --version output (CMake support)
  * fixed win_bison.exe to use "/data" subfolder related to executable path rather than current working directory
  * added BISON_PKGDATADIR environment variable to redirect "/data" subfolder to a different place
