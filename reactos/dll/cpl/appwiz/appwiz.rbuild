<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="appwiz" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_APPWIZ}"  installbase="system32" installname="appwiz.cpl" unicode="yes">
	<importlibrary definition="appwiz.def" />
	<include base="appwiz">.</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>msvcrt</library>
	<library>ole32</library>
	<library>uuid</library>
	<library>shell32</library>
	<file>appwiz.c</file>
	<file>remove.c</file>
	<file>add.c</file>
	<file>rossetup.c</file>
	<file>createlink.c</file>
	<file>appwiz.rc</file>
</module>
