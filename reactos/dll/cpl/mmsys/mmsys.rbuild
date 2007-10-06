<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mmsys" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_MMSYS}" installbase="system32" installname="mmsys.cpl" unicode="yes">
	<importlibrary definition="mmsys.def" />
	<include base="mmsys">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>msvcrt</library>
	<library>devmgr</library>
	<library>gdi32</library>
	<library>winmm</library>
	<library>advapi32</library>
	<file>mmsys.c</file>
	<file>sounds.c</file>
	<file>volume.c</file>
	<file>mmsys.rc</file>
</module>
