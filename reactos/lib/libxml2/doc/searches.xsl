<?xml version="1.0"?>
<!-- this stylesheet builds the API*.html , it works based on libxml2-refs.xml
  -->
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:exsl="http://exslt.org/common"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="exsl">

  <!-- Import the rest of the site stylesheets -->
  <xsl:import href="site.xsl"/>

  <!-- Generate XHTML-1.0 transitional -->
  <xsl:output method="xml" encoding="ISO-8859-1" indent="yes"
      doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
      doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>

  <xsl:variable name="href_base" select="''"/>

  <xsl:template name="statistics">
      <h2> weekly statistics: </h2>
      <p><xsl:value-of select="@total"/> total words,
      <xsl:value-of select="@uniq"/> uniq words.</p>
      <p> Top <xsl:value-of select="@nr"/> queries:</p>
  </xsl:template>

  <xsl:template match="query">
     <br/><a href="search.php?query={string(.)}"><xsl:value-of
          select="string(.)"/></a>
	  <xsl:text> </xsl:text><xsl:value-of select="@count"/> times.
  </xsl:template>

  <xsl:template match="queries">
    <xsl:variable name="date" select="@date"/>
    <xsl:variable name="title">Search statistics for <xsl:value-of select="$date"/></xsl:variable>
    <xsl:document href="searches.html" method="xml" encoding="ISO-8859-1"
      doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
      doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
      <html>
        <head>
        <xsl:call-template name="style"/>
	<xsl:element name="title">
	  <xsl:value-of select="$title"/>
	</xsl:element>
        </head>
        <body bgcolor="#8b7765" text="#000000" link="#a06060" vlink="#000000">
          <xsl:call-template name="titlebox">
	    <xsl:with-param name="title" select="$title"/>
	  </xsl:call-template>
          <table border="0" cellpadding="4" cellspacing="0" width="100%" align="center">
            <tr>
              <td bgcolor="#8b7765">
                <table border="0" cellspacing="0" cellpadding="2" width="100%">
                  <tr>
                    <td valign="top" width="200" bgcolor="#8b7765">
                      <xsl:call-template name="toc"/>
                    </td>
                    <td valign="top" bgcolor="#8b7765">
                      <table border="0" cellspacing="0" cellpadding="1" width="100%">
                        <tr>
                          <td>
                            <table border="0" cellspacing="0" cellpadding="1" width="100%" bgcolor="#000000">
                              <tr>
                                <td>
                                  <table border="0" cellpadding="3" cellspacing="1" width="100%">
                                    <tr>
                                      <td bgcolor="#fffacd">
				        <xsl:call-template name="statistics"/>
					<p>
				        <xsl:apply-templates select="query"/>
					</p>
					<p><a href="{$href_base}bugs.html">Daniel Veillard</a></p>
                                      </td>
                                    </tr>
                                  </table>
                                </td>
                              </tr>
                            </table>
                          </td>
                        </tr>
                      </table>
                    </td>
                  </tr>
                </table>
              </td>
            </tr>
          </table>
        </body>
      </html>
    </xsl:document>
  </xsl:template>

  <xsl:template match="/">
    <xsl:apply-templates select="queries"/>
  </xsl:template>

</xsl:stylesheet>
