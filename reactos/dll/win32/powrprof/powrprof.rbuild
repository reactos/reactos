<module name="powrprof" type="win32dll" baseaddress="${BASEADDRESS_POWRPROF}" installbase="system32" installname="powrprof.dll" allowwarnings="true">
	<importlibrary definition="powrprof_gcc.def" />
	<include base="ReactOS">include/wine</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>ntdll</library>
	<library>rtl</library>
	<library>kernel32</library>
	<library>ntoskrnl</library>
	<file>powrprof.c</file>
	<file>powrprof.rc</file>
</module>
