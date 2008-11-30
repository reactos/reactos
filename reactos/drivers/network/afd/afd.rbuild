<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="afd" type="kernelmodedriver" installbase="system32/drivers" installname="afd.sys">
	<include base="afd">include</include>
	<include base="ReactOS">include/reactos/drivers</include>
	<include base="ReactOS">include/ndk</include>
	<library>pseh</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<directory name="include">
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38269
	<pch>afd.h</pch>
	-->
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
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38054#c7 -->
	<compilerflag>-fno-unit-at-a-time</compilerflag>
</module>
