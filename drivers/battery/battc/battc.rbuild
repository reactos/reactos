<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="battc" type="kernelmodedriver" installbase="system32/drivers" installname="battc.sys">
	<library>ntoskrnl</library>
	<library>hal</library>
	<importlibrary definition="battc.spec"/>
	<include base="battc">.</include>
	<file>battc.c</file>
	<file>battc.rc</file>
</module>
