<module name="cards" type="win32dll" baseaddress="${BASEADDRESS_CARDS}" installbase="system32" installname="cards.dll" allowwarnings="true">
	<importlibrary definition="cards.spec.def" />
	<include base="cards">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>cards.c</file>
	<library>ntdll</library>
	<file>cards.rc</file>
	<library>ntdll</library>
	<file>version.rc</file>
	<file>cards.spec</file>
</module>
