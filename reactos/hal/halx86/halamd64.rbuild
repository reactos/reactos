<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="hal" type="kernelmodedll" entrypoint="HalInitSystem" installbase="system32" installname="hal.dll">
	<importlibrary base="hal" definition="../hal.pspec" />
	<bootstrap installbase="$(CDOUTPUT)" nameoncd="hal.dll" />
	<include base="hal">include</include>
	<include base="ntoskrnl">include</include>
	<include base="x86emu">.</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_NTHAL_" />
	<define name="_X86BIOS_" />
	<library>hal_generic</library>
	<library>hal_generic_up</library>
	<library>ntoskrnl</library>
	<library>x86emu</library>

	<directory name="up">
		<file>halinit_up.c</file>
		<file>halup.rc</file>
	</directory>

	<directory name="mp">
		<directory name="amd64">
			<file>mps.S</file>
		</directory>
	</directory>
	<directory name="generic">
		<directory name="amd64">
			<file>x86bios.c</file>
		</directory>
	</directory>

</module>
</group>

