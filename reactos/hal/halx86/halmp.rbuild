<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
	<module name="halmp" type="kernelmodedll" entrypoint="0">
		<importlibrary base="hal" definition="hal.pspec" />
		<bootstrap installbase="$(CDOUTPUT)" />
		<include>include</include>
		<include base="ntoskrnl">include</include>
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
</group>
