<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="calc" type="win32gui" installbase="system32" installname="calc.exe" allowwarnings="true" unicode="yes">
	<include base="calc">.</include>
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<file>about.c</file>
	<file>function.c</file>
	<file>rpn.c</file>
	<file>utl.c</file>
	<file>winmain.c</file>
	<file>resource.rc</file>
</module>
