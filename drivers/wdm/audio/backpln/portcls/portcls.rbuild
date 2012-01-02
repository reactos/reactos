<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="portcls" type="kernelmodedriver" installbase="system32/drivers" installname="portcls.sys" entrypoint="0">
	<importlibrary definition="portcls.spec" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<include base="portcls">.</include>
	<library>ntoskrnl</library>
	<library>ks</library>
	<library>drmk</library>
	<library>hal</library>
	<library>libcntpr</library>
	<library>pseh</library>

	<group compilerset="gcc">
		<compilerflag compiler="cxx">-fno-exceptions</compilerflag>
		<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	</group>

	<group compilerset="msc">
		<compilerflag compiler="cxx">/GR-</compilerflag>
	</group>

	<file>adapter.cpp</file>
	<file>api.cpp</file>
	<file>connection.cpp</file>
	<file>dispatcher.cpp</file>
	<file>dll.cpp</file>
	<file>dma_slave.cpp</file>
	<file>drm.cpp</file>
	<file>drm_port.cpp</file>
	<file>filter_topology.cpp</file>
	<file>filter_dmus.cpp</file>
	<file>filter_wavecyclic.cpp</file>
	<file>filter_wavepci.cpp</file>
	<file>filter_wavert.cpp</file>
	<file>guids.cpp</file>
	<file>interrupt.cpp</file>
	<file>irp.cpp</file>
	<file>irpstream.cpp</file>
	<file>miniport.cpp</file>
	<file>miniport_dmus.cpp</file>
	<file>miniport_fmsynth.cpp</file>
	<file>pin_dmus.cpp</file>
	<file>pin_wavecyclic.cpp</file>
	<file>pin_wavepci.cpp</file>
	<file>pin_wavert.cpp</file>
	<file>pool.cpp</file>
	<file>port.cpp</file>
	<file>port_dmus.cpp</file>
	<file>port_topology.cpp</file>
	<file>port_wavecyclic.cpp</file>
	<file>port_wavepci.cpp</file>
	<file>port_wavert.cpp</file>
	<file>port_wavertstream.cpp</file>
	<file>power.cpp</file>
	<file>propertyhandler.cpp</file>
	<file>purecall.cpp</file>
	<file>registry.cpp</file>
	<file>resource.cpp</file>
	<file>service_group.cpp</file>
	<file>undoc.cpp</file>
	<file>unregister.cpp</file>
	<file>version.cpp</file>
	<file>portcls.rc</file>
	<pch>private.hpp</pch>
</module>
