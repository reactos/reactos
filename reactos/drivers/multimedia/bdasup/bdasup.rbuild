<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="bdasup" type="kernelmodedriver" installbase="system32/drivers" installname="bdasup.sys" entrypoint="0">
	<importlibrary definition="bdasup.spec" />
	<library>ntoskrnl</library>
	<library>ks</library>
	<library>pseh</library>
	<file>bdasup.c</file>
</module>
