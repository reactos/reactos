<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="autochk" type="nativecui" installbase="system32" installname="autochk.exe">
	<include base="autochk">.</include>
	<define name="_DISABLE_TIDENTS" />
	<library>nt</library>
	<library>ntdll</library>
	<file>autochk.c</file>
	<file>autochk.rc</file>
</module>
