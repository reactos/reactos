<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="winemine" type="win32gui" installbase="system32" installname="winemine.exe">
	<include base="winemine">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>advapi32</library>
	<file>main.c</file>
	<file>dialog.c</file>
	<file>rsrc.rc</file>
</module>
