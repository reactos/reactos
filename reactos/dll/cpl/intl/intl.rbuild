<module name="intl" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_INTL}" installbase="system32" installname="intl.cpl">
	<importlibrary definition="intl.def" />
	<include base="intl">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>advapi32</library>
	<library>setupapi</library>
	<library>msvcrt</library>
	<file>currency.c</file>
	<file>date.c</file>
	<file>generalp.c</file>
	<file>intl.c</file>
	<file>inplocale.c</file>
	<file>numbers.c</file>
	<file>time.c</file>
	<file>intl.rc</file>
</module>
