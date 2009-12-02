<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="ksproxy" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_KSPROXY}" installbase="system32" installname="ksproxy.ax">
	<importlibrary definition="ksproxy.spec" />
	<include base="ksproxy">.</include>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>ksproxy.c</file>
	<file>ksproxy.rc</file>
</module>
</group>
