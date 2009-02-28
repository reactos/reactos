<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="sysaudio" type="kernelmodedriver" installbase="system32/drivers" installname="sysaudio.sys">
	<include base="sysaudio">.</include>
	<library>ntoskrnl</library>
	<library>ks</library>
	<library>hal</library>
	<library>libcntpr</library>
	<define name="_COMDDK_" />
	<file>control.c</file>
	<file>deviface.c</file>
	<file>dispatcher.c</file>
	<file>main.c</file>
	<file>pin.c</file>
	<file>sysaudio.rc</file>
</module>
