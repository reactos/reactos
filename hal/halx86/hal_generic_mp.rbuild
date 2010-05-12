<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic_mp" type="objectlibrary">
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHALDLL_" />
		<define name="_NTHAL_" />
		<define name="CONFIG_SMP" />
		<directory name="generic">
			<file>spinlock.c</file>
		</directory>
		<directory name="mp">
			<file>apic.c</file>
			<file>halinit_mp.c</file>
			<file>ioapic.c</file>
			<file>ipi_mp.c</file>
			<file>mpconfig.c</file>
			<file>processor_mp.c</file>
			<file>halmp.rc</file>
		</directory>
	</module>
</group>
