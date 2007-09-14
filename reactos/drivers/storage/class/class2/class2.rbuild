<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="class2" type="kernelmodedriver" installbase="system32/drivers" installname="class2.sys">
	<bootstrap base="$(CDOUTPUT)" />
	<define name="__USE_W32API" />
	<importlibrary definition="class2.def" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>scsiport</library>
	<include base="class2">..</include>
	<file>class2.c</file>
	<file>class2.rc</file>
</module>
