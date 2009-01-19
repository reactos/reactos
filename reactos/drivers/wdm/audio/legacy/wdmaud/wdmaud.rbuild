<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="wdmaud_kernel" type="kernelmodedriver" installbase="system32/drivers" installname="wdmaud.sys">
	<include base="wdmaud">.</include>
	<include base="wdmaud">..</include>
	<library>ntoskrnl</library>
	<file>entry.c</file>
</module>
