<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="halup" type="kernelmodedll" entrypoint="0" installname="hal.dll">
		<importlibrary base="hal" definition="hal.spec" />
		<bootstrap installbase="$(CDOUTPUT)" />
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_DISABLE_TIDENTS" />
		<define name="_NTHAL_" />
		<library>hal_generic</library>
		<library>hal_generic_up</library>
		<library>hal_generic_pc</library>
		<library>ntoskrnl</library>
		<directory name="up">
			<file>halinit_up.c</file>
			<file>halup.rc</file>
		</directory>
	</module>
</group>
