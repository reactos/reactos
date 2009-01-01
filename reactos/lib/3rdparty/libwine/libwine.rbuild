<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="wine" type="staticlibrary">
	<include base="ReactOS">include/reactos/wine</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__WINESRC__" />
	<file>config.c</file>
	<file>debug_ros.c</file>
	<file>string.c</file>
</module>
