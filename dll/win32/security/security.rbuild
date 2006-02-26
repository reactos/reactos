<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="security" type="win32dll" baseaddress="${BASEADDRESS_SECUR32}" installbase="system32" installname="security.dll">
	<importlibrary definition="security.def" />
	<include base="security">.</include>
	<define name="__SECURITY__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x501</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<file>security.rc</file>
</module>
</rbuild>
