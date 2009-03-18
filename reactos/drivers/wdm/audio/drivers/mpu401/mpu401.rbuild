<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="mpu401" type="kernelmodedriver" installbase="system32/drivers" installname="mpu401.sys" allowwarnings="true">
	<include base="mpu401">.</include>
	<include base="mpu401">..</include>
	<library>ntoskrnl</library>
	<library>portcls</library>
	<file>mpu401.rc</file>
	<file>adapter.cpp</file>
</module>
