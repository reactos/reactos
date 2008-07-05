<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="write" type="win32gui" installbase="system32" installname="write.exe" unicode="yes">
	<include base="write">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<file>write.c</file>
	<file>rsrc.rc</file>
</module>
