<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="audio_test" type="win32cui" installbase="system32" installname="audio_test.exe">
	<define name="PC_NO_IMPORTS" />
	<include base="ReactOS">include/reactos/libs/sound</include>
	<include base="wdmaud_kernel">.</include>
	<include base="libsamplerate">.</include>
	<library>setupapi</library>
	<library>libsamplerate</library>
	<library>ksuser</library>
	<file>audio_test.c</file>

</module>
