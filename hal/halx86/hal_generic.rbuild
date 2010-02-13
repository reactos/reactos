<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic" type="objectlibrary">
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHAL_" />
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
			<file>drive.c</file>
			<file>display.c</file>
			<file>halinit.c</file>
			<file>misc.c</file>
			<file>portio.c</file>
			<file>profil.c</file>
			<file>reboot.c</file>
			<file>sysinfo.c</file>
			<file>systimer.S</file>
			<file>timer.c</file>
			<file>trap.S</file>
			<file>usage.c</file>
		</directory>
		<directory name="include">
			<pch>hal.h</pch>
		</directory>
	</module>
</group>
