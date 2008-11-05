<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ks" type="kernelmodedriver" installbase="system32/drivers" installname="ks.sys" allowwarnings="true">
	<include base="ks">.</include>
	<include base="ks">..</include>
	<include base="ks">../include</include>
	<importlibrary definition="ks.spec" />
	<library>ntoskrnl</library>
	<define name="BUILDING_KS" />
	<define name="_NTDDK_" />
	<define name="_COMDDK_" />
	<file>ks.rc</file>
	<file>allocators.c</file>
	<file>clocks.c</file>
	<file>connectivity.c</file>
	<file>events.c</file>
	<file>irp.c</file>
	<file>methods.c</file>
	<file>misc.c</file>
	<file>properties.c</file>
	<file>topology.c</file>
	<file>worker.c</file>
	<file>kcom.c</file>
</module>
