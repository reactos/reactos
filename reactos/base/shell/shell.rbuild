<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
  <group>
  <directory name="cmd">
    <xi:include href="cmd/cmd.rbuild" />
  </directory>
  <if property="ARCH" value="x86">
    <directory name="explorer">
      <xi:include href="explorer/explorer.rbuild" />
    </directory>
  </if>
  </group>
</rbuild>
