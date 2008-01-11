<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="charmap" type="win32gui" installbase="system32" installname="charmap.exe" unicode="yes">
	<include base="charmap">.</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>ntdll</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<compilationunit name="unit.c">
		<file>about.c</file>
		<file>charmap.c</file>
		<file>lrgcell.c</file>
		<file>map.c</file>
	</compilationunit>
	<file>charmap.rc</file>
	<pch>precomp.h</pch>
</module>
