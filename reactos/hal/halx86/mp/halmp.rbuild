<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="halmp" type="kernelmodedll" entrypoint="0">
	<importlibrary definition="../../hal/hal.def" />
	<bootstrap base="$(CDOUTPUT)" />
	<include base="hal_generic">../include</include>
	<include base="ntoskrnl">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="CONFIG_SMP" />
	<define name="_NTHAL_" />
	<define name="__USE_W32API" />
	<library>hal_generic</library>
	<library>hal_generic_pc</library>
	<library>ntoskrnl</library>
	<file>apic.c</file>
	<file>halinit.c</file>
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
</module>
