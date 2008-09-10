<module name="fusion" type="win32dll" baseaddress="${BASEADDRESS_FUSION}" installbase="system32" installname="fusion.dll" allowwarnings="true">
	<importlibrary definition="fusion.spec.def" />
	<include base="fusion">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x601</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>kernel32</library>
	<library>version</library>
	<library>shlwapi</library>
	<library>advapi32</library>
	<library>dbghelp</library>
	<file>asmcache.c</file>
	<file>asmname.c</file>
	<file>assembly.c</file>
	<file>fusion.c</file>
	<file>fusion_main.c</file>
	<file>fusion.spec</file>
</module>
