<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="sound" type="kernelmodedriver">
	<library>ntoskrnl</library>
	<library>hal</library>
	<include base="sound">.\include</include>
	<file>dsp.c</file>
	<file>mixer.c</file>
	<file>sb16.c</file>
	<file>sb_waveout.c</file>
	<file>sound.c</file>
	<file>wave.c</file>
	<file>sb16.rc</file>
</module>
