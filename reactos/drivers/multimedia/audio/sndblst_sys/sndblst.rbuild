<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="sndblst_sys" type="kernelmodedriver">
	<include base="sndblst_sys">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>sndblst.c</file>
	<file>sndblst.rc</file>
</module>
