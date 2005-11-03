<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<!-- Include common profiling stylesheet -->
<xsl:include href="profile-mode.xsl"/>

<!-- Generate DocBook instance with correct DOCTYPE -->
<xsl:output method="xml" 
            doctype-public="-//OASIS//DTD DocBook XML V4.1.2//EN"
            doctype-system="http://www.oasis-open.org/docbook/xml/4.0/docbookx.dtd"/>

<!-- Profiling parameters -->
<xsl:param name="profile.arch" select="''"/>
<xsl:param name="profile.condition" select="''"/>
<xsl:param name="profile.conformance" select="''"/>
<xsl:param name="profile.lang" select="''"/>
<xsl:param name="profile.os" select="''"/>
<xsl:param name="profile.revision" select="''"/>
<xsl:param name="profile.revisionflag" select="''"/>
<xsl:param name="profile.role" select="''"/>
<xsl:param name="profile.security" select="''"/>
<xsl:param name="profile.userlevel" select="''"/>
<xsl:param name="profile.vendor" select="''"/>
<xsl:param name="profile.attribute" select="''"/>
<xsl:param name="profile.value" select="''"/>
<xsl:param name="profile.separator" select="';'"/>

<!-- Call common profiling mode -->
<xsl:template match="/">
  <xsl:apply-templates select="." mode="profile"/>
</xsl:template>

</xsl:stylesheet>

