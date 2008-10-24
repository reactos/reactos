<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mmsys" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_MMSYS}" installbase="system32" installname="mmsys.cpl" unicode="yes">
	<importlibrary definition="mmsys.spec" />
	<include base="mmsys">.</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>devmgr</library>
	<library>gdi32</library>
	<library>winmm</library>
	<library>advapi32</library>
	<library>shell32</library>
	<library>setupapi</library>
	<file>mmsys.c</file>
	<file>sounds.c</file>
	<file>volume.c</file>
	<file>audio.c</file>
	<file>voice.c</file>
	<file>mmsys.rc</file>
</module>
