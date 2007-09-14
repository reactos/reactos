<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="sndvol32" type="win32gui" installbase="system32" installname="sndvol32.exe">
	<include base="ReactOS">include/wine</include>
	<include base="sndvol32">.</include>
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>ntdll</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>comctl32</library>
	<library>shell32</library>
	<library>winmm</library>
	<pch>sndvol32.h</pch>
	<file>misc.c</file>
	<file>mixer.c</file>
	<file>sndvol32.c</file>
	<file>sndvol32.rc</file>
</module>
