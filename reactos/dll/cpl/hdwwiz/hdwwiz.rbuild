<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="hdwwiz" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_HDWWIZ}" installbase="system32" installname="hdwwiz.cpl" unicode="yes">
	<importlibrary definition="hdwwiz.spec" />
	<include base="hdwwiz">.</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>setupapi</library>
	<library>kernel32</library>
	<file>hdwwiz.c</file>
	<file>hdwwiz.rc</file>
</module>
