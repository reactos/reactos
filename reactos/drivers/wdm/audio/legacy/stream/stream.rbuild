<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="stream" type="kernelmodedriver" installbase="system32/drivers" installname="stream.sys" entrypoint="0">
	<include base="stream">.</include>
	<define name="_COMDDK_" />
	<importlibrary definition="stream.spec" />
	<library>ntoskrnl</library>
	<library>ks</library>
	<library>pseh</library>
	<file>control.c</file>
	<file>driver.c</file>
	<file>dll.c</file>
	<file>filter.c</file>
	<file>helper.c</file>
	<file>pnp.c</file>
	<file>stream.rc</file>
</module>
