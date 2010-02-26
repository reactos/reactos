<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="halxbox" type="kernelmodedll" entrypoint="HalInitSystem@8">
		<importlibrary base="hal" definition="../hal.pspec" />
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHAL_" />
		<define name="SARCH_XBOX" />
		<library>hal_generic</library>
		<library>hal_generic_up</library>
		<library>ntoskrnl</library>
		<library>libcntpr</library>
		<directory name="xbox">
			<file>halinit_xbox.c</file>
			<file>part_xbox.c</file>
			<file>halxbox.rc</file>
			<pch>halxbox.h</pch>
		</directory>
	</module>
</group>
