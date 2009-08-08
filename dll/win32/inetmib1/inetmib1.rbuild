<module name="inetmib1" type="win32dll" baseaddress="${BASEADDRESS_INETMIB1}" installbase="system32" installname="inetmib1.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="inetmib1.spec.def" />
        <include base="inetmib1">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <define name="__USE_W32API" />
        <define name="_WIN32_IE">0x600</define>
        <define name="_WIN32_WINNT">0x501</define>
        <define name="WINVER">0x501</define>
        <library>wine</library>
        <library>snmpapi</library>
        <library>kernel32</library>
        <library>iphlpapi</library>
        <library>ntdll</library>
        <file>main.c</file>
        <file>inetmib1.spec</file>
</module>
