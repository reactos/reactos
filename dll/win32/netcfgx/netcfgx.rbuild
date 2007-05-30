<module name="netcfgx" type="win32dll" entrypoint="0" installbase="system32" installname="netcfgx.dll">
	<importlibrary definition="netcfgx.def" />
	<define name="__REACTOS__" />
	<file>netcfgx.c</file>
	<library>ntdll</library>
	<library>rpcrt4</library>
	<library>setupapi</library>
	<library>kernel32</library>
	<library>advapi32</library>
</module>
