<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="smss2" type="nativecui" installbase="system32" installname="smss2.exe">
	<bootstrap installbase="$(CDOUTPUT)/system32" />
	<include base="smss2">.</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<library>nt</library>
	<library>pseh</library>
	<library>ntdll</library>
	<pch>smss.h</pch>
	<compilationunit name="unit.c">
		<file>smss.c</file>
		<file>crashdmp.c</file>
		<file>pagefile.c</file>
		<file>sminit.c</file>
		<file>smloop.c</file>
		<file>smsbapi.c</file>
		<file>smsessn.c</file>
		<file>smsubsys.c</file>
		<file>smutil.c</file>
	</compilationunit>
	<file>smss.rc</file>
</module>
