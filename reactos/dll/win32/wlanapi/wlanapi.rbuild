<module name="wlanapi" type="win32dll" baseaddress="${BASEADDRESS_WLANAPI}" installbase="system32" installname="wlanapi.dll" entrypoint="0">
	<importlibrary definition="wlanapi.spec" />
	<include base="wlanapi">.</include>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>main.c</file>
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38054#c7 -->
	<compilerflag>-fno-unit-at-a-time</compilerflag>
</module>
