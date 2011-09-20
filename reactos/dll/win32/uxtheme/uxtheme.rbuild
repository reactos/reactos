<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="uxtheme" type="win32dll" baseaddress="${BASEADDRESS_UXTHEME}" installbase="system32" installname="uxtheme.dll" allowwarnings="true">
	<importlibrary definition="uxtheme.spec" />
	<include base="uxtheme">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<pch>uxthemep.h</pch>
	<file>buffer.c</file>
	<file>draw.c</file>
	<file>main.c</file>
	<file>metric.c</file>
	<file>msstyles.c</file>
	<file>ncscrollbar.c</file>
	<file>nonclient.c</file>
	<file>property.c</file>
	<file>stylemap.c</file>
	<file>system.c</file>
	<file>themehooks.c</file>
	<file>uxini.c</file>
	<file>version.rc</file>
	<library>wine</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>msimg32</library>
	<library>ntdll</library>
</module>
</group>
