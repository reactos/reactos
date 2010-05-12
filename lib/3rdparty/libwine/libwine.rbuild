<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="wine" type="staticlibrary">
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>config.c</file>
	<file>debug_ros.c</file>
	<file>loader.c</file>
	<file>string.c</file>
</module>
<module name="wineldr" type="staticlibrary">
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>loader.c</file>
</module>
</group>