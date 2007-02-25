Native Development Kit README
        NDK 1.00
-----------------------------

0. PREABMLE

0.1 COPYRIGHT

The NDK is Copyright © 2005 Alex Ionescu.

0.2 CONTACT INFORMATION

The author, Alex Ionescu, may be reached through the following means:

Email: 	alexi@tinykrnl.org
Mail:	2246, Duvernay. H3J 2Y3. Montreal, QC. CANADA.	
Phone: 	(514)581-7156

1. LICENSE

1.1 OPEN SOURCE USAGE

Open Source Projects may choose to use one of either the two following licenses:

GNU GENERAL PUBLIC LICENSE Version 2, June 1991

		OR

GNU LESSER GENERAL PUBLIC LICENSE Version 2.1, February 1999

The choice is yours to make based on the license which is most compatible with your
software.

You MUST read GPL.TXT or LGPL.TXT after your decision. Violating your chosen license
voids your usage rights of the NDK and will lead to legal action on the part of the
author.

If your Open Source product does not use a license which is compatible with the ones
listed above, please contact the author to reach a mutual agreement to find a better
solution for your product. Alternatively, you may choose to use the Proprietary Usage
license displayed below in section 1.2

If you are unsure of whether or not your product qualifies as an Open Source product,
please contact the Free Software Foundation, or visit their website at www.fsf.org.


1.2 PROPRIETARY USAGE

Because it may be undesirable or impossible to adapt this software to your commercial
and/or proprietary product(s) and/or service(s) using a (L)GPL license, proprietary
products are free to use the following license:

NDK LICENSE Version 1, November 2005

You MUST read NDK.TXT for the full text of this license. Violating your chosen license
voids your usage rights of the NDK, constitutes a copyright violation, and will lead to
legal action on the part of the author.

If you are unsure of have any questions about the NDK License, please contact the
author for further clarification.

2. ORIGINS OF NDK MATERIAL, AND ADDING YOUR OWN

2.1 CONTRIBUTIONS AND SOURCES

The NDK could not exist without the various contributions made by a variety of people
and sources. The following public sources of information were lawfully used:

- GNU NTIFS.H, Revision 43
- W32API, Version 2.5
- Microsoft Windows Driver Kit
- Microsoft Driver Development Kit 2003 SP1
- Microsoft Driver Development Kit 2000
- Microsoft Driver Development Kit NT 4
- Microsoft Driver Development Kit WinME
- Microsoft Installable File Systems Kit 2003 SP1
- Microsoft Windows Debugger (WinDBG) 6.5.0003.7
- Microsoft Public Symbolic Data
- Microsoft Public Windows Binaries (strings)
- OSR Technical Articles
- Undocumented windows 2000 Secrets, a Programmer's Cookbook
- Windows NT/2000 Native API Reference
- Windows NT File System Internals
- Windows Internals I - II
- Windows Internals 4th Edition

If the information contained in these sources was copyrighted, the information was not
copied, but simply used as a basis for developing a compatible and identical definition.
No information protected by a patent or NDA was used. All information was publically
located through the Internet or purchased or licensed for lawful use.

Additionally, the following people contributed to the NDK:

- Art Yerkes
- Eric Kohl
- Filip Navara
- Steven Edwards

2.2 BECOMING A CONTRIBUTOR

To contribute information to the NDK, simply contact the author with your new structure,
definition, enumeration, or prototype. Please make sure that your addition is:

1) Actually correct!
2) Present in Windows NT 5, 5.1, 5.2 and/or 6.0
3) Not already accessible through another public header in the DDK, IFS, WDK and/or PSDK.
4) From a publically verifiable source. The author needs to be able to search for your
   addition in a public information location (book, Internet, etc) and locate this definition.
5) Not Reversed. Reversing a type is STRONGLY discouraged and a reversed type will more then likely
   not be accepted, due to the fact that functionality and naming will be entirely guessed, and things
   like unions are almost impossible to determine. It can also bring up possible legal ramifications
   depending on your location. However, using a tool to dump the strings inside an executable
   for the purpose of locating the actual name or definition of a structure (sometimes possible due
   to ASSERTs or debugging strings) is considered 'fair use' and will be a likely candidate.

If your addition satsfies these points, then please submit it, and also include whether or not
you would like to be credited for it.

3. USAGE

3.1 ORGANIZATION

   * The NDK is organized in a main folder (include/ndk) with arch-specific subfolders (ex: include/ndk/i386). 
   * The NDK is structured by NT Subsystem Component (ex: ex, ps, rtl, etc). 
   * The NDK can either be included on-demand (#include <ndk/xxxxx.h>) or globally (#include <ndk/ntndk.h>).
     The former is recommended to reduce compile time. 
   * The NDK is structured by function and type. Every Subsystem Component has an associated "xxfuncs.h" and
    "xxtypes.h" header, where "xx" is the Subsystem (ex: iofuncs.h, iotypes.h) 
   * The NDK has a special file called "umtypes.h" which exports to User-Mode or Native-Mode Applications the
     basic NT types which are present in ntdef.h. This file cannot be included since it would conflict with
     winnt.h and/or windef.h. Thus, umtypes.h provides the missing types. This file is automatically included
     in a User-Mode NDK project. 
   * The NDK also includes a file called "umfuncs.h" which exports to User-Mode or Native-Mode Applications
     undocumented functions which can only be accessed from ntdll.dll. 
   * The NDK has another special file called "ifssupp.h", which exports to Kernel-Mode drivers a few types which
     are only documented in the IFS kit, and are part of some native definitions. It will be deprecated next year
     with the release of the WDK. 

3.2 USING IN YOUR PROJECT

    *  User Mode Application requiring Native Types: 

       #define WIN32_NO_STATUS   /* Tell Windows headers you'll use ntstatus.s from NDK */
       #include "windows.h"      /* Declare Windows Headers like you normally would */
       #include "ntndk.h"        /* Declare the NDK Headers */

    * Native Mode Application: 

       #include "windows.h"      /* Declare Windows Headers for basic types. NEEDED UNTIL NDK 1.5 */
       #include "ntndk.h"        /* Declare the NDK Headers */

    * Kernel Mode Driver: 

       #include "ntddk.h"       /* Declare DDK Headers like you normally would */
       #include "ntndk.h"       /* Declare the NDK Headers */

    * You may also include only the files you need (example for User-Mode application):

       #define WIN32_NO_STATUS   /* Tell Windows headers you'll use ntstatus.s from NDK */
       #include "windows.h"      /* Declare Windows Headers like you normally would */
       #include "rtlfuncs.h"     /* Declare the Rtl* Functions */

3.3 CAVEATS

    * winternl.h: This header, part of the PSDK, was released by Microsoft as part of one of the governmen
      lawsuits against it, and documents a certain (minimal) part of the Native API and/or types. Unforunately,
      Microsoft decided to hack the Native Types and to define them incorrectly, replacing real members by "reserved"
      ones. As such, you 'cannot include winternl.h in any project that uses the NDK. Note however, that the NDK fully
      replaces it and retains compatibility with any project that used it.
    * Native programs: Native programs must include "windows.h" until the next release of the NDK (1.5). The upcoming
      version will automatically detect the lack of missing types and include them. Note however that you will still
      need to have the PSDK installed.
