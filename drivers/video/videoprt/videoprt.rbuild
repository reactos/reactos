<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="videoprt" type="kernelmodedriver" installbase="system32/drivers" installname="videoprt.sys">
	<importlibrary definition="videoprt.def" />
	<include base="videoprt">.</include>
	<include base="ntoskrnl">include</include>
	<define name="_VIDEOPORT_" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<pch>videoprt.h</pch>
	<file>agp.c</file>
	<file>ddc.c</file>
	<file>dispatch.c</file>
	<file>dma.c</file>
	<file>event.c</file>
	<file>funclist.c</file>
	<file>int10.c</file>
	<file>interrupt.c</file>
	<file>resource.c</file>
	<file>services.c</file>
	<file>spinlock.c</file>
	<file>timer.c</file>
	<file>videoprt.c</file>
	<file>videoprt.rc</file>
</module>
