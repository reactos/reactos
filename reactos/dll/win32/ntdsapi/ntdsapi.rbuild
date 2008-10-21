<module name="ntdsapi" type="win32dll" baseaddress="${BASEADDRESS_NTDSAPI}" installbase="system32" installname="ntdsapi.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="ntdsapi.spec" />
        <include base="ntdsapi">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <define name="__USE_W32API" />
        <define name="_WIN32_IE">0x600</define>
        <define name="_WIN32_WINNT">0x501</define>
        <define name="WINVER">0x501</define>
        <library>wine</library>
        <library>user32</library>
        <library>kernel32</library>
        <library>ntdll</library>
        <file>ntdsapi.c</file>
        <file>ntdsapi.spec</file>
</module>
