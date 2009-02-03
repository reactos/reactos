<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="wdmaud_kernel" type="kernelmodedriver" installbase="system32/drivers" installname="wdmaud.sys">
	<include base="wdmaud_kernel">.</include>
	<library>ntoskrnl</library>
	<library>ks</library>
	<file>entry.c</file>
</module>
