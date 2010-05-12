<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group>
	<module name="mini_hal" type="objectlibrary" crt="static">
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHALDLL_" />
		<define name="_NTHAL_" />
		<define name="_BLDR_" />
		<define name="_MINIHAL_" />
		<directory name="generic">
			<directory name="bus">
				<file>bushndlr.c</file>
				<file>isabus.c</file>
				<file>halbus.c</file>
				<file>pcibus.c</file>
				<file>pcidata.c</file>
				<file>sysbus.c</file>
			</directory>
			<file>beep.c</file>
			<file>bios.c</file>
			<file>cmos.c</file>
			<file>dma.c</file>
			<file>display.c</file>
			<file>drive.c</file>
			<file>misc.c</file>
			<file>profil.c</file>
			<file>reboot.c</file>
			<file>spinlock.c</file>
			<file>sysinfo.c</file>
			<file>timer.c</file>
			<file>usage.c</file>
			<directory name="i386">
				<file>portio.c</file>
				<file>systimer.S</file>
			</directory>
		</directory>
		<directory name="up">
			<file>halinit_up.c</file>
			<file>pic.c</file>
			<file>processor.c</file>
		</directory>
	</module>
</group>
