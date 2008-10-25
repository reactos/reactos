<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="parallel" type="kernelmodedriver">
	<include base="parallel">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>parallel.c</file>
	<file>parallel.rc</file>
</module>
