<module name="cryptnet" type="win32dll" baseaddress="${BASEADDRESS_CRYPTNET}" installbase="system32" installname="cryptnet.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="cryptnet.spec.def" />
	<include base="cryptnet">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>cryptnet_main.c</file>
	<file>cryptnet.spec</file>
</module>
