<module name="mlang" type="win32dll" baseaddress="${BASEADDRESS_MLANG}" installbase="system32" installname="mlang.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="mlang.spec.def" />
	<include base="mlang">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>oleaut32</library>
	<library>uuid</library>
	<file>mlang.c</file>
	<file>regsvr.c</file>
	<file>mlang.spec</file>
</module>
