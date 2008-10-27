<module name="icmp" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_ICMP}" installbase="system32" installname="icmp.dll" allowwarnings="true">
	<importlibrary definition="icmp.spec" />
	<include base="icmp">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library>wine</library>
	<library>kernel32</library>
	<library>ws2_32</library>
	<library>wine</library>
	<file>icmp_main.c</file>
	<file>icmp.rc</file>
	<file>icmp.spec</file>
</module>
