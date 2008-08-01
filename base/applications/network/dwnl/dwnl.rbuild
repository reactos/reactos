<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="dwnl" type="win32cui" installbase="system32" installname="dwnl.exe" unicode="yes">
	<include base="dwnl">.</include>
	<library>kernel32</library>
	<library>urlmon</library>
	<library>wininet</library>
	<library>uuid</library>
	<define name="WINVER">0x0501</define>
	<define name="_WIN32_IE">0x0600</define>
	<file>dwnl.c</file>
</module>
