<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="rasacd" type="kernelmodedriver" installbase="system32/drivers" installname="rasacd.sys">
	<include base="rasacd">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<directory name="include">
		<pch>acdapi.h</pch>
	</directory>
	<directory name="acd">
		<file>main.c</file>
	</directory>
	<file>rasacd.rc</file>
</module>
