<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="netcfgx" type="win32dll" installbase="system32" installname="netcfgx.dll">
	<importlibrary definition="netcfgx.def" />
	<define name="__REACTOS__" />
	<file>netcfgx.c</file>
	<library>ntdll</library>
	<library>rpcrt4</library>
	<library>setupapi</library>
</module>
</rbuild>
