<module name="ntmarta" type="win32dll" baseaddress="${BASEADDRESS_NTMARTA}" installbase="system32" installname="ntmarta.dll">
	<importlibrary definition="ntmarta.def" />
	<include base="ntmarta">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x600</define>
	<define name="WINVER">0x0600</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<file>ntmarta.c</file>
	<file>ntmarta.rc</file>
	<pch>ntmarta.h</pch>
</module>
