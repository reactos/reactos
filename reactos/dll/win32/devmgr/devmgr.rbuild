<module name="devmgr" type="win32dll" baseaddress="${BASEADDRESS_DEVENUM}" installbase="system32" installname="devmgr.dll" allowwarnings="true">
	<include base="devmgr">.</include>
	<importlibrary definition="devmgr.spec.def" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<define name="_SETUPAPI_VER">0x501</define>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>setupapi</library>
	<library>advapi32</library>
	<library>user32</library>
	<file>devmgr.rc</file>
	<file>advprop.c</file>
	<file>devprblm.c</file>
	<file>hwpage.c</file>
	<file>misc.c</file>
	<file>stubs.c</file>
	<file>devmgr.spec</file>
	<pch>precomp.h</pch>
</module>