<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<xsl:import href="../html/docbook.xsl"/>
<xsl:include href="olink-common.xsl"/>

<xsl:output method="xml"
            indent="yes"
            doctype-public="-//Norman Walsh//DTD DocBook OLink Summary V1.2//EN"
            doctype-system="http://docbook.sourceforge.net/???"/>

<xsl:param name="base-uri" select="''"/>

<xsl:template name="olink.href.target">
  <xsl:param name="nd" select="."/>

  <xsl:value-of select="$base-uri"/>
  <xsl:call-template name="href.target">
    <xsl:with-param name="object" select="$nd"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>
