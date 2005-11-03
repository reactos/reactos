<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/html/chunk.xsl"/>

<xsl:include href="olink-common.xsl"/>

<xsl:output method="xml"
            indent="yes"
            doctype-public="-//Norman Walsh//DTD DocBook OLink Summary V1.2//EN"
            doctype-system="http://docbook.sourceforge.net/release/xsl/current/tools/olink/olinksum.dtd"/>

<xsl:template name="olink.href.target">
  <xsl:param name="nd" select="."/>

  <xsl:call-template name="href.target">
    <xsl:with-param name="object" select="$nd"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>
