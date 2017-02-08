<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ntlmssp" type="win32dll" baseaddress="${BASEADDRESS_NTLMSSP}" installbase="system32" installname="ntlmssp.dll" allowwarnings="true">
	<importlibrary definition="ntlmssp.spec" />
	<include base="ntlmssp">.</include>
	<library>advapi32</library>
	<library>crypt32</library>
	<library>netapi32</library>
	<library>ntdll</library>
	<library>wine</library>	
	<file>avl.c</file>
	<file>calculations.c</file>
	<file>ciphers.c</file>	
	<file>context.c</file>
	<file>credentials.c</file>
	<file>crypt.c</file>
	<file>debug.c</file>
	<file>dllmain.c</file>
	<file>messages.c</file>
	<file>ntlmssp.c</file>
	<file>protocol.c</file>
	<file>stubs.c</file>
	<file>sign.c</file>
	<file>util.c</file>
</module>


