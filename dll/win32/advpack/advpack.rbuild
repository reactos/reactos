<module name="advpack" type="win32dll" baseaddress="${BASEADDRESS_ADVPACK}" installbase="system32" installname="advpack.dll" allowwarnings="true">
	<importlibrary definition="advpack.spec.def" />
	<include base="advpack">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>ole32</library>
	<library>setupapi</library>
	<library>version</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>advpack.c</file>
	<file>files.c</file>
	<file>install.c</file>
	<file>reg.c</file>
	<file>advpack.spec</file>
</module>
