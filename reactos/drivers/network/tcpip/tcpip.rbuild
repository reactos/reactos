<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="tcpip" type="kernelmodedriver" installbase="system32/drivers" installname="tcpip.sys">
	<importlibrary definition="tcpip.spec" />
	<include base="tcpip">include</include>
	<include base="oskittcp">include</include>
	<define name="NDIS40" />
	<define name="_NTDRIVER_" />
	<library>ip</library>
	<library>oskittcp</library>
	<library>ndis</library>
	<library>pseh</library>
	<library>chew</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<directory name="include">
		<pch>precomp.h</pch>
	</directory>
	<directory name="datalink">
		<file>lan.c</file>
	</directory>
	<directory name="recmutex">
		<file>recmutex.c</file>
	</directory>
	<directory name="tcpip">
		<file>buffer.c</file>
		<file>bug.c</file>
		<file>dispatch.c</file>
		<file>fileobjs.c</file>
		<file>iinfo.c</file>
		<file>info.c</file>
		<file>irp.c</file>
		<file>lock.c</file>
		<file>main.c</file>
		<file>ninfo.c</file>
		<file>pool.c</file>
		<file>proto.c</file>
		<file>tinfo.c</file>
		<file>wait.c</file>
	</directory>
	<file>tcpip.rc</file>
</module>
