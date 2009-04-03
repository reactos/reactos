<module name="wlanapi" type="win32dll" baseaddress="${BASEADDRESS_WLANAPI}" installbase="system32" installname="wlanapi.dll" entrypoint="0">
	<importlibrary definition="wlanapi.spec" />
	<include base="wlanapi">.</include>
	<include base="wlansvc_client">.</include>
	<library>wlansvc_client</library>
	<library>wine</library>
	<library>kernel32</library>
	<library>rpcrt4</library>
	<library>pseh</library>
	<library>ntdll</library>
	<file>main.c</file>
	<!-- See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38054#c7 -->
	<compilerflag>-fno-unit-at-a-time</compilerflag>
</module>
