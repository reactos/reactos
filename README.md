# ReactOS Project

Current version: __0.4.6__
![ReactOS Logo](https://reactos.org/wiki/images/0/02/ReactOS_logo.png)

## Quick Links
- [Website](https://reactos.org)
- [Wiki](https://reactos.org/wiki)
- [JIRA Bug tracker](https://jira.reactos.org/issues)
- [ReactOS Git mirror](https://git.reactos.org)

## What is ReactOS?

ReactOS™ is an Open Source effort to develop a quality operating system that is
compatible with applications and drivers written for the Microsoft® Windows™ NT
family of operating systems (NT4, 2000, XP, 2003, Vista, Seven).

The ReactOS project, although currently focused on Windows Server 2003
compatibility, is always keeping an eye toward compatibility with
Windows Vista and future Windows NT releases.

The code of ReactOS is licensed under [GNU GPL 2.0+](https://spdx.org/licenses/GPL-2.0+.html).

## Building ReactOS

### Prerequisites

To build the system it is strongly advised to use the ReactOS Build Environment
(RosBE). Up-to-date versions for Windows and for Unix/GNU-Linux are available
from our download page at: http://www.reactos.org/wiki/Build_Environment/

Alternatively one can use Microsoft Visual C++ (MSVC) version 2010+, together
with separate installations of CMake and the Ninja build utility.

### Building binaries

To build ReactOS run 'ninja' (without the quotes), or alternatively run
'make' if you are using the Make utility, from the top directory.
NOTE: In the other examples listed in the following, similar modification
holds if you are using the Make utility instead of Ninja.
If you are using RosBE, follow on-screen instructions.

### Building bootable images

To build a bootable CD image run 'ninja bootcd' (without the quotes) from the
top directory. This will create a CD image with a filename, ReactOS.iso, in
the top directory.

See more at http://www.reactos.org/wiki/Building_ReactOS

## More information

ReactOS is a Free and Open Source operating system based on the Windows architecture, 
providing support for existing applications and drivers, and an alternative to the current dominant consumer operating system.

It is not another wrapper built on Linux, like WINE. It does not attempt or plan to compete with WINE; in fact, the user-mode part of ReactOS is almost entirely WINE-based and our two teams have cooperated closely in the past. ReactOS is also not "yet another OS". It does not attempt to be a third player, like SkyOS or any other alternative OS out there. People are not meant to uninstall Linux and use ReactOS instead; ReactOS is a replacement for Windows users who want a Windows replacement that behaves just like Windows.

More information is available at: https://www.reactos.org.

Also see the [media\doc](/media/doc/) subdirectory for some sparse notes.

## Who is responsible

See the [CREDITS] file.

## ReactOS SVN and GIT mirrors

The main development is done here on Github. However, read-only mirrors exist.

* SVN mirror: https://svn.reactos.org/svn/reactos?view=revision
* Git mirror: https://git.reactos.org/?p=reactos.git;a=summary
 
## Disclaimer

_ReactOS® is a registered trademark of the ReactOS Foundation._

_Windows® NT™ is a registered trademark of Microsoft Corporation._
