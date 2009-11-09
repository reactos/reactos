<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mouclass" type="kernelmodedriver" installbase="system32/drivers" installname="mouclass.sys">
	<include base="mouclass">.</include>
	<library>pseh</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>misc.c</file>
	<file>mouclass.c</file>
	<file>mouclass.rc</file>
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38054#c7 -->
	<compilerflag compilerset="gcc">-fno-unit-at-a-time</compilerflag>
</module>
