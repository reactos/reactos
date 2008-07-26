<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="halarm_up" type="kernelmodedll" installbase="system32" installname="hal.dll">
	<importlibrary definition="../../hal/hal_amd64.def" />
	<bootstrap installbase="$(CDOUTPUT)" nameoncd="hal.dll" />
	<include base="halamd64_generic">../include</include>
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_NTHAL_" />
	<library>halamd64_generic</library>
	<library>ntoskrnl</library>
	<file>halinit_up.c</file>
	<file>halup.rc</file>
</module>
