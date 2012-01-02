<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="splitter" type="kernelmodedriver" installbase="system32/drivers" installname="splitter.sys">
	<library>ntoskrnl</library>
	<library>ks</library>
	<file>splitter.c</file>
	<file>filter.c</file>
	<file>pin.c</file>
</module>
