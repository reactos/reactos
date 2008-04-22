<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="chew" type="staticlibrary">
	<define name="_NTOSKRNL_" />
	<include base="chew">include</include>
	<include base="ReactOS">include/ddk</include>
	<file>workqueue.c</file>
</module>