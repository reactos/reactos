<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:template match="/">
<html>
<head>
<title>ReactOS Component Version Report</title>
</head>
<body>
<h1>ReactOS Component Version Report</h1>
<table border="1">
  <tr bgcolor="#9acd32">
    <th align="left">Module Name</th>
    <th align="left">Module Location</th>
    <th align="left">Version</th>
    <th align="left">Date</th>
    <th align="left">Owner</th>
  </tr>
  <xsl:for-each select="components/component">
    <tr>
      <td><xsl:value-of select="name"/></td>
      <td><xsl:value-of select="base"/></td>
      <td><xsl:value-of select="version"/></td>
      <td><xsl:value-of select="date"/></td>
      <td><xsl:value-of select="owner"/></td>
    </tr>
    </xsl:for-each>
    </table>
  </body>
  </html>
</xsl:template>

</xsl:stylesheet>
