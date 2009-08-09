<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../../tools/rbuild/project.dtd">
<module name="sb16_ks" type="kernelmodedriver" installbase="system32/drivers" installname="sb16_ks.sys">
	<include base="sb16_ks">.</include>
	<include base="sb16_ks">..</include>
	<library>ntoskrnl</library>
	<library>portcls</library>
	<file>stdunk.cpp</file>
	<file>adapter.cpp</file>
	<file>main.cpp</file>
</module>
