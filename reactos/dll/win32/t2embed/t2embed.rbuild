<module name="t2embed" type="win32dll" baseaddress="${BASEADDRESS_T2EMBED}" installbase="system32" installname="t2embed.dll" unicode="yes">
	<importlibrary definition="t2embed.spec" />
	<include base="t2embed">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library>kernel32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>version</library>
	<library>wine</library>
	<file>t2embed.c</file>
	<file>t2embed.rc</file>
</module>