<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ndis" type="kernelmodedriver" installbase="system32/drivers" installname="ndis.sys">
	<importlibrary definition="ndis-$(ARCH).def"></importlibrary>
	<include base="ndis">include</include>
	<define name="NDIS_WRAPPER" />
	<define name="NDIS51" />
	<define name="NDIS51_MINIPORT" />
	<define name="NDIS_LEGACY_DRIVER" />
	<define name="NDIS_LEGACY_MINIPORT" />
	<define name="NDIS_LEGACY_PROTOCOL" />
	<define name="NDIS_MINIPORT_DRIVER" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<directory name="include">
		<pch>ndissys.h</pch>
	</directory>
	<directory name="ndis">
		<file>30stubs.c</file>
		<file>40stubs.c</file>
		<file>50stubs.c</file>
		<file>buffer.c</file>
		<file>cl.c</file>
		<file>cm.c</file>
		<file>co.c</file>
		<file>config.c</file>
		<file>control.c</file>
		<file>efilter.c</file>
		<file>hardware.c</file>
		<file>io.c</file>
		<file>main.c</file>
		<file>memory.c</file>
		<file>miniport.c</file>
		<file>misc.c</file>
		<file>protocol.c</file>
		<file>string.c</file>
		<file>time.c</file>
	</directory>
	<file>ndis.rc</file>
</module>
