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
  <xsl:variable name="apirefs" select="document('libxml2-refs.xml')"/>
  <xsl:variable name="module" select="$apirefs/apirefs/@name"/>
  <xsl:key name="refhref" match="reference" use="@name"/>

  <xsl:template match="ref" mode="anchor">
    <xsl:variable name="name" select="@name"/>
    <xsl:for-each select="document('libxml2-refs.xml')">
	<a href="{key('refhref', $name)/@href}"><xsl:value-of select="$name"/></a><br/>
    </xsl:for-each>
  </xsl:template>
  <xsl:template match="type" mode="reflist">
    <h2>Type <xsl:value-of select="@name"/>:</h2>
    <p>
      <xsl:for-each select="ref">
        <xsl:apply-templates mode="anchor" select="."/>
	<xsl:text>
</xsl:text>
      </xsl:for-each>
    </p>
  </xsl:template>
  <xsl:template match="letter" mode="reflist">
    <h2>Letter <xsl:value-of select="@name"/>:</h2>
    <p>
      <xsl:for-each select="ref">
        <xsl:apply-templates mode="anchor" select="."/>
	<xsl:text>
</xsl:text>
      </xsl:for-each>
    </p>
  </xsl:template>
  <xsl:template match="file" mode="reflist">
    <h2><a name="{@name}">Module <xsl:value-of select="@name"/></a>:</h2>
    <p>
      <xsl:for-each select="ref">
        <xsl:apply-templates mode="anchor" select="."/>
	<xsl:text>
</xsl:text>
      </xsl:for-each>
    </p>
  </xsl:template>
  <xsl:template match="letter" mode="wordlist">
    <h2>Letter <xsl:value-of select="@name"/>:</h2>
    <dl>
      <xsl:for-each select="word">
        <dt><xsl:value-of select="@name"/></dt>
	<dd>
	<xsl:for-each select="ref">
	  <xsl:apply-templates mode="anchor" select="."/>
	  <xsl:text>
</xsl:text>
	</xsl:for-each>
	</dd>
      </xsl:for-each>
    </dl>
  </xsl:template>

  <xsl:template match="constructors">
    <xsl:message>Generating API Constructors</xsl:message>
    <xsl:variable name="title">List of constructors for <xsl:value-of select="$module"/></xsl:variable>
    <xsl:document href="APIconstructors.html" method="xml" encoding="ISO-8859-1"
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
                      <xsl:call-template name="develtoc"/>
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
				        <xsl:apply-templates mode="reflist" select="type"/>
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
  <xsl:template match="files">
    <xsl:message>Generating API List of synbols per file</xsl:message>
    <xsl:variable name="title">List of Symbols per Module for <xsl:value-of select="$module"/></xsl:variable>
    <xsl:document href="APIfiles.html" method="xml" encoding="ISO-8859-1"
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
                      <xsl:call-template name="develtoc"/>
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
				        <xsl:apply-templates mode="reflist" select="file"/>
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
  <xsl:template match="functions">
    <xsl:message>Generating API Functions by Type</xsl:message>
    <xsl:variable name="title">List of function manipulating types in <xsl:value-of select="$module"/></xsl:variable>
    <xsl:document href="APIfunctions.html" method="xml" encoding="ISO-8859-1"
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
                      <xsl:call-template name="develtoc"/>
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
				        <xsl:apply-templates mode="reflist" select="type"/>
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
  <xsl:template match="alpha">
    <xsl:message>Generating API Alphabetic list</xsl:message>
    <xsl:variable name="title">Alphabetic List of Symbols in <xsl:value-of select="$module"/></xsl:variable>
    <xsl:document href="APIsymbols.html" method="xml" encoding="ISO-8859-1"
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
                      <xsl:call-template name="develtoc"/>
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
				        <xsl:apply-templates mode="reflist" select="letter"/>
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
  <xsl:template name="apichunks">
    <h2 align="center">
    <xsl:for-each select="/apirefs/index/chunks/chunk">
      <xsl:variable name="name" select="@name"/>
      <xsl:variable name="start" select="@start"/>
      <xsl:variable name="end" select="@end"/>
      <xsl:variable name="block" select="concat($start, '-', $end)"/>
      <a href="API{$name}.html"><xsl:value-of select="$block"/></a>
      <xsl:text>
</xsl:text>
    </xsl:for-each>
    </h2>
  </xsl:template>
  <xsl:template match="chunk">
    <xsl:variable name="name" select="@name"/>
    <xsl:variable name="start" select="@start"/>
    <xsl:variable name="end" select="@end"/>
    <xsl:variable name="block" select="concat($start, '-', $end)"/>
    <xsl:variable name="target" select="/apirefs/index/chunk[@name = $name]"/>
    <xsl:variable name="title">API Alphabetic Index <xsl:value-of select="$block"/> for <xsl:value-of select="$module"/></xsl:variable>
    <xsl:document href="API{$name}.html" method="xml" encoding="ISO-8859-1"
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
                      <xsl:call-template name="develtoc"/>
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
				        <xsl:call-template name="apichunks"/>
				        <xsl:apply-templates mode="wordlist"
					           select="$target/letter"/>
				        <xsl:call-template name="apichunks"/>
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

  <xsl:template match="index">
    <xsl:message>Generating API Index</xsl:message>
    <xsl:apply-templates select="chunks/chunk"/>
  </xsl:template>

  <xsl:template match="apirefs">
    <xsl:message>Generating API Cross References</xsl:message>
    <xsl:apply-templates select="constructors"/>
    <xsl:apply-templates select="functions"/>
    <xsl:apply-templates select="alpha"/>
    <xsl:apply-templates select="files"/>
    <xsl:apply-templates select="index"/>
  </xsl:template>

  <xsl:template match="/">
    <xsl:apply-templates select="$apirefs/apirefs"/>
  </xsl:template>

</xsl:stylesheet>
