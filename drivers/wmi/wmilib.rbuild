<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="wmilib" type="kernelmodedriver" installbase="system32/drivers" installname="wmilib.sys" entrypoint="0">
	<importlibrary definition="wmilib.spec" />
	<include base="wmilib">.</include>
	<library>ntoskrnl</library>
	<file>wmilib.c</file>
	<file>wmilib.rc</file>
</module>
