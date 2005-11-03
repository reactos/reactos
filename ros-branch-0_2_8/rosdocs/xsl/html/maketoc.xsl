<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
		version="1.0"
                exclude-result-prefixes="doc">

<xsl:import href="docbook.xsl"/>
<xsl:import href="chunk.xsl"/>

<xsl:output method="xml" indent="no" encoding='utf-8'/>

<xsl:param name="toc.list.type" select="'tocentry'"/>

<xsl:template name="subtoc">
  <xsl:param name="nodes" select="NOT-AN-ELEMENT"/>
  <xsl:variable name="filename">
    <xsl:apply-templates select="." mode="chunk-filename"/>
  </xsl:variable>

  <xsl:variable name="chunk">
    <xsl:call-template name="chunk"/>
  </xsl:variable>

  <xsl:if test="$chunk != 0">
    <xsl:call-template name="indent-spaces"/>
    <tocentry linkend="{@id}">
      <xsl:processing-instruction name="dbhtml">
        <xsl:text>filename="</xsl:text>
        <xsl:value-of select="$filename"/>
        <xsl:text>"</xsl:text>
      </xsl:processing-instruction>
      <xsl:text>&#xA;</xsl:text>
      <xsl:apply-templates mode="toc" select="$nodes"/>
      <xsl:call-template name="indent-spaces"/>
    </tocentry>
    <xsl:text>&#xA;</xsl:text>
  </xsl:if>
</xsl:template>

<xsl:template name="indent-spaces">
  <xsl:param name="node" select="."/>
  <xsl:text>  </xsl:text>
  <xsl:if test="$node/parent::*">
    <xsl:call-template name="indent-spaces">
      <xsl:with-param name="node" select="$node/parent::*"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="/" priority="-1">
  <xsl:text>&#xA;</xsl:text>
  <toc role="chunk-toc">
    <xsl:text>&#xA;</xsl:text>
    <xsl:apply-templates select="/" mode="toc"/>
  </toc>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

</xsl:stylesheet>
