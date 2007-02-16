<module name="mpr" type="win32dll" baseaddress="${BASEADDRESS_MPR}" installbase="system32" installname="mpr.dll" allowwarnings="true">
	<importlibrary definition="mpr.spec.def" />
	<include base="mpr">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>auth.c</file>
	<file>mpr_main.c</file>
	<file>multinet.c</file>
	<file>nps.c</file>
	<file>pwcache.c</file>
	<file>wnet.c</file>
	<file>mpr.rc</file>
	<file>mpr.spec</file>
</module>
