<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="mmixer" type="staticlibrary" allowwarnings="false" unicode="yes">
	<include base="ReactOS">include/reactos/libs/sound</include>
	<define name="NDEBUG">1</define>
	<file>controls.c</file>
	<file>filter.c</file>
	<file>midi.c</file>
	<file>mixer.c</file>
	<file>sup.c</file>
	<file>wave.c</file>
	<file>topology.c</file>
	<pch>priv.h</pch>
</module>
