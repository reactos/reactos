<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ntlmssp" type="win32dll" baseaddress="${BASEADDRESS_SCHANNEL}" installbase="system32" installname="ntlmssp.dll" allowwarnings="true">
	<importlibrary definition="ntlmssp.spec" />
	<include base="ntlmssp">.</include>
	<library>wine</library>
	<library>advapi32</library>
	<library>ntdll</library>
	<file>base64_codec.c</file>
	<file>context.c</file>
	<file>credentials.c</file>
	<file>hmac_md5.c</file>
	<file>messages.c</file>
	<file>ntlm.c</file>
	<file>sign.c</file>
	<file>util.c</file>
	<file>dllmain.c</file>
</module>


