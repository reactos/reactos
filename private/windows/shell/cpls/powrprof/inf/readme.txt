The default processor program builds the Memphis setup
file from an Excel spread sheet, which contains the default power policies data.

The default processor program takes the defaults.csv, comma delimited Excel
source file and creates the Memphis setup file, in INF format.

Run MAKEINF.EXE to build the Memphis setup file.

OEM's can use this tool to make changes to the registry to match the hardware.

 
File list:

README.TXT      This file.
MAKEINF.EXE     Builds POWRCFG.INF.
SOURCES         Sources for MAKEINF.EXE processor.
makefile        Make file for MAKEINF.EXE processor.
MAKEINF.C       Source file for MAKEINF.EXE processor.
PARSE.C         Parsing helper file for MAKEINF.EXE processor.
MAKEINF.RC      Resource file for MAKEINF.EXE processor.
obj             Object directory contains MAKEINF.EXE processor.

POWRCFG.INF     Output. Memphis setup file, in INF format.

