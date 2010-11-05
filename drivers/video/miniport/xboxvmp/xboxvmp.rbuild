<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="xboxvmp" type="kernelmodedriver">
	<include base="xboxvmp">.</include>
	<library>ntoskrnl</library>
	<library>videoprt</library>
	<file>xboxvmp.c</file>
	<file>xboxvmp.rc</file>
</module>
