<module name="mscoree" type="win32dll" baseaddress="${BASEADDRESS_MSCOREE}" installbase="system32" installname="mscoree.dll">
	<importlibrary definition="mscoree.spec" />
	<include base="mscoree">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>dbghelp</library>
	<library>advapi32</library>
	<library>shell32</library>
	<library>ole32</library>
	<library>shlwapi</library>
	<library>uuid</library>
	<file>assembly.c</file>
	<file>config.c</file>
	<file>cordebug.c</file>
	<file>corruntimehost.c</file>
	<file>metadata.c</file>
	<file>metahost.c</file>
	<file>mscoree_main.c</file>
	<file>mscoree.rc</file>
</module>
