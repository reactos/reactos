<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="subst" type="win32cui" installbase="system32" installname="subst.exe" >
	<include base="ReactOS">include/reactos/wine</include>
	<include base="subst">.</include>
	<library>kernel32</library>
	<file>subst.c</file>
	<file>subst.rc</file>
</module>
