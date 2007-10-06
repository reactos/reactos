<module name="cryptui" type="win32dll" baseaddress="${BASEADDRESS_CRYPTUI}" installbase="system32" installname="cryptui.dll" allowwarnings="true">
	<importlibrary definition="cryptui.def" />
	<include base="cryptui">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>cryptui.c</file>
</module>
