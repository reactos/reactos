<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic" type="objectlibrary">
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHAL_" />
		<define name="_X86BIOS_" />
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
			<file>cmos.c</file>
			<file>dma.c</file>
			<file>drive.c</file>
			<file>display.c</file>
			<file>profil.c</file>
			<file>reboot.c</file>
			<file>sysinfo.c</file>
			<file>timer.c</file>
			<if property="ARCH" value="i386">
				<file>bios.c</file>
				<file>halinit.c</file>
				<file>misc.c</file>
				<file>usage.c</file>
				<directory name="i386">
					<file>portio.c</file>
					<file>systimer.S</file>
					<file>v86.s</file>
				</directory>
			</if>
			<if property="ARCH" value="amd64">
				<directory name="amd64">
					<file>halinit.c</file>
					<file>irq.s</file>
					<file>misc.c</file>
					<file>pic.c</file>
					<file>systimer.S</file>
					<file>usage.c</file>
					<file>x86bios.c</file>
				</directory>
			</if>
		</directory>
		<if property="ARCH" value="amd64">
			<directory name="mp">
				<file>apic.c</file>
			</directory>
		</if>
		<directory name="include">
			<pch>hal.h</pch>
		</directory>
	</module>
</group>
