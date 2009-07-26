<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="kmixer" type="kernelmodedriver" installbase="system32/drivers" installname="kmixer.sys">
	<include base="kmixer">.</include>
	<include base="libsamplerate">.</include>
	<library>ntoskrnl</library>
	<library>ks</library>
	<library>hal</library>
	<library>libcntpr</library>
	<library>libsamplerate</library>
	<file>kmixer.c</file>
	<file>filter.c</file>
	<file>pin.c</file>
</module>
