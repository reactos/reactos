<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="drmk" type="kernelmodedriver" installbase="system32/drivers" installname="drmk.sys" allowwarnings="true">
	<include base="drmk">.</include>
	<include base="drmk">..</include>
	<include base="drmk">../include</include>
	<importlibrary definition="drmk.def" />
	<library>ntoskrnl</library>
	<define name="__USE_W32API" />
	<define name="BUILDING_DRMK" />
	<file>drmk.rc</file>
	<file>stubs.cpp</file>
</module>
