<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdclass" type="kernelmodedriver" installbase="system32/drivers" installname="kbdclass.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<library>pseh</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>kbdclass.c</file>
	<file>misc.c</file>
	<file>setup.c</file>
	<file>kbdclass.rc</file>
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38054#c7 -->
	<compilerflag compilerset="gcc">-fno-unit-at-a-time</compilerflag>
</module>
