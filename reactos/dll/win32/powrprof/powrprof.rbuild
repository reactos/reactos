<module name="powrprof" type="win32dll" baseaddress="${BASEADDRESS_POWRPROF}" installbase="system32" installname="powrprof.dll" allowwarnings="true">
	<importlibrary definition="powrprof.spec.def" />
	<include base="powrprof">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>powrprof.c</file>
	<file>powrprof.spec</file>
</module>
