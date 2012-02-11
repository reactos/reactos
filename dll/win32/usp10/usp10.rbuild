<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="usp10" type="win32dll" baseaddress="${BASEADDRESS_USP10}" installbase="system32" installname="usp10.dll" allowwarnings="true">
	<importlibrary definition="usp10.spec" />
	<include base="usp10">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>bidi.c</file>
	<file>breaking.c</file>
	<file>usp10.c</file>
	<file>indic.c</file>
	<file>linebreak.c</file>
	<file>indicsyllable.c</file>
	<file>mirror.c</file>
	<file>opentype.c</file>
	<file>shape.c</file>
	<file>shaping.c</file>
	<library>wine</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>ntdll</library>
	<library>msvcrt</library>
</module>
</group>
