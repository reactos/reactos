<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'>

<!-- ********************************************************************
     $Id: block.xsl,v 1.1 2002/06/13 20:32:34 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:template name="block.object">
  <div class="{name(.)}">
    <xsl:call-template name="anchor"/>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="para">
  <xsl:variable name="p">
    <p>
      <xsl:if test="position() = 1 and parent::listitem">
        <xsl:call-template name="anchor">
          <xsl:with-param name="node" select="parent::listitem"/>
        </xsl:call-template>
      </xsl:if>

      <xsl:call-template name="anchor"/>
      <xsl:apply-templates/>
    </p>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$html.cleanup != 0">
      <xsl:call-template name="unwrap.p">
        <xsl:with-param name="p" select="$p"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:copy-of select="$p"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="simpara">
  <!-- see also listitem/simpara in lists.xsl -->
  <p>
    <xsl:call-template name="anchor"/>
    <xsl:apply-templates/>
  </p>
</xsl:template>

<xsl:template match="formalpara">
  <p>
    <xsl:call-template name="anchor"/>
    <xsl:apply-templates/>
  </p>
</xsl:template>

<xsl:template match="formalpara/title">
  <xsl:variable name="titleStr" select="."/>
  <xsl:variable name="lastChar">
    <xsl:if test="$titleStr != ''">
      <xsl:value-of select="substring($titleStr,string-length($titleStr),1)"/>
    </xsl:if>
  </xsl:variable>

  <b>
    <xsl:apply-templates/>
    <xsl:if test="$lastChar != ''
                  and not(contains($runinhead.title.end.punct, $lastChar))">
      <xsl:value-of select="$runinhead.default.title.end.punct"/>
    </xsl:if>
    <xsl:text>&#160;</xsl:text>
  </b>
</xsl:template>

<xsl:template match="formalpara/para">
  <xsl:apply-templates/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="blockquote">
  <xsl:call-template name="anchor"/>
  <xsl:choose>
    <xsl:when test="attribution">
      <table border="0" width="100%"
	     cellspacing="0" cellpadding="0" class="blockquote"
             summary="Block quote">
	<tr>
	  <td width="10%" valign="top">&#160;</td>
	  <td width="80%" valign="top">
	    <xsl:apply-templates
	      select="child::*[local-name(.)!='attribution']"/>
	  </td>
	  <td width="10%" valign="top">&#160;</td>
	</tr>
	<tr>
	  <td colspan="2" align="right" valign="top">
	    <xsl:text>--</xsl:text>
	    <xsl:apply-templates select="attribution"/>
	  </td>
	  <td width="10%" valign="top">&#160;</td>
	</tr>
      </table>
    </xsl:when>
    <xsl:otherwise>
      <blockquote class="blockquote">
	<xsl:apply-templates/>
      </blockquote>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="blockquote/title">
  <div class="blockquote-title">
    <p>
      <b>
        <xsl:apply-templates/>
      </b>
    </p>
  </div>
</xsl:template>

<xsl:template match="epigraph">
  <div class="{name(.)}">
    <xsl:apply-templates select="para"/>
    <span>--<xsl:apply-templates select="attribution"/></span>
  </div>
</xsl:template>

<xsl:template match="attribution">
  <span class="{name(.)}"><xsl:apply-templates/></span>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="sidebar">
  <div class="{name(.)}">
    <xsl:call-template name="anchor"/>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="sidebar/title">
  <p class="title">
    <b><xsl:apply-templates/></b>
  </p>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="abstract">
  <div class="{name(.)}">
    <xsl:call-template name="anchor"/>
    <xsl:call-template name="formal.object.heading">
      <xsl:with-param name="title">
        <xsl:apply-templates select="." mode="title.markup">
          <xsl:with-param name="allow-anchors" select="'1'"/>
        </xsl:apply-templates>
      </xsl:with-param>
    </xsl:call-template>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="abstract/title">
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="msgset">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="msgentry">
  <xsl:call-template name="block.object"/>
</xsl:template>

