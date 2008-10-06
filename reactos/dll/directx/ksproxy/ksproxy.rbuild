<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="ksproxy" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_KSPROXY}" installbase="system32" installname="ksproxy.ax" allowwarnings="true">
	<importlibrary definition="ksproxy.spec.def" />
	<include base="ksproxy">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>ksproxy.c</file>
	<file>ksproxy.rc</file>
	<file>ksproxy.spec</file>
</module>
</group>
