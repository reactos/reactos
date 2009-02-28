<group>
<module name="activeds" type="win32dll" baseaddress="${BASEADDRESS_ACTIVEDS}" installbase="system32" installname="activeds.dll">
	<importlibrary definition="activeds.spec" />
	<include base="activeds">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>kernel32</library>
	<library>wine</library>
	<file>activeds_main.c</file>
</module>
</group>
