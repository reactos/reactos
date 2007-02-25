<module name="powercfg" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_PWRCFG}" installbase="system32" installname="powercfg.cpl" allowwarnings="true">
	<importlibrary definition="powercfg.def" />
	<include base="powercfg">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>powrprof</library>
	<library>comctl32</library>
	<library>msvcrt</library>
	<file>powercfg.c</file>
	<file>powershemes.c</file>
	<file>alarms.c</file>
	<file>advanced.c</file>
	<file>hibernate.c</file>
	<file>powercfg.rc</file>
</module>
