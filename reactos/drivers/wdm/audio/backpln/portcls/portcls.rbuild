<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="portcls" type="kernelmodedriver" installbase="system32/drivers" installname="portcls.sys" entrypoint="0">
	<importlibrary definition="portcls.spec" />
	<define name="PC_NO_IMPORTS" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<include base="portcls">../include</include>
	<library>ntoskrnl</library>
	<library>ks</library>
	<library>drmk</library>
	<library>rtl</library>
	<library>hal</library>
	<library>libcntpr</library>
	<file>adapter.c</file>
	<file>api.c</file>
	<file>connection.c</file>
	<file>dispatcher.c</file>
	<file>dll.c</file>
	<file>dma_slave.c</file>
	<file>drm.c</file>
	<file>drm_port.c</file>
	<file>filter_wavecyclic.c</file>
	<file>filter_wavepci.c</file>
	<file>filter_wavert.c</file>
	<file>guids.c</file>
	<file>interrupt.c</file>
	<file>irp.c</file>
	<file>irpstream.c</file>
	<file>miniport.c</file>
	<file>miniport_dmus.c</file>
	<file>miniport_fmsynth.c</file>
	<file>pin_wavecyclic.c</file>
	<file>pin_wavepci.c</file>
	<file>pin_wavert.c</file>
	<file>pool.c</file>
	<file>port.c</file>
	<file>port_dmus.c</file>
	<file>port_midi.c</file>
	<file>port_topology.c</file>
	<file>port_wavecyclic.c</file>
	<file>port_wavepci.c</file>
	<file>port_wavepcistream.c</file>
	<file>port_wavert.c</file>
	<file>port_wavertstream.c</file>
	<file>power.c</file>
	<file>propertyhandler.c</file>
	<file>registry.c</file>
	<file>resource.c</file>
	<file>service_group.c</file>
	<file>undoc.c</file>
	<file>version.c</file>
	<file>portcls.rc</file>
</module>
