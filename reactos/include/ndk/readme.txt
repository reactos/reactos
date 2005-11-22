Native Development Kit README
        NDK 1.00
-----------------------------

0. PREABMLE

0.1 COPYRIGHT

The NDK is Copyright © 2005 Alex Ionescu.

0.2 CONTACT INFORMATION

The author, Alex Ionescu, may be reached through the following means:

Email: 	alex.ionescu@reactos.com
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

3.1 TODO (COPY FROM WIKI)

... TODO ... (COPY FROM WIKI)
