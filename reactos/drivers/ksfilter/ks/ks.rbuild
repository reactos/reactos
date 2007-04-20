<module name="ks" type="exportdriver" installbase="system32/drivers" installname="ks.sys" allowwarnings="true">
	<include base="ks">.</include>
	<include base="ks">..</include>
	<include base="ks">../include</include>
	<importlibrary definition="ks.def" />
	<library>ntoskrnl</library>
	<define name="__USE_W32API" />
	<define name="BUILDING_KS" />
	<define name="_NTDDK_" />
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
</module>
