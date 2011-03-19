<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../tools/rbuild/project.dtd">
<group>
	<module name="hal_generic" type="objectlibrary">
		<include>include</include>
		<include base="ntoskrnl">include</include>
		<define name="_NTHALDLL_" />
		<define name="_NTHAL_" />
		<directory name="generic">
			<file>beep.c</file>
			<file>cmos.c</file>
			<file>display.c</file>
			<file>dma.c</file>
			<file>drive.c</file>
			<file>memory.c</file>
			<file>misc.c</file>
			<file>profil.c</file>
			<file>reboot.c</file>
			<file>sysinfo.c</file>
			<file>timer.c</file>
			<file>usage.c</file>
			<if property="ARCH" value="i386">
				<file>bios.c</file>
				<directory name="i386">
					<file>portio.c</file>
					<file>systimer.S</file>
					<file>trap.S</file>
				</directory>
			</if>
		</directory>
		<directory name="include">
			<pch>hal.h</pch>
		</directory>
	</module>
</group>
