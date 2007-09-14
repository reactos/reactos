<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">

<!--
	Conflicts with Kernel Streaming MPU401 driver.
	Here for reference only.
-->

<module name="mpu401" type="kernelmodedriver">
	<include base="mpu401_nt4">.</include>
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>mpu401.c</file>
	<file>portio.c</file>
	<file>settings.c</file>
	<file>mpu401.rc</file>
</module>
