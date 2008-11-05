<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="portcls" type="kernelmodedriver" installbase="system32/drivers" installname="portcls.sys" allowwarnings="true">

	<!-- MinGW32-specific linker options. Worth having but not essential. -->
	<!--
	<linkerflag>-fno-exceptions</linkerflag>
	<linkerflag>-fno-rtti</linkerflag>
	-->
	<compilerflag compiler="cpp">-Wno-non-virtual-dtor</compilerflag>
	<importlibrary definition="portcls.spec" />

	<define name="_NTDDK_" />
	<define name="PC_NO_IMPORTS" />

	<include base="portcls">../include</include>

	<library>ntoskrnl</library>
	<library>ks</library>
	<library>drmk</library>

	<file>dll.c</file>
	<file>adapter.c</file>
	<file>irp.c</file>
	<file>drm.c</file>
	<file>stubs.c</file>
	<file>undoc.c</file>

	<!-- Probably not the best idea to have this separate -->
	<!--<file>../stdunk/stdunk.c</file>-->

	<file>ResourceList.c</file>

	<file>port_factory.c</file>
	<file>Port.cpp</file>
	<file>PortDMus.cpp</file>
	<file>PortMidi.cpp</file>
	<file>PortTopology.cpp</file>
	<file>PortWaveCyclic.cpp</file>
	<file>PortWavePci.cpp</file>

	<file>miniport_factory.cpp</file>
	<file>Miniport.cpp</file>
	<file>MiniportDMus.cpp</file>
	<file>MiniportMidi.cpp</file>
	<file>MiniportTopology.cpp</file>
	<file>MiniportWaveCyclic.cpp</file>
	<file>MiniportWavePci.cpp</file>

	<file>portcls.rc</file>
</module>
