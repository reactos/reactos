<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="diskpart" type="win32cui" installbase="system32" installname="diskpart.exe" unicode="true" >
	<include base="diskpart">.</include>
	<!-- library>ntdll</library -->
	<library>user32</library>
	<file>active.c</file>
	<file>add.c</file>
	<file>assign.c</file>
	<file>attach.c</file>
	<file>attributes.c</file>
	<file>automount.c</file>
	<file>break.c</file>
	<file>clean.c</file>
	<file>compact.c</file>
	<file>convert.c</file>
	<file>create.c</file>
	<file>delete.c</file>
	<file>detach.c</file>
	<file>detail.c</file>
	<file>diskpart.c</file>
	<file>expand.c</file>
	<file>extend.c</file>
	<file>filesystems.c</file>
	<file>format.c</file>
	<file>gpt.c</file>
	<file>help.c</file>
	<file>import.c</file>
	<file>inactive.c</file>
	<file>interpreter.c</file>
	<file>list.c</file>
	<file>merge.c</file>
	<file>offline.c</file>
	<file>online.c</file>
	<file>recover.c</file>
	<file>remove.c</file>
	<file>repair.c</file>
	<file>rescan.c</file>
	<file>retain.c</file>
	<file>san.c</file>
	<file>select.c</file>
	<file>setid.c</file>
	<file>shrink.c</file>
	<file>uniqueid.c</file>
	<file>diskpart.rc</file>
</module>