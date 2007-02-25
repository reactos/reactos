<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
  <module name="winefile" type="win32gui" installbase="system32" installname="winefile.exe" allowwarnings="true">
    <include base="winefile">.</include>
    <define name="UNICODE" />
    <define name="__USE_W32API" />
    <define name="_WIN32_IE">0x0501</define>
    <define name="_WIN32_WINNT">0x0501</define>
    <library>uuid</library>
    <library>kernel32</library>
    <library>gdi32</library>
    <library>user32</library>
    <library>comctl32</library>
    <library>advapi32</library>
    <library>comdlg32</library>
    <library>shell32</library>
    <library>ole32</library>
    <library>version</library>
    <library>mpr</library>
    <file>winefile.c</file>
    <file>winefile.rc</file>
  </module>
</rbuild>
