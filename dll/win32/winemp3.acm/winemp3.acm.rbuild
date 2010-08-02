<module name="winemp3.acm" type="win32dll" installbase="system32" installname="winemp3.acm" allowwarnings="true" entrypoint="0" crt="MSVCRT">
	<importlibrary definition="winemp3.acm.spec" />
	<include base="winemp3.acm">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="ReactOS">include/reactos/libs/libmpg123</include>
	<define name="__WINESRC__" />
	<define name="WIN32" />
	<file>mpegl3.c</file>
	<library>wine</library>
	<library>winmm</library>
	<library>user32</library>
	<library>libmpg123</library>
	<library>ntdll</library>
</module>
