<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version="1.0">

<!-- ==================================================================== -->

<xsl:template name="setup.pagemasters">
  <fo:layout-master-set>
    <!-- one sided, single column -->
    <fo:simple-page-master master-name="blank"
                           page-width="{$page.width}"
                           page-height="{$page.height}"
                           margin-top="{$page.margin.top}"
                           margin-bottom="{$page.margin.bottom}"
                           margin-left="{$page.margin.inner}"
                           margin-right="{$page.margin.outer}">
      <fo:region-body
                      margin-bottom="{$body.margin.bottom}"
                      margin-top="{$body.margin.top}"/>
      <fo:region-before region-name="xsl-region-before-blank"
                        extent="{$region.before.extent}"
                        display-align="after"/>
      <fo:region-after region-name="xsl-region-after-blank"
                       extent="{$region.after.extent}"
                        display-align="after"/>
    </fo:simple-page-master>

    <!-- one sided, single column -->
    <fo:simple-page-master master-name="simple1"
                           page-width="{$page.width}"
                           page-height="{$page.height}"
                           margin-top="{$page.margin.top}"
                           margin-bottom="{$page.margin.bottom}"
                           margin-left="{$page.margin.inner}"
                           margin-right="{$page.margin.outer}">
      <fo:region-body
                      margin-bottom="{$body.margin.bottom}"
                      margin-top="{$body.margin.top}"/>
      <fo:region-before extent="{$region.before.extent}"
                        display-align="after"/>
      <fo:region-after extent="{$region.after.extent}"
                        display-align="after"/>
    </fo:simple-page-master>

    <!-- one sided, single column, draft mode -->
    <fo:simple-page-master master-name="draft1"
                           page-width="{$page.width}"
                           page-height="{$page.height}"
                           margin-top="{$page.margin.top}"
                           margin-bottom="{$page.margin.bottom}"
                           margin-left="{$page.margin.inner}"
                           margin-right="{$page.margin.outer}">
      <fo:region-body margin-bottom="{$body.margin.bottom}"
                      margin-top="{$body.margin.top}">
        <xsl:if test="$draft.watermark.image != ''
                      and $fop.extensions = 0">
          <xsl:attribute name="background-image">
            <xsl:value-of select="$draft.watermark.image"/>
          </xsl:attribute>
          <xsl:attribute name="background-attachment">fixed</xsl:attribute>
          <xsl:attribute name="background-repeat">no-repeat</xsl:attribute>
          <xsl:attribute name="background-position-horizontal">center</xsl:attribute>
          <xsl:attribute name="background-position-vertical">center</xsl:attribute>
        </xsl:if>
      </fo:region-body>
      <fo:region-before extent="{$region.before.extent}"
                        display-align="after"/>
      <fo:region-after extent="{$region.after.extent}"
                        display-align="after"/>
    </fo:simple-page-master>

    <!-- for left-hand/even pages in twosided mode, single column -->
    <fo:simple-page-master master-name="left1"
                           page-width="{$page.width}"
                           page-height="{$page.height}"
                           margin-top="{$page.margin.top}"
                           margin-bottom="{$page.margin.bottom}"
                           margin-left="{$page.margin.outer}"
                           margin-right="{$page.margin.inner}">
      <fo:region-body
                      margin-bottom="{$body.margin.bottom}"
                      margin-top="{$body.margin.top}"/>
      <fo:region-before region-name="xsl-region-before-left"
                        extent="{$region.before.extent}"
                        display-align="after"/>
      <fo:region-after region-name="xsl-region-after-left"
                       extent="{$region.after.extent}"
                        display-align="after"/>
    </fo:simple-page-master>

    <!-- for right-hand/odd pages in twosided mode, single column -->
    <fo:simple-page-master master-name="right1"
                           page-width="{$page.width}"
                           page-height="{$page.height}"
                           margin-top="{$page.margin.top}"
                           margin-bottom="{$page.margin.bottom}"
                           margin-left="{$page.margin.inner}"
                           margin-right="{$page.margin.outer}">
      <fo:region-body
                      margin-bottom="{$body.margin.bottom}"
                      margin-top="{$body.margin.top}"/>
      <fo:region-before region-name="xsl-region-before-right"
                        extent="{$region.before.extent}"
                        display-align="after"/>
      <fo:region-after region-name="xsl-region-after-right"
                       extent="{$region.after.extent}"
                        display-align="after"/>
    </fo:simple-page-master>

    <!-- special case of first page in either mode, single column -->
    <fo:simple-page-master master-name="first1"
                           page-width="{$page.width}"
                           page-height="{$page.height}"
                           margin-top="{$page.margin.top}"
                           margin-bottom="{$page.margin.bottom}"
                           margin-left="{$page.margin.inner}"
                           margin-right="{$page.margin.outer}">
      <fo:region-body
                      margin-bottom="{$body.margin.bottom}"
                      margin-top="{$body.margin.top}"/>
      <fo:region-before region-name="xsl-region-before-first"
                        extent="{$region.before.extent}"
                        display-align="after"/>
      <fo:region-after region-name="xsl-region-after-first"
                       extent="{$region.after.extent}"
                        display-align="after"/>
    </fo:simple-page-master>

    <!-- for pages in one-side mode, 2 column -->
    <fo:simple-page-master master-name="simple2"
                           page-width="{$page.width}"
                           page-height="{$page.height}"
                           margin-top="{$page.margin.top}"
                           margin-bottom="{$page.margin.bottom}"
                           margin-left="{$page.margin.inner}"
                           margin-right="{$page.margin.outer}">
      <fo:region-body margin-bottom="{$body.margin.bottom}"
                      margin-top="{$body.margin.top}">
        <xsl:attribute name="column-count">
          <!-- FIXME: how bad is this hack? If the overall column.count is 1,
               use 2 here. Otherwise, use whatever is specified. This allows
               some pages to be 2 column and others to be 1 column. Perhaps
               this should always be 2 and if you want a 3 column page, you should
               make a new pagemaster? Or maybe if you want to mix single and multi-
               column pages, you should write another pagemaster for the ones
               you want to be two column... -->
          <xsl:choose>
            <xsl:when test="$column.count &lt; 2">2</xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="$column.count"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
      </fo:region-body>

      <fo:region-before extent="{$region.before.extent}"
                        display-align="after"/>
      <fo:region-after extent="{$region.after.extent}"
                        display-align="after"/>
    </fo:simple-page-master>

    <!-- for left-hand/even pages in twosided mode, 2 column -->
    <fo:simple-page-master master-name="left2"
                           page-width="{$page.width}"
                           page-height="{$page.height}"
                           margin-top="{$page.margin.top}"
                           margin-bottom="{$page.margin.bottom}"
                           margin-left="{$page.margin.outer}"
                           margin-right="{$page.margin.inner}">
      <fo:region-body
                      column-count="{$column.count}"
                      margin-bottom="{$body.margin.bottom}"
                      margin-top="{$body.margin.top}"/>
      <fo:region-before region-name="xsl-region-before-left"
                        extent="{$region.before.extent}"
                        display-align="after"/>
      <fo:region-after region-name="xsl-region-after-left"
                       extent="{$region.after.extent}"
                        display-align="after"/>
    </fo:simple-page-master>

    <!-- for right-hand/odd pages in twosided mode, 2 column -->
    <fo:simple-page-master master-name="right2"
                           page-width="{$page.width}"
                           page-height="{$page.height}"
                           margin-top="{$page.margin.top}"
                           margin-bottom="{$page.margin.bottom}"
                           margin-left="{$page.margin.inner}"
                           margin-right="{$page.margin.outer}">
      <fo:region-body
                      column-count="{$column.count}"
                      margin-bottom="{$body.margin.bottom}"
                      margin-top="{$body.margin.top}"/>
      <fo:region-before region-name="xsl-region-before-right"
                        extent="{$region.before.extent}"
                        display-align="after"/>
      <fo:region-after region-name="xsl-region-after-right"
                       extent="{$region.after.extent}"
                        display-align="after"/>
    </fo:simple-page-master>

    <!-- special case of first page in either mode -->
    <fo:simple-page-master master-name="first2"
                           page-width="{$page.width}"
                           page-height="{$page.height}"
                           margin-top="{$page.margin.top}"
                           margin-bottom="{$page.margin.bottom}"
                           margin-left="{$page.margin.inner}"
                           margin-right="{$page.margin.outer}">
      <fo:region-body
                      column-count="1"
                      margin-bottom="{$body.margin.bottom}"
                      margin-top="{$body.margin.top}"/>
      <fo:region-before region-name="xsl-region-before-first"
                        extent="{$region.before.extent}"
                        display-align="after"/>
      <fo:region-after region-name="xsl-region-after-first"
                       extent="{$region.after.extent}"
                        display-align="after"/>
    </fo:simple-page-master>

    <!-- setup for title-page, 1 column -->
    <fo:page-sequence-master master-name="titlepage1">
      <fo:repeatable-page-master-alternatives>
        <fo:conditional-page-master-reference master-reference="first1"/>
      </fo:repeatable-page-master-alternatives>
    </fo:page-sequence-master>

    <!-- setup for single-sided, 1 column -->
    <fo:page-sequence-master master-name="oneside1">
      <fo:repeatable-page-master-alternatives>
        <fo:conditional-page-master-reference master-reference="simple1"/>
      </fo:repeatable-page-master-alternatives>
    </fo:page-sequence-master>

    <!-- setup for single-sided, 1 column -->
    <fo:page-sequence-master master-name="onesidedraft1">
      <fo:repeatable-page-master-alternatives>
        <fo:conditional-page-master-reference master-reference="draft1"/>
      </fo:repeatable-page-master-alternatives>
    </fo:page-sequence-master>

    <!-- setup for double-sided, 1 column -->
    <fo:page-sequence-master master-name="twoside1">
      <fo:repeatable-page-master-alternatives>
        <fo:conditional-page-master-reference master-reference="blank"
                                              blank-or-not-blank="blank"/>
        <fo:conditional-page-master-reference master-reference="right1"
                                              odd-or-even="odd"/>
        <fo:conditional-page-master-reference master-reference="left1"
                                              odd-or-even="even"/>
      </fo:repeatable-page-master-alternatives>
    </fo:page-sequence-master>

    <!-- setup for title-page, 2 column -->
    <fo:page-sequence-master master-name="titlepage2">
      <fo:repeatable-page-master-alternatives>
        <fo:conditional-page-master-reference master-reference="first2"/>
      </fo:repeatable-page-master-alternatives>
    </fo:page-sequence-master>

    <!-- setup for single-sided, 2 column -->
    <fo:page-sequence-master master-name="oneside2">
      <fo:repeatable-page-master-alternatives>
        <fo:conditional-page-master-reference master-reference="simple2"/>
      </fo:repeatable-page-master-alternatives>
    </fo:page-sequence-master>

    <!-- setup for double-sided, 2 column -->
    <fo:page-sequence-master master-name="twoside2">
      <fo:repeatable-page-master-alternatives>
        <fo:conditional-page-master-reference master-reference="blank"
                                              blank-or-not-blank="blank"/>
        <fo:conditional-page-master-reference master-reference="right2"
                                              odd-or-even="odd"/>
        <fo:conditional-page-master-reference master-reference="left2"
                                              odd-or-even="even"/>
      </fo:repeatable-page-master-alternatives>
    </fo:page-sequence-master>

    <xsl:call-template name="user.pagemasters"/>

    </fo:layout-master-set>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="user.pagemasters"/> <!-- intentionally empty -->

