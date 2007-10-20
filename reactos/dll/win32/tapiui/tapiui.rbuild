<module name="tapiui" type="win32dll" baseaddress="${BASEADDRESS_TAPIUI}" installbase="system32" installname="tapiui.dll" unicode="true">
	<include base="tapiui">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<file>tapiui.c</file>
	<file>tapiui.rc</file>
</module>
