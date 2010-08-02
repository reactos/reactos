<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="wdmaud_kernel" type="kernelmodedriver" installbase="system32/drivers" installname="wdmaud.sys">
	<include base="wdmaud_kernel">.</include>
	<include base="mmixer">.</include>
	<include base="ReactOS">include/reactos/libs/sound</include>
	<define name="_COMDDK_" />
	<library>mmixer</library>
	<library>ntoskrnl</library>
	<library>rtl</library>
	<library>libcntpr</library>
	<library>ks</library>
	<library>pseh</library>
	<library>hal</library>
	<file>control.c</file>
	<file>deviface.c</file>
	<file>entry.c</file>
	<file>mmixer.c</file>
	<file>sup.c</file>
	<file>wdmaud.rc</file>
</module>
