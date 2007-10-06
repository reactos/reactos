<module name="usp10" type="win32dll" entrypoint="0"  installbase="system32" installname="usp10.dll" allowwarnings ="true">
	<importlibrary definition="usp10.spec.def" />
	<include base="usp10">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>usp10.c</file>
	<file>usp10.spec</file>
</module>
