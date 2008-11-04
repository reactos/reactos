<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="usrmgr" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_USRMGR}" installbase="system32" installname="usrmgr.cpl" unicode="yes">
	<importlibrary definition="usrmgr.def" />
	<include base="usrmgr">.</include>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>ntdll</library>
	<library>netapi32</library>
	<file>extra.c</file>
	<file>groupprops.c</file>
	<file>groups.c</file>
	<file>misc.c</file>
	<file>userprops.c</file>
	<file>users.c</file>
	<file>usrmgr.c</file>
	<file>usrmgr.rc</file>
</module>
