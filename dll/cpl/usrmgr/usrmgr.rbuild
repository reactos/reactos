<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="usrmgr" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_USRMGR}" installbase="system32" installname="usrmgr.cpl" unicode="yes">
	<importlibrary definition="usrmgr.def" />
	<include base="usrmgr">.</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x609</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>ntdll</library>
	<library>netapi32</library>
	<library>msvcrt</library>
	<file>extra.c</file>
	<file>groups.c</file>
	<file>users.c</file>
	<file>usrmgr.c</file>
	<file>usrmgr.rc</file>
</module>
