<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="hidparse" type="kernelmodedriver" installbase="system32/drivers" installname="hidparse.sys">
	<importlibrary definition="hidparse.spec" />
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<define name="DEBUG_MODE" />
	<include base="ntoskrnl">include</include>
	<include base="ReactOS">lib/drivers/hidparser</include>
	<library>ntoskrnl</library>
	<library>hidparser</library>
	<file>hidparse.c</file>
	<file>hidparse.rc</file>
</module>
