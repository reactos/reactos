<module name="shlwapi" type="win32dll" baseaddress="${BASEADDRESS_SHLWAPI}" installbase="system32" installname="shlwapi.dll" allowwarnings="true">
	<importlibrary definition="shlwapi.spec.def" />
	<include base="shlwapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<define name="_SHLWAPI_"/>
	<define name="WINSHLWAPI">""</define>
	<linkerflag>-nostdlib</linkerflag>
	<linkerflag>-lgcc</linkerflag>
	<library>wine</library>
	<library>uuid</library>
	<library>msvcrt</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<file>assoc.c</file>
	<file>clist.c</file>
	<file>istream.c</file>
	<file>msgbox.c</file>
	<file>ordinal.c</file>
	<file>path.c</file>
	<file>reg.c</file>
	<file>regstream.c</file>
	<file>shlwapi_main.c</file>
	<file>stopwatch.c</file>
	<file>string.c</file>
	<file>thread.c</file>
	<file>url.c</file>
	<file>wsprintf.c</file>
	<file>shlwapi.rc</file>
	<file>shlwapi.spec</file>
</module>
