<module name="odbccp32i" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_ODBCCP32}" installbase="system32" installname="odbccp32.cpl" >
	<importlibrary definition="odbccp32.def" />
	<include base="odbccp32">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>msvcrt</library>
	<file>odbccp32.c</file>
	<file>odbccp32.rc</file>
</module>
