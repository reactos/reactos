<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="hidclass" type="kernelmodedriver" entrypoint="0" installbase="system32/drivers" installname="hidclass.sys">
	<importlibrary definition="hidclass.spec" />
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<library>ntoskrnl</library>
	<library>hidparse</library>
	<library>hal</library>
	<file>fdo.c</file>
	<file>hidclass.c</file>
	<file>hidclass.rc</file>
	<file>pdo.c</file>
</module>
