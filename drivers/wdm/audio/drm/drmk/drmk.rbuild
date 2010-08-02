<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="drmk" type="kernelmodedriver" installbase="system32/drivers" installname="drmk.sys" entrypoint="0">
	<include base="drmk">.</include>
	<include base="drmk">..</include>
	<include base="drmk">../include</include>
	<group compilerset="gcc">
		<compilerflag compiler="cxx">-fno-exceptions</compilerflag>
		<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	</group>
	<importlibrary definition="drmk.spec" />
	<library>ntoskrnl</library>
	<define name="BUILDING_DRMK" />
	<file>stubs.cpp</file>
	<file>drmk.rc</file>
</module>
