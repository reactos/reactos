<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ks" type="kernelmodedriver" installbase="system32/drivers" installname="ks.sys">
	<include base="ks">.</include>
	<include base="ks">..</include>
	<include base="ks">../include</include>
	<importlibrary definition="ks.spec" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>pseh</library>
	<define name="BUILDING_KS" />
	<define name="_COMDDK_" />
	<file>ks.rc</file>
	<file>api.c</file>
	<file>allocators.c</file>
	<file>bag.c</file>
	<file>device.c</file>
	<file>deviceinterface.c</file>
	<file>driver.c</file>
	<file>clocks.c</file>
	<file>connectivity.c</file>
	<file>event.c</file>
	<file>filter.c</file>
	<file>filterfactory.c</file>
	<file>image.c</file>
	<file>irp.c</file>
	<file>methods.c</file>
	<file>misc.c</file>
	<file>property.c</file>
	<file>topology.c</file>
	<file>worker.c</file>
	<file>kcom.c</file>
</module>
