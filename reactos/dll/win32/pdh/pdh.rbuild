<module name="pdh" type="win32dll" baseaddress="${BASEADDRESS_PDH}" installbase="system32" installname="pdh.dll" entrypoint="0">
        <importlibrary definition="pdh.spec" />
        <include base="pdh">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <define name="_WIN32_WINNT">0x600</define>
        <library>wine</library>
        <library>kernel32</library>
        <library>ntdll</library>
        <file>pdh_main.c</file>
</module>
