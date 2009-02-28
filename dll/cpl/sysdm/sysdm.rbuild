<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="sysdm" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_SYSDM}" installbase="system32" installname="sysdm.cpl" unicode="yes">
	<importlibrary definition="sysdm.spec" />
	<include base="sysdm">.</include>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>setupapi</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
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
