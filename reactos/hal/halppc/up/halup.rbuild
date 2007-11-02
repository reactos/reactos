<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="halppc_up" type="kernelmodedll">
	<importlibrary definition="../../hal/hal.def" />
	<bootstrap nameoncd="hal.dll" />
	<include base="halppc_generic">../include</include>
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<define name="_NTHAL_" />
	<library>halppc_generic</library>
	<library>ntoskrnl</library>
	<file>halinit_up.c</file>
	<file>halup.rc</file>
</module>
