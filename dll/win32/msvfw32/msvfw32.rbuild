<module name="msvfw32" type="win32dll" baseaddress="${BASEADDRESS_MSVFW32}" installbase="system32" installname="msvfw32.dll" allowwarnings="true">
	<importlibrary definition="msvfw32.spec.def" />
	<include base="msvfw32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>winmm</library>
	<library>comctl32</library>
	<library>version</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>mciwnd.c</file>
	<file>msvideo_main.c</file>
	<file>drawdib.c</file>
	<file>rsrc.rc</file>
	<file>msvfw32.spec</file>
</module>
