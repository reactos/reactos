<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="sndblst" type="kernelmodedriver">
	<include base="sndblst">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>card.c</file>
	<file>dma.c</file>
	<file>irq.c</file>
	<file>portio.c</file>
	<file>settings.c</file>
	<file>sndblst.c</file>
	<file>sndblst.rc</file>
</module>
