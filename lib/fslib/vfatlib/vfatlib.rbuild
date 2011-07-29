<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="vfatlib" type="staticlibrary">
	<include base="vfatlib">.</include>
	<file>fat12.c</file>
	<file>fat16.c</file>
	<file>fat32.c</file>
	<file>vfatlib.c</file>

	<directory name="check">
		<file>boot.c</file>
		<file>check.c</file>
		<file>common.c</file>
		<file>fat.c</file>
		<file>file.c</file>
		<file>io.c</file>
		<file>lfn.c</file>
	</directory>
</module>
