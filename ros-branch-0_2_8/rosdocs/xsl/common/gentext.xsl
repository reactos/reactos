<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
                exclude-result-prefixes="doc"
                version='1.0'>

<!-- ============================================================ -->

<xsl:template match="*" mode="object.title.template">
  <xsl:call-template name="gentext.template">
    <xsl:with-param name="context" select="'title'"/>
    <xsl:with-param name="name" select="local-name(.)"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="chapter" mode="object.title.template">
  <xsl:choose>
    <xsl:when test="$chapter.autolabel != 0">
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'title-numbered'"/>
        <xsl:with-param name="name" select="local-name(.)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'title-unnumbered'"/>
        <xsl:with-param name="name" select="local-name(.)"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="appendix" mode="object.title.template">
  <xsl:choose>
    <xsl:when test="$appendix.autolabel != 0">
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'title-numbered'"/>
        <xsl:with-param name="name" select="local-name(.)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'title-unnumbered'"/>
        <xsl:with-param name="name" select="local-name(.)"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="section|sect1|sect2|sect3|sect4|sect5|simplesect
                     |bridgehead"
              mode="object.title.template">
  <xsl:choose>
    <xsl:when test="$section.autolabel != 0">
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'title-numbered'"/>
        <xsl:with-param name="name" select="local-name(.)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'title-unnumbered'"/>
        <xsl:with-param name="name" select="local-name(.)"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="procedure" mode="object.title.template">
  <xsl:choose>
    <xsl:when test="$formal.procedures != 0">
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'title'"/>
        <xsl:with-param name="name" select="'procedure.formal'"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'title'"/>
        <xsl:with-param name="name" select="local-name(.)"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="article/appendix"
              mode="object.title.template">
  <!-- FIXME: HACK HACK HACK! -->
  <xsl:text>%n. %t</xsl:text>
</xsl:template>

<!-- ============================================================ -->

<xsl:template match="*" mode="object.subtitle.template">
  <xsl:call-template name="gentext.template">
    <xsl:with-param name="context" select="'subtitle'"/>
    <xsl:with-param name="name" select="local-name(.)"/>
  </xsl:call-template>
</xsl:template>

<!-- ============================================================ -->

<xsl:template match="*" mode="object.xref.template">
  <xsl:call-template name="gentext.template">
    <xsl:with-param name="context" select="'xref'"/>
    <xsl:with-param name="name" select="local-name(.)"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="section|simplesect
                     |sect1|sect2|sect3|sect4|sect5
                     |refsect1|refsect2|refsect3
                     |bridgehead"
              mode="object.xref.template">
  <xsl:choose>
    <xsl:when test="$section.autolabel != 0">
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'section-xref-numbered'"/>
        <xsl:with-param name="name" select="local-name(.)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext.template">
        <xsl:with-param name="context" select="'section-xref'"/>
        <xsl:with-param name="name" select="local-name(.)"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ============================================================ -->

<xsl:template match="*" mode="object.title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:variable name="template">
    <xsl:apply-templates select="." mode="object.title.template"/>
  </xsl:variable>

<!--
  <xsl:message>
    <xsl:text>object.title.markup: </xsl:text>
    <xsl:value-of select="local-name(.)"/>
    <xsl:text>: </xsl:text>
    <xsl:value-of select="$template"/>
  </xsl:message>
-->

  <xsl:call-template name="substitute-markup">
    <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
    <xsl:with-param name="template" select="$template"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="*" mode="object.title.markup.textonly">
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="object.title.markup"/>
  </xsl:variable>
  <xsl:value-of select="$title"/>
</xsl:template>

<!-- ============================================================ -->

<xsl:template match="*" mode="object.subtitle.markup">
  <xsl:variable name="template">
    <xsl:apply-templates select="." mode="object.subtitle.template"/>
  </xsl:variable>

  <xsl:call-template name="substitute-markup">
    <xsl:with-param name="template" select="$template"/>
  </xsl:call-template>
</xsl:template>

<!-- ============================================================ -->

<xsl:template match="*" mode="object.xref.markup">
  <xsl:variable name="template">
    <xsl:apply-templates select="." mode="object.xref.template"/>
  </xsl:variable>

<!--
  <xsl:message>
    <xsl:text>object.xref.markup: </xsl:text>
    <xsl:value-of select="local-name(.)"/>
    <xsl:text>: [</xsl:text>
    <xsl:value-of select="$template"/>
    <xsl:text>]</xsl:text>
  </xsl:message>
-->

  <xsl:call-template name="substitute-markup">
    <xsl:with-param name="template" select="$template"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="listitem" mode="object.xref.markup">
  <xsl:choose>
    <xsl:when test="parent::orderedlist">
      <xsl:variable name="template">
        <xsl:apply-templates select="." mode="object.xref.template"/>
      </xsl:variable>
      <xsl:call-template name="substitute-markup">
        <xsl:with-param name="template" select="$template"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message>
        <xsl:text>Xref is only supported to listitems in an</xsl:text>
        <xsl:text> orderedlist: </xsl:text>
        <xsl:value-of select="@id"/>
      </xsl:message>
      <xsl:text>???</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ============================================================ -->

<xsl:template name="substitute-markup">
  <xsl:param name="template" select="''"/>
  <xsl:param name="allow-anchors" select="'0'"/>
  <xsl:param name="title" select="''"/>
  <xsl:param name="subtitle" select="''"/>
  <xsl:param name="label" select="''"/>
  <xsl:param name="pagenumber" select="''"/>

  <xsl:choose>
    <xsl:when test="contains($template, '%')">
      <xsl:value-of select="substring-before($template, '%')"/>
      <xsl:variable name="candidate"
             select="substring(substring-after($template, '%'), 1, 1)"/>
      <xsl:choose>
        <xsl:when test="$candidate = 't'">
          <xsl:choose>
            <xsl:when test="$title != ''">
              <xsl:value-of select="$title"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="." mode="title.markup">
                <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
              </xsl:apply-templates>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="$candidate = 's'">
          <xsl:choose>
            <xsl:when test="$subtitle != ''">
              <xsl:value-of select="$subtitle"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="." mode="subtitle.markup">
                <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
              </xsl:apply-templates>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="$candidate = 'n'">
          <xsl:choose>
            <xsl:when test="$label != ''">
              <xsl:value-of select="$label"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="." mode="label.markup"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="$candidate = 'p' ">
          <xsl:choose>
            <xsl:when test="$pagenumber != ''">
              <xsl:value-of select="$pagenumber"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="." mode="pagenumber.markup"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="$candidate = '%' ">
          <xsl:text>%</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>%</xsl:text><xsl:value-of select="$candidate"/>
        </xsl:otherwise>
      </xsl:choose>
      <!-- recurse with the rest of the template string -->
      <xsl:variable name="rest"
            select="substring($template,
            string-length(substring-before($template, '%'))+3)"/>
      <xsl:call-template name="substitute-markup">
        <xsl:with-param name="template" select="$rest"/>
        <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
        <xsl:with-param name="title" select="$title"/>
        <xsl:with-param name="subtitle" select="$subtitle"/>
        <xsl:with-param name="label" select="$label"/>
        <xsl:with-param name="pagenumber" select="$label"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$template"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ============================================================ -->

</xsl:stylesheet>

