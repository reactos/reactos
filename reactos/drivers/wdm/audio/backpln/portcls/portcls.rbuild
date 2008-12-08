<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="portcls" type="kernelmodedriver" installbase="system32/drivers" installname="portcls.sys" allowwarnings="true">
	<importlibrary definition="portcls.spec" />
	<define name="_NTDDK_" />
	<define name="PC_NO_IMPORTS" />
	<include base="portcls">../include</include>
	<library>ntoskrnl</library>
	<library>ks</library>
	<library>drmk</library>
	<library>rtl</library>
	<file>api.c</file>
	<file>dll.c</file>
	<file>dma_master.c</file>
	<file>dma_slave.c</file>
	<file>adapter.c</file>
	<file>irp.c</file>
	<file>interrupt.c</file>
	<file>drm.c</file>
	<file>stubs.c</file>
	<file>undoc.c</file>
	<file>resource.c</file>
	<file>registry.c</file>
	<file>service_group.c</file>	
	<file>port.c</file>
	<file>port_dmus.c</file>
	<file>port_midi.c</file>
	<file>port_topology.c</file>
	<file>port_wavepci.c</file>
	<file>port_wavecyclic.c</file>
	<file>miniport.c</file>
	<file>miniport_dmus.c</file>
	<file>miniport_fmsynth.c</file>
	<file>portcls.rc</file>
</module>