<!-- ==================================================================== -->

<!-- $double.sided, $column.count, and context -->

<xsl:template name="select.pagemaster">
  <xsl:param name="element" select="local-name(.)"/>
  <!-- column.count is a param so it can be selected dynamically; see index.xsl -->
  <xsl:param name="column.count" select="$column.count"/>

  <xsl:choose>
    <xsl:when test="$double.sided != 0">
      <xsl:choose>
        <xsl:when test="$column.count &gt; 1">
          <xsl:call-template name="select.doublesided.multicolumn.pagemaster">
            <xsl:with-param name="element" select="$element"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="select.doublesided.pagemaster">
            <xsl:with-param name="element" select="$element"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$column.count &gt; 1">
          <xsl:call-template name="select.singlesided.multicolumn.pagemaster">
            <xsl:with-param name="element" select="$element"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="select.singlesided.pagemaster">
            <xsl:with-param name="element" select="$element"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="select.doublesided.multicolumn.pagemaster">
  <xsl:param name="element" select="local-name(.)"/>
  <xsl:choose>
    <xsl:when test="$element='set' or $element='book' or $element='part'">
      <xsl:text>titlepage2</xsl:text>
    </xsl:when>
    <xsl:otherwise>twoside2</xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="select.doublesided.pagemaster">
  <xsl:param name="element" select="local-name(.)"/>
  <xsl:choose>
    <xsl:when test="$element='set' or $element='book' or $element='part'">
      <xsl:text>titlepage1</xsl:text>
    </xsl:when>
    <xsl:otherwise>twoside1</xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="select.singlesided.multicolumn.pagemaster">
  <xsl:param name="element" select="local-name(.)"/>
  <xsl:choose>
    <xsl:when test="$element='set' or $element='book' or $element='part'">
      <xsl:text>titlepage2</xsl:text>
    </xsl:when>
    <xsl:otherwise>oneside2</xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="select.singlesided.pagemaster">
  <xsl:param name="element" select="local-name(.)"/>
  <xsl:choose>
    <xsl:when test="ancestor-or-self::*[@status][1]/@status = 'draft'">
      <xsl:text>onesidedraft1</xsl:text>
    </xsl:when>
    <xsl:when test="$element='set' or $element='book' or $element='part'">
      <xsl:text>titlepage1</xsl:text>
    </xsl:when>
    <xsl:otherwise>oneside1</xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="*" mode="running.head.mode">
  <xsl:param name="master-reference" select="'unknown'"/>
  <!-- by default, nothing -->
  <xsl:choose>
    <xsl:when test="$master-reference='titlepage1'">
    </xsl:when>
    <xsl:when test="$master-reference='oneside1'">
    </xsl:when>
    <xsl:when test="$master-reference='twoside1'">
    </xsl:when>
    <xsl:when test="$master-reference='titlepage2'">
    </xsl:when>
    <xsl:when test="$master-reference='oneside2'">
    </xsl:when>
    <xsl:when test="$master-reference='twoside2'">
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="chapter|appendix" mode="running.head.mode">
  <xsl:param name="master-reference" select="'unknown'"/>
  <xsl:variable name="head">
    <fo:block font-size="{$body.font.size}">
      <xsl:apply-templates select="." mode="object.title.markup"/>
    </fo:block>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$master-reference='titlepage1'"></xsl:when>
    <xsl:when test="$master-reference='oneside1'">
      <fo:static-content flow-name="xsl-region-before">
        <fo:block text-align="center">
          <xsl:copy-of select="$head"/>
        </fo:block>
      </fo:static-content>
    </xsl:when>
    <xsl:when test="$master-reference='twoside1'">
      <fo:static-content flow-name="xsl-region-before-left">
        <fo:block text-align="right">
          <xsl:copy-of select="$head"/>
        </fo:block>
      </fo:static-content>
      <fo:static-content flow-name="xsl-region-before-right">
        <fo:block text-align="left">
          <xsl:copy-of select="$head"/>
        </fo:block>
      </fo:static-content>
    </xsl:when>
    <xsl:when test="$master-reference='titlepage2'"></xsl:when>
    <xsl:when test="$master-reference='oneside2'">
      <fo:static-content flow-name="xsl-region-before">
        <fo:block text-align="center">
          <xsl:copy-of select="$head"/>
        </fo:block>
      </fo:static-content>
    </xsl:when>
    <xsl:when test="$master-reference='twoside2'">
      <fo:static-content flow-name="xsl-region-before-left">
        <fo:block text-align="right">
          <xsl:copy-of select="$head"/>
        </fo:block>
      </fo:static-content>
      <fo:static-content flow-name="xsl-region-before-right">
        <fo:block text-align="left">
          <xsl:copy-of select="$head"/>
        </fo:block>
      </fo:static-content>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message>
        <xsl:text>Unexpected master-reference (</xsl:text>
        <xsl:value-of select="$master-reference"/>
        <xsl:text>) in running.head.mode for </xsl:text>
        <xsl:value-of select="name(.)"/>
        <xsl:text>. No header generated.</xsl:text>
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="*" mode="running.foot.mode">
  <xsl:param name="master-reference" select="'unknown'"/>
  <xsl:variable name="foot">
    <fo:page-number/>
  </xsl:variable>
  <!-- by default, the page number -->
  <xsl:choose>
    <xsl:when test="$master-reference='titlepage1'"></xsl:when>
    <xsl:when test="$master-reference='oneside1'">
      <fo:static-content flow-name="xsl-region-after">
        <fo:block text-align="center" font-size="{$body.font.size}">
          <xsl:copy-of select="$foot"/>
        </fo:block>
      </fo:static-content>
    </xsl:when>
    <xsl:when test="$master-reference='twoside1'">
      <fo:static-content flow-name="xsl-region-after-left">
        <fo:block text-align="left" font-size="{$body.font.size}">
          <xsl:copy-of select="$foot"/>
        </fo:block>
      </fo:static-content>
      <fo:static-content flow-name="xsl-region-after-right">
        <fo:block text-align="right" font-size="{$body.font.size}">
          <xsl:copy-of select="$foot"/>
        </fo:block>
      </fo:static-content>
    </xsl:when>
    <xsl:when test="$master-reference='titlepage2'"></xsl:when>
    <xsl:when test="$master-reference='oneside2'">
      <fo:static-content flow-name="xsl-after-before">
        <fo:block text-align="center" font-size="{$body.font.size}">
          <xsl:copy-of select="$foot"/>
        </fo:block>
      </fo:static-content>
    </xsl:when>
    <xsl:when test="$master-reference='twoside2'">
      <fo:static-content flow-name="xsl-region-after-left">
        <fo:block text-align="left" font-size="{$body.font.size}">
          <xsl:copy-of select="$foot"/>
        </fo:block>
      </fo:static-content>
      <fo:static-content flow-name="xsl-region-after-right">
        <fo:block text-align="right" font-size="{$body.font.size}">
          <xsl:copy-of select="$foot"/>
        </fo:block>
      </fo:static-content>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message>
        <xsl:text>Unexpected master-reference (</xsl:text>
        <xsl:value-of select="$master-reference"/>
        <xsl:text>) in running.foot.mode for </xsl:text>
        <xsl:value-of select="name(.)"/>
        <xsl:text>. No footer generated.</xsl:text>
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="set|book|part|reference" mode="running.foot.mode">
  <!-- nothing -->
</xsl:template>

<!-- ==================================================================== -->

</xsl:stylesheet>
