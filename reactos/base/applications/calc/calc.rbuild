<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="calc" type="win32gui" installbase="system32" installname="calc.exe">
	<include base="calc">.</include>
	<define name="_WIN32_IE">0x0501</define>
	<define name="WINVER">0x0501</define>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<file>dialog.c</file>
	<file>stats.c</file>
	<file>winecalc.c</file>
	<file>rsrc.rc</file>
</module>
