<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
  <cdfile base="reactos">services</cdfile>
  <installfile base="system32/drivers/etc">services</installfile>
  <if property="KDBG" value="1">
	  <cdfile base="reactos">KDBinit</cdfile>
	  <installfile base="system32/drivers/etc">KDBinit</installfile>
  </if>
</rbuild>
