<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="sysaudio" type="kernelmodedriver" installbase="system32/drivers" installname="sysaudio.sys">
	<include base="sysaudio">.</include>
	<library>ntoskrnl</library>
	<define name="_NTDDK_" />
	<define name="_COMDDK_" />

	<file>main.c</file>

	<file>sysaudio.rc</file>
</module>