<xsl:template match="simplemsgentry">
  <xsl:call-template name="block.object"/>
</xsl:template>

<xsl:template match="msg">
  <xsl:call-template name="block.object"/>
</xsl:template>

<xsl:template match="msgmain">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="msgmain/title">
  <b><xsl:apply-templates/></b>
</xsl:template>

<xsl:template match="msgsub">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="msgsub/title">
  <b><xsl:apply-templates/></b>
</xsl:template>

<xsl:template match="msgrel">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="msgrel/title">
  <b><xsl:apply-templates/></b>
</xsl:template>

<xsl:template match="msgtext">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="msginfo">
  <xsl:call-template name="block.object"/>
</xsl:template>

<xsl:template match="msglevel">
  <p>
    <b>
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'msgset'"/>
        <xsl:with-param name="name" select="'MsgLevel'"/>
      </xsl:call-template>
    </b>
    <xsl:apply-templates/>
  </p>
</xsl:template>

<xsl:template match="msgorig">
  <p>
    <b>
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'msgset'"/>
        <xsl:with-param name="name" select="'MsgOrig'"/>
      </xsl:call-template>
    </b>
    <xsl:apply-templates/>
  </p>
</xsl:template>

<xsl:template match="msgaud">
  <p>
    <b>
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'msgset'"/>
        <xsl:with-param name="name" select="'MsgAud'"/>
      </xsl:call-template>
    </b>
    <xsl:apply-templates/>
  </p>
</xsl:template>

<xsl:template match="msgexplan">
  <xsl:call-template name="block.object"/>
</xsl:template>

<xsl:template match="msgexplan/title">
  <p><b><xsl:apply-templates/></b></p>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="revhistory">
  <div class="{name(.)}">
    <table border="0" width="100%" summary="Revision history">
      <tr>
        <th align="left" valign="top" colspan="3">
          <b>
            <xsl:call-template name="gentext">
              <xsl:with-param name="key" select="'RevHistory'"/>
            </xsl:call-template>
          </b>
        </th>
      </tr>
      <xsl:apply-templates/>
    </table>
  </div>
</xsl:template>

<xsl:template match="revhistory/revision">
  <xsl:variable name="revnumber" select=".//revnumber"/>
  <xsl:variable name="revdate"   select=".//date"/>
  <xsl:variable name="revauthor" select=".//authorinitials"/>
  <xsl:variable name="revremark" select=".//revremark|../revdescription"/>
  <tr>
    <td align="left">
      <xsl:if test="$revnumber">
        <xsl:call-template name="gentext">
          <xsl:with-param name="key" select="'Revision'"/>
        </xsl:call-template>
        <xsl:call-template name="gentext.space"/>
        <xsl:apply-templates select="$revnumber"/>
      </xsl:if>
    </td>
    <td align="left">
      <xsl:apply-templates select="$revdate"/>
    </td>
    <xsl:choose>
      <xsl:when test="count($revauthor)=0">
        <td align="left">
          <xsl:call-template name="dingbat">
            <xsl:with-param name="dingbat">nbsp</xsl:with-param>
          </xsl:call-template>
        </td>
      </xsl:when>
      <xsl:otherwise>
        <td align="left">
          <xsl:apply-templates select="$revauthor"/>
        </td>
      </xsl:otherwise>
    </xsl:choose>
  </tr>
  <xsl:if test="$revremark">
    <tr>
      <td align="left" colspan="3">
        <xsl:apply-templates select="$revremark"/>
      </td>
    </tr>
  </xsl:if>
</xsl:template>

<xsl:template match="revision/revnumber">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="revision/date">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="revision/authorinitials">
  <xsl:text>, </xsl:text>
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="revision/authorinitials[1]" priority="2">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="revision/revremark">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="revision/revdescription">
  <xsl:apply-templates/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="ackno">
  <p class="{name(.)}">
    <xsl:apply-templates/>
  </p>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="highlights">
  <xsl:call-template name="block.object"/>
</xsl:template>

<!-- ==================================================================== -->

</xsl:stylesheet>
