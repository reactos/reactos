<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="null" type="kernelmodedriver" installbase="system32/drivers" installname="null.sys">
	<include base="null">.</include>
	<library>pseh</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>null.c</file>
	<file>null.rc</file>
</module>
