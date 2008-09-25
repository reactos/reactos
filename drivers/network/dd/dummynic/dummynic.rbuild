<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="dummynic" type="kernelmodedriver" installbase="system32/drivers" installname="dummynic.sys">
	<include base="dummynic">.</include>
	<define name="NDIS50_MINIPORT" />
	<define name="NDIS_MINIPORT_DRIVER" />
	<define name="NDIS_LEGACY_MINIPORT" />
	<define name="NDIS51_MINIPORT" />
	<library>ndis</library>
        <library>ntoskrnl</library>
        <library>hal</library>
	<file>main.c</file>
</module>
