<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="rpcss" type="win32cui" installbase="system32" installname="rpcss.exe">
	<include base="rpcss">.</include>
	<include base="rpcss" root="intermediate">.</include>
	<library>wine</library>
	<library>rpcss_epm_server</library>
	<library>rpcss_irot_server</library>
	<library>rpcrt4</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<file>rpcss_main.c</file>
	<file>epmp.c</file>
	<file>irotp.c</file>
	<file>epm.idl</file>
	<file>irot.idl</file>
	<file>rpcss.rc</file>
</module>
<module name="rpcss_epm_server" type="rpcserver">
	<file>epm.idl</file>
</module>
<module name="rpcss_irot_server" type="rpcserver">
	<file>irot.idl</file>
</module>
