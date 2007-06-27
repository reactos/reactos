<module name="olepro32" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_OLEPRO32}" installbase="system32" installname="olepro32.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="olepro32.spec.def" />
	<include base="olepro32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>oleaut32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>olepro32stubs.c</file>
	<file>version.rc</file>
	<file>olepro32.spec</file>
</module>
