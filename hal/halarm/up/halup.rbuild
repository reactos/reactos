<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="halarm_up" type="kernelmodedll" installbase="system32" installname="hal.dll">
	<importlibrary base="hal" definition="hal.pspec" />
	<bootstrap installbase="$(CDOUTPUT)" nameoncd="hal.dll" />
	<include base="halarm_generic">../include</include>
	<include base="ntoskrnl">include</include>
	<define name="_NTHAL_" />
	<library>halarm_generic</library>
	<library>ntoskrnl</library>
	<file>halinit_up.c</file>
	<file>halup.rc</file>
</module>
