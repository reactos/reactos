<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="hal" type="kernelmodedll" installbase="system32" installname="hal.dll">
	<importlibrary base="hal" definition="../../hal.pspec" />
	<bootstrap installbase="$(CDOUTPUT)" nameoncd="hal.dll" />
	<include base="hal">../include</include>
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_NTHAL_" />
	<library>hal_generic_amd64</library>
	<library>hal_generic</library>
	<library>ntoskrnl</library>
	<file>halinit_up.c</file>
	<file>halup.rc</file>
	<file>processor.c</file>
</module>
