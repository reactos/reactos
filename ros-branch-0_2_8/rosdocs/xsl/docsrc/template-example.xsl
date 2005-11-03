<?xml version="1.0" encoding="utf-8"?><xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!-- This stylesheet was created by titlepage.xsl; do not edit it by hand. -->

<xsl:import href="/path/to/html/docbook.xsl"/>

<xsl:template name="article.titlepage.recto"><xsl:apply-templates mode="article.titlepage.recto.mode" select="(articleinfo/title|artheader/title|title)[1]|articleinfo/author|artheader/author|articleinfo/edition|artheader/edition"/>
</xsl:template>

<xsl:template name="article.titlepage">
  <div class="titlepage">
    <xsl:call-template name="article.titlepage.before.recto"/>
    <xsl:call-template name="article.titlepage.recto"/>
    <xsl:call-template name="article.titlepage.before.verso"/>
    <xsl:call-template name="article.titlepage.verso"/>
    <xsl:call-template name="article.titlepage.separator"/>
  </div>
</xsl:template>

</xsl:stylesheet>