<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="mmebuddy" type="staticlibrary" allowwarnings="false" unicode="yes">
	<include base="ReactOS">include/reactos/libs/sound</include>
	<define name="DEBUG_NT4">1</define>
	<file>capabilities.c</file>
	<file>devicelist.c</file>
	<file>deviceinstance.c</file>
	<file>functiontable.c</file>
    <file>mmewrap.c</file>
	<file>reentrancy.c</file>
	<file>utility.c</file>
	<file>kernel.c</file>
    <file>thread.c</file>
	<directory name="wave">
		<file>widMessage.c</file>
		<file>wodMessage.c</file>
		<file>format.c</file>
		<file>header.c</file>
		<file>streaming.c</file>
	</directory>
	<directory name="midi">
		<file>midMessage.c</file>
		<file>modMessage.c</file>
	</directory>
	<directory name="mixer">
		<file>mxdMessage.c</file>
	</directory>
	<directory name="auxiliary">
		<file>auxMessage.c</file>
	</directory>
</module>
