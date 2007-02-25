<module name="intl_new" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_INTL}" installbase="system32" installname="intl_new.cpl" usewrc="false">
	<importlibrary definition="intl.def" />
	<include base="intl_new">.</include>
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
	<library>shell32</library>
	<library>msvcrt</library>
	<file>intl.c</file>
	<file>locale.c</file>
	<file>extra.c</file>
	<file>setupreg.c</file>
	<file>langs.c</file>
	<file>date.c</file>
	<file>time.c</file>
	<file>nums.c</file>
	<file>currency.c</file>
	<file>intl.rc</file>
</module>
