<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="hal" type="kernelmodedll" entrypoint="HalInitSystem@8" installbase="system32" installname="hal.dll">
		<importlibrary base="hal" definition="../hal.pspec" />
		<bootstrap installbase="$(CDOUTPUT)" />
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHAL_" />
		<library>hal_generic</library>
		<library>ntoskrnl</library>

		<directory name="versa">
			<file>halinit_up.c</file>
			<file>halup.rc</file>
		</directory>
	</module>
</group>
