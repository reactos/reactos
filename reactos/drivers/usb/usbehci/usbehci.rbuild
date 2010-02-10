<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="usbehci" type="kernelmodedriver" installbase="system32/drivers" installname="usbehci.sys">
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>usbehci.c</file>
	<file>fdo.c</file>
	<file>pdo.c</file>
	<file>common.c</file>
	<file>misc.c</file>
	<file>irp.c</file>
      <file>usbiffn.c</file>
	<file>urbreq.c</file>
</module>
