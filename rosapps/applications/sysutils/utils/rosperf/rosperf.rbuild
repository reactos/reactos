<module name="rosperf" type="win32cui" installbase="system32" installname="rosperf.exe">
	<include base="rosperf">.</include>
	<define name="UNICODE" />
	<library>version</library>
	<library>msimg32</library>
	<library>gdi32</library>
	<library>shell32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>rosperf.c</file>
	<file>lines.c</file>
	<file>fill.c</file>
	<file>text.c</file>
	<file>alphablend.c</file>
	<file>testlist.c</file>
	<file>gradient.c</file>
	<file>rosperf.rc</file>
</module>
