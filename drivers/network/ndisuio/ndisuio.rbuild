<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ndisuio" type="kernelmodedriver" installbase="system32/drivers" installname="ndisuio.sys">
	<include base="ndisuio">.</include>
	<define name="NDIS50" />
	<define name="_NTDRIVER_" />
	<library>ndis</library>
	<library>pseh</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<pch>ndisuio.h</pch>
	<file>main.c</file>
	<file>ndisuio.rc</file>
</module>
