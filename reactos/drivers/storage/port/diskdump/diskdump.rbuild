<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="diskdump" type="kernelmodedriver" installbase="system32/drivers" installname="diskdump.sys">
	<bootstrap base="$(CDOUTPUT)" />
	<importlibrary definition="diskdump.def" />
	<define name="__USE_W32API" />
	<include base="ReactOS">include/reactos/drivers</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>class2</library>
	<include base="diskdump">..</include>
	<file>diskdump.c</file>
	<file>diskdump_helper.S</file>
	<file>diskdump.rc</file>
</module>
