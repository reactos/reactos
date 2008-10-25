<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="afd" type="kernelmodedriver" installbase="system32/drivers" installname="afd.sys">
	<include base="afd">include</include>
	<include base="ReactOS">include/reactos/drivers</include>
	<library>pseh</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<directory name="include">
	<pch>afd.h</pch>
	</directory>
	<directory name="afd">
		<file>bind.c</file>
		<file>connect.c</file>
		<file>context.c</file>
		<file>info.c</file>
		<file>listen.c</file>
		<file>lock.c</file>
		<file>main.c</file>
		<file>read.c</file>
		<file>select.c</file>
		<file>tdi.c</file>
		<file>tdiconn.c</file>
		<file>write.c</file>
	</directory>
	<file>afd.rc</file>
</module>
