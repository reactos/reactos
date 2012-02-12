<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mouhid" type="kernelmodedriver" installbase="system32/drivers" installname="mouhid.sys">
	<define name="DEBUG_MODE" />
	<include base="ntoskrnl">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>hidparse</library>
	<file>mouhid.c</file>
	<file>mouhid.rc</file>
</module>
