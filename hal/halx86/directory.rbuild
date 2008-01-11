<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<directory name="generic">
		<xi:include href="generic/generic.rbuild" />
	</directory>

	<module name="halup" type="kernelmodedll" entrypoint="0">
		<importlibrary definition="../hal/hal.def" />
		<bootstrap installbase="$(CDOUTPUT)" nameoncd="hal.dll" />
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
	<module name="halmp" type="kernelmodedll" entrypoint="0">
		<importlibrary definition="../hal/hal.def" />
		<bootstrap installbase="$(CDOUTPUT)" />
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_DISABLE_TIDENTS" />
		<define name="CONFIG_SMP" />
		<define name="_NTHAL_" />
		<library>hal_generic</library>
		<library>hal_generic_pc</library>
		<library>ntoskrnl</library>
		<directory name="mp">
			<file>apic.c</file>
			<file>halinit_mp.c</file>
			<file>ioapic.c</file>
			<file>ipi_mp.c</file>
			<file>mpconfig.c</file>
			<file>mps.S</file>
			<file>mpsboot.asm</file>
			<file>mpsirql.c</file>
			<file>processor_mp.c</file>
			<file>spinlock.c</file>
			<file>halmp.rc</file>
		</directory>
	</module>
	<module name="halxbox" type="kernelmodedll" entrypoint="0" allowwarnings="true">
		<importlibrary definition="../hal/hal.def" />
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_DISABLE_TIDENTS" />
		<define name="_NTHAL_" />
		<define name="SARCH_XBOX" />
		<library>hal_generic</library>
		<library>hal_generic_up</library>
		<library>ntoskrnl</library>
		<directory name="generic">
			<file>pci.c</file>
		</directory>
		<directory name="xbox">
			<file>halinit_xbox.c</file>
			<file>part_xbox.c</file>
			<file>halxbox.rc</file>
			<pch>halxbox.h</pch>
		</directory>
	</module>
</group>
