<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
  <module name="tcpsvcs" type="win32cui" installbase="system32" installname="tcpsvcs.exe">
    <include base="arp">.</include>
    <define name="__USE_W32API" />
    <library>kernel32</library>
    <library>iphlpapi</library>
    <library>ws2_32</library>
    <library>shlwapi</library>
    <library>advapi32</library>
    <library>user32</library>
    <file>tcpsvcs.c</file>
    <file>skelserver.c</file>
    <file>echo.c</file>
    <file>discard.c</file>
    <file>daytime.c</file>
    <file>qotd.c</file>
    <file>chargen.c</file>
    <file>tcpsvcs.rc</file>
    <pch>tcpsvcs.h</pch>
  </module>
</rbuild>
