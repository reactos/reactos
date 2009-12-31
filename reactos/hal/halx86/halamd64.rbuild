<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="hal" type="kernelmodedll" entrypoint="HalInitSystem" installbase="system32" installname="hal.dll">
	<importlibrary base="hal" definition="../hal.pspec" />
	<bootstrap installbase="$(CDOUTPUT)" nameoncd="hal.dll" />
	<include base="hal">include</include>
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_NTHAL_" />
	<library>hal_generic</library>
	<library>hal_generic_up</library>
	<library>ntoskrnl</library>

	<directory name="up">
		<file>halinit_up.c</file>
		<file>halup.rc</file>
	</directory>

	<directory name="mp">
		<directory name="amd64">
			<file>mps.S</file>
		</directory>
	</directory>
</module>
