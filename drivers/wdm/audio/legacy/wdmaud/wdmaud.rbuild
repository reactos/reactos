<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="wdmaud_kernel" type="kernelmodedriver" installbase="system32/drivers" installname="wdmaud.sys">
	<include base="wdmaud_kernel">.</include>
	<define name="_COMDDK_" />
	<include base="ReactOS">include/reactos/libs/sound</include>
	<library>ntoskrnl</library>
	<library>ks</library>
	<library>pseh</library>
	<file>control.c</file>
	<file>deviface.c</file>
	<file>entry.c</file>
	<file>wdmaud.rc</file>
</module>
