<module name="mscoree" type="win32dll" baseaddress="${BASEADDRESS_MSCOREE}" installbase="system32" installname="mscoree.dll">
	<importlibrary definition="mscoree.spec" />
	<include base="mscoree">.</include>
	<library>wine</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>uuid</library>
	<file>corruntimehost.c</file>
	<file>mscoree_main.c</file>
</module>
