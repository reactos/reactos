<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="hal" type="kernelmodedll" entrypoint="HalInitSystem">
	<importlibrary definition="../../hal/hal.pspec" />
	<bootstrap installbase="$(CDOUTPUT)" nameoncd="hal.dll" />
	<include base="halppc_generic">../include</include>
	<include base="ntoskrnl">include</include>
	<define name="_NTHAL_" />
	<library>halppc_generic</library>
	<library>ntoskrnl</library>
	<file>halinit_up.c</file>
	<file>halup.rc</file>
</module>
