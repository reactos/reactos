<module name="winemp3.acm" type="win32dll" installbase="system32" installname="winemp3.acm" allowwarnings="true" entrypoint="0">
	<importlibrary definition="winemp3.acm.spec" />
	<include base="winemp3.acm">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>common.c</file>
	<file>dct64_i386.c</file>
	<file>decode_i386.c</file>
	<file>interface.c</file>
	<file>layer1.c</file>
	<file>layer2.c</file>
	<file>layer3.c</file>
	<file>mpegl3.c</file>
	<file>tabinit.c</file>
	<library>wine</library>
	<library>winmm</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
