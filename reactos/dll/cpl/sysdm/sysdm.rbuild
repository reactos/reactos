<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="sysdm" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_SYSDM}" installbase="system32" installname="sysdm.cpl" unicode="yes">
	<importlibrary definition="sysdm.def" />
	<include base="sysdm">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>setupapi</library>
	<library>msvcrt</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>ntdll</library>
	<library>msimg32</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<library>ole32</library>
	<library>powrprof</library>
	<file>advanced.c</file>
	<file>environment.c</file>
	<file>general.c</file>
	<file>hardprof.c</file>
	<file>hardware.c</file>
	<file>licence.c</file>
	<file>startrec.c</file>
	<file>sysdm.c</file>
	<file>userprofile.c</file>
	<file>virtmem.c</file>
	<file>sysdm.rc</file>
	<pch>precomp.h</pch>
</module>
