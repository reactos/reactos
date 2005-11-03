<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<xsl:template match="/">
  <xsl:message terminate="yes">WARNING:
Profiling stylesheet has moved to new location profiling/profile.xsl.
This new version uses different names of parameters. Please use e.g.
"profile.os" instead of simply "os". You can now also perform
profiling in a single step as an integral part of transformation. Check
new stylesheets profile-docbook.xsl and profile-chunk.xsl.
  </xsl:message>  
</xsl:template>

</xsl:stylesheet>

