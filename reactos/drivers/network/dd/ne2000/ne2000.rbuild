<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="ne2000" type="kernelmodedriver" installbase="system32/drivers" installname="ne2000.sys">
	<include base="ne2000">include</include>
	<library>ndis</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<directory name="ne2000">
		<file>8390.c</file>
		<file>main.c</file>
	</directory>
	<file>ne2000.rc</file>
</module>
