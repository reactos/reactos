<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic" type="objectlibrary">
		<include base="hal_generic">../include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHAL_" />
		<file>beep.c</file>
		<file>bios.c</file>
		<file>bus.c</file>
		<file>cmos.c</file>
		<file>dma.c</file>
		<file>drive.c</file>
		<file>display.c</file>
		<file>halinit.c</file>
		<file>misc.c</file>
		<file>portio.c</file>
		<file>profil.c</file>
		<file>reboot.c</file>
		<file>sysinfo.c</file>
		<file>timer.c</file>
		<file>systimer.S</file>
		<file>v86.s</file>
		<pch>../include/hal.h</pch>
	</module>
	<module name="hal_generic_up" type="objectlibrary">
		<include base="hal_generic_up">../include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHAL_" />
		<file>irq.S</file>
		<file>processor.c</file>
		<file>spinlock.c</file>
	</module>
	<module name="hal_generic_pc" type="objectlibrary">
		<include base="hal_generic_pc">../include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHAL_" />
		<file>pci.c</file>
	</module>
</group>
