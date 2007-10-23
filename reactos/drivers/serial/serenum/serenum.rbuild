<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="serenum" type="kernelmodedriver" installbase="system32/drivers" installname="serenum.sys">
	<include base="serenum">.</include>
	<define name="NDEBUG" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>detect.c</file>
	<file>fdo.c</file>
	<file>misc.c</file>
	<file>pdo.c</file>
	<file>serenum.c</file>
	<file>serenum.rc</file>
</module>
