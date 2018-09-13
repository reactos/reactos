The default processor program builds the NT registry specification
file from an Excel spread sheet, which contains the default power policies data.

The default processor program takes the defaults.csv, comma delimited Excel
source file and creates the registry specification file, in regini format.

Run MAKEINI.EXE to build the registry specification file.

Changes to POWRCFG.INI must be checked into the following NT registry source
files seperately:

\nt\private\windows\setup\inf\win4\inf\hivesft.inx - HKEY_LOCAL_MACHINE entries.
\nt\private\windows\setup\inf\win4\inf\hivedef.inx - New user HKEY_CURRENT_USER entries.
\nt\private\windows\setup\inf\win4\inf\hiveusd.inx - Existing user HKEY_CURRENT_USER entries.

OEM's can use this tool to make changes to the registry to match their hardware.

To write the win95 registry:

regini -w c:\memphis c:\memphis POWERCFG.INI

 
File list:

README.TXT      This file.
MAKEINI.EXE     Builds POWERCFG.INI, registry specification file.
SOURCES         Sources for MAKEINI.EXE processor.
makefile        Make file for MAKEINI.EXE processor.
MAKEINI.C       Source file for MAKEINI.EXE processor.
MAKEINI.RC      Resource file for MAKEINI.EXE processor.
obj             Object directory contains MAKEINI.EXE processor.

defaults.xls    Reference. Original Excel worksheet for defaults.csv.
defaults.csv    Input.  Comma delimited Excel source file.
POWRCFG.INI     Output. Registry specification file, in regini format.

