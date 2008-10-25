<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="mstask" type="win32dll" baseaddress="${BASEADDRESS_MSTASK}" installbase="system32" installname="mstask.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="mstask.spec" />
	<include base="mstask">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>factory.c</file>
	<file>mstask_main.c</file>
	<file>task.c</file>
	<file>task_scheduler.c</file>
	<file>task_trigger.c</file>
	<file>mstask_local.idl</file>
	<file>rsrc.rc</file>
	<file>mstask.spec</file>
	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
