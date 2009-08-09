<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mmcclient" type="win32gui" installbase="system32" installname="mmc.exe" unicode="yes">
	<include base="mmcclient">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comdlg32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<library>comctl32</library>
	<file>console.c</file>
	<file>misc.c</file>
	<file>mmc.c</file>
	<file>mmc.rc</file>
	<pch>precomp.h</pch>
</module>
