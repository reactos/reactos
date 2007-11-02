<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="halup" type="kernelmodedll" entrypoint="0">
	<importlibrary definition="../../hal/hal.def" />
	<bootstrap nameoncd="hal.dll" />
	<include base="hal_generic">../include</include>
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_NTHAL_" />
	<linkerflag>-enable-stdcall-fixup</linkerflag>
	<library>hal_generic</library>
	<library>hal_generic_up</library>
	<library>hal_generic_pc</library>
	<library>ntoskrnl</library>
	<file>halinit_up.c</file>
	<file>halup.rc</file>
</module>
