<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="swenum" type="kernelmodedriver" installbase="system32/drivers" installname="swenum.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<library>ntoskrnl</library>
	<library>ks</library>
	<file>swenum.c</file>
</module>
