<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="tdilib" type="staticlibrary">
	<include base="iphlpapi">.</include>
	<include base="tdilib">.</include>
	<library>ntdll</library>
	<file>enum.c</file>
	<file>handle.c</file>
</module>
