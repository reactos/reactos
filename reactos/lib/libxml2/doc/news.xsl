<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:xhtml="http://www.w3.org/1999/xhtml"
                version="1.0">
  <xsl:output method="text" encoding="ISO-8859-1"/>

  <xsl:template match="/">
    <xsl:text>
        NEWS file for libxml2

  Note that this is automatically generated from the news webpage at:
       http://xmlsoft.org/news.html

</xsl:text>
    <xsl:apply-templates select="//xhtml:h3[1]/.."/>
  </xsl:template>
  <xsl:template match="xhtml:h3">
    <xsl:text>
</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>:
</xsl:text>
  </xsl:template>
  <xsl:template match="xhtml:ul">
    <xsl:apply-templates select=".//xhtml:li"/>
    <xsl:text>
</xsl:text>
  </xsl:template>
  <xsl:template match="xhtml:li">
    <xsl:text>   - </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>
</xsl:text>
  </xsl:template>
  <xsl:template match="xhtml:a">
    <xsl:value-of select="."/>
    <xsl:text> at 
</xsl:text>
    <xsl:value-of select="@href"/>
    <xsl:text>
</xsl:text>
  </xsl:template>
</xsl:stylesheet>

