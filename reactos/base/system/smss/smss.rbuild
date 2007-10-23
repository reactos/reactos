<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="smss" type="nativecui" installbase="system32" installname="smss.exe">
	<include base="smss">.</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<define name="_DISABLE_TIDENTS" />
	<library>nt</library>
	<library>smlib</library>
	<library>ntdll</library>
	<linkerflag>-lgcc</linkerflag>
	<pch>smss.h</pch>
	<compilationunit name="unit.c">
		<file>client.c</file>
		<file>debug.c</file>
		<file>init.c</file>
		<file>initdosdev.c</file>
		<file>initenv.c</file>
		<file>initheap.c</file>
		<file>initmv.c</file>
		<file>initobdir.c</file>
		<file>initpage.c</file>
		<file>initreg.c</file>
		<file>initrun.c</file>
		<file>initss.c</file>
		<file>initwkdll.c</file>
		<file>print.c</file>
		<file>smapi.c</file>
		<file>smapicomp.c</file>
		<file>smapiexec.c</file>
		<file>smapiquery.c</file>
		<file>smss.c</file>
	</compilationunit>
	<file>smss.rc</file>
</module>
