<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [

<!ENTITY lowercase "'abcdefghijklmnopqrstuvwxyz'">
<!ENTITY uppercase "'ABCDEFGHIJKLMNOPQRSTUVWXYZ'">

<!ENTITY primary   'concat(primary/@sortas, primary[not(@sortas)])'>
<!ENTITY secondary 'concat(secondary/@sortas, secondary[not(@sortas)])'>
<!ENTITY tertiary  'concat(tertiary/@sortas, tertiary[not(@sortas)])'>

<!ENTITY section   '(ancestor-or-self::set
                     |ancestor-or-self::book
                     |ancestor-or-self::part
                     |ancestor-or-self::reference
                     |ancestor-or-self::partintro
                     |ancestor-or-self::chapter
                     |ancestor-or-self::appendix
                     |ancestor-or-self::preface
                     |ancestor-or-self::section
                     |ancestor-or-self::sect1
                     |ancestor-or-self::sect2
                     |ancestor-or-self::sect3
                     |ancestor-or-self::sect4
                     |ancestor-or-self::sect5
                     |ancestor-or-self::refsect1
                     |ancestor-or-self::refsect2
                     |ancestor-or-self::refsect3
                     |ancestor-or-self::simplesect
                     |ancestor-or-self::bibliography
                     |ancestor-or-self::glossary
                     |ancestor-or-self::index)[last()]'>

<!ENTITY section.id 'generate-id(&section;)'>
<!ENTITY sep '" "'>
]>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version="1.0">

<!-- ==================================================================== -->
<!-- Derived from Jeni Tennison's work in the HTML case -->

<xsl:key name="letter"
         match="indexterm"
         use="translate(substring(&primary;, 1, 1),&lowercase;,&uppercase;)"/>

<xsl:key name="primary"
         match="indexterm"
         use="&primary;"/>

<xsl:key name="secondary"
         match="indexterm"
         use="concat(&primary;, &sep;, &secondary;)"/>

<xsl:key name="tertiary"
         match="indexterm"
         use="concat(&primary;, &sep;, &secondary;, &sep;, &tertiary;)"/>

<xsl:key name="primary-section"
         match="indexterm[not(secondary) and not(see)]"
         use="concat(&primary;, &sep;, &section.id;)"/>

<xsl:key name="secondary-section"
         match="indexterm[not(tertiary) and not(see)]"
         use="concat(&primary;, &sep;, &secondary;, &sep;, &section.id;)"/>

<xsl:key name="tertiary-section"
         match="indexterm[not(see)]"
         use="concat(&primary;, &sep;, &secondary;, &sep;, &tertiary;, &sep;, &section.id;)"/>

<xsl:key name="see-also"
         match="indexterm[seealso]"
         use="concat(&primary;, &sep;, &secondary;, &sep;, &tertiary;, &sep;, seealso)"/>

<xsl:key name="see"
         match="indexterm[see]"
         use="concat(&primary;, &sep;, &secondary;, &sep;, &tertiary;, &sep;, see)"/>

<xsl:key name="sections" match="*[@id]" use="@id"/>

<xsl:template name="generate-index">
  <xsl:variable name="terms" select="//indexterm[count(.|key('letter',
                                     translate(substring(&primary;, 1, 1),&lowercase;,&uppercase;))[1]) = 1]"/>
  <xsl:variable name="alphabetical"
                select="$terms[contains(concat(&lowercase;, &uppercase;),
                                        substring(&primary;, 1, 1))]"/>
  <xsl:variable name="others" select="$terms[not(contains(concat(&lowercase;,
                                                 &uppercase;),
                                             substring(&primary;, 1, 1)))]"/>
  <fo:block>
    <xsl:if test="$others">
      <fo:block font-size="16pt"
                font-weight="bold"
                keep-with-next.within-column="always"
                space-before="1em">
        <xsl:call-template name="gentext">
          <xsl:with-param name="key" select="'index symbols'"/>
        </xsl:call-template>
      </fo:block>
      <fo:block>
        <xsl:apply-templates select="$others[count(.|key('primary',
                                     &primary;)[1]) = 1]"
                             mode="index-primary">
          <xsl:sort select="&primary;"/>
        </xsl:apply-templates>
      </fo:block>
    </xsl:if>
    <xsl:apply-templates select="$alphabetical[count(.|key('letter',
                                 translate(substring(&primary;, 1, 1),&lowercase;,&uppercase;))[1]) = 1]"
                         mode="index-div">
      <xsl:sort select="&primary;"/>
    </xsl:apply-templates>
  </fo:block>
</xsl:template>

<xsl:template match="indexterm" mode="index-div">
  <xsl:variable name="key" select="translate(substring(&primary;, 1, 1),&lowercase;,&uppercase;)"/>
  <fo:block>
    <!-- this isn't quite exactly right. ideally all the symbols would -->
    <!-- be grouped together. as it stands, they all get separate divs -->
    <!-- but at least this test makes sure that they don't all get     -->
    <!-- separate titles as well. -->
    <xsl:if test="contains(concat(&lowercase;, &uppercase;), $key)">
      <fo:block font-size="16pt"
                font-weight="bold"
                keep-with-next.within-column="always"
                space-before="1em">
        <xsl:value-of select="translate($key, &lowercase;, &uppercase;)"/>
      </fo:block>
    </xsl:if>
    <fo:block>
      <xsl:apply-templates select="key('letter', $key)[count(.|key('primary', &primary;)[1]) = 1]"
                           mode="index-primary">
        <xsl:sort select="&primary;"/>
      </xsl:apply-templates>
    </fo:block>
  </fo:block>
</xsl:template>

<xsl:template match="indexterm" mode="index-primary">
  <xsl:variable name="key" select="&primary;"/>
  <xsl:variable name="refs" select="key('primary', $key)"/>
  <fo:block>
    <xsl:value-of select="primary"/>

    <xsl:variable name="page-number-citations">
      <xsl:for-each select="$refs[generate-id() = generate-id(key('primary-section', concat($key, &sep;, &section.id;))[1])]">
        <xsl:apply-templates select="." mode="reference"/>
      </xsl:for-each>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$passivetex.extensions != '0'">
        <fotex:sort xmlns:fotex="http://www.tug.org/fotex">
          <xsl:copy-of select="$page-number-citations"/>
        </fotex:sort>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="$page-number-citations"/>
      </xsl:otherwise>
    </xsl:choose>
  </fo:block>
  <xsl:if test="$refs/secondary or $refs[not(secondary)]/*[self::see or self::seealso]">
    <fo:block start-indent="1pc">
      <fo:block>
        <xsl:apply-templates select="$refs[generate-id() = generate-id(key('see', concat(&primary;, &sep;, &sep;, &sep;, see))[1])]"
                             mode="index-see">
          <xsl:sort select="see"/>
        </xsl:apply-templates>
        <xsl:apply-templates select="$refs[generate-id() = generate-id(key('see-also', concat(&primary;, &sep;, &sep;, &sep;, seealso))[1])]"
                             mode="index-seealso">
          <xsl:sort select="seealso"/>
        </xsl:apply-templates>
        <xsl:apply-templates select="$refs[secondary and count(.|key('secondary', concat($key, &sep;, &secondary;))[1]) = 1]" 
                             mode="index-secondary">
          <xsl:sort select="&secondary;"/>
        </xsl:apply-templates>
      </fo:block>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="indexterm" mode="index-secondary">
  <xsl:variable name="key" select="concat(&primary;, &sep;, &secondary;)"/>
  <xsl:variable name="refs" select="key('secondary', $key)"/>
  <fo:block>
    <xsl:value-of select="secondary"/>

    <xsl:variable name="page-number-citations">
      <xsl:for-each select="$refs[generate-id() = generate-id(key('secondary-section', concat($key, &sep;, &section.id;))[1])]">
        <xsl:apply-templates select="." mode="reference"/>
      </xsl:for-each>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$passivetex.extensions != '0'">
        <fotex:sort xmlns:fotex="http://www.tug.org/fotex">
          <xsl:copy-of select="$page-number-citations"/>
        </fotex:sort>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="$page-number-citations"/>
      </xsl:otherwise>
    </xsl:choose>
  </fo:block>
  <xsl:if test="$refs/tertiary or $refs[not(tertiary)]/*[self::see or self::seealso]">
    <fo:block start-indent="2pc">
      <fo:block>
        <xsl:apply-templates select="$refs[generate-id() = generate-id(key('see', concat(&primary;, &sep;, &secondary;, &sep;, &sep;, see))[1])]"
                             mode="index-see">
          <xsl:sort select="see"/>
        </xsl:apply-templates>
        <xsl:apply-templates select="$refs[generate-id() = generate-id(key('see-also', concat(&primary;, &sep;, &secondary;, &sep;, &sep;, seealso))[1])]"
                             mode="index-seealso">
          <xsl:sort select="seealso"/>
        </xsl:apply-templates>
        <xsl:apply-templates select="$refs[tertiary and count(.|key('tertiary', concat($key, &sep;, &tertiary;))[1]) = 1]" 
                             mode="index-tertiary">
          <xsl:sort select="&tertiary;"/>
        </xsl:apply-templates>
      </fo:block>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="indexterm" mode="index-tertiary">
  <xsl:variable name="key" select="concat(&primary;, &sep;, &secondary;, &sep;, &tertiary;)"/>
  <xsl:variable name="refs" select="key('tertiary', $key)"/>
  <fo:block>
    <xsl:value-of select="tertiary"/>

    <xsl:variable name="page-number-citations">
      <xsl:for-each select="$refs[generate-id() = generate-id(key('tertiary-section', concat($key, &sep;, &section.id;))[1])]">
        <xsl:apply-templates select="." mode="reference"/>
      </xsl:for-each>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$passivetex.extensions != '0'">
        <fotex:sort xmlns:fotex="http://www.tug.org/fotex">
          <xsl:copy-of select="$page-number-citations"/>
        </fotex:sort>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="$page-number-citations"/>
      </xsl:otherwise>
    </xsl:choose>
  </fo:block>
  <xsl:variable name="see" select="$refs/see | $refs/seealso"/>
  <xsl:if test="$see">
    <fo:block>
      <fo:block>
        <xsl:apply-templates select="$refs[generate-id() = generate-id(key('see', concat(&primary;, &sep;, &secondary;, &sep;, &tertiary;, &sep;, see))[1])]"
                             mode="index-see">
          <xsl:sort select="see"/>
        </xsl:apply-templates>
        <xsl:apply-templates select="$refs[generate-id() = generate-id(key('see-also', concat(&primary;, &sep;, &secondary;, &sep;, &tertiary;, &sep;, seealso))[1])]"
                             mode="index-seealso">
          <xsl:sort select="seealso"/>
        </xsl:apply-templates>
      </fo:block>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="indexterm" mode="reference">
  <xsl:if test="$passivetex.extensions = '0'">
    <xsl:text>, </xsl:text>
  </xsl:if>
  <xsl:choose>
    <xsl:when test="@zone and string(@zone)">
      <xsl:call-template name="reference">
        <xsl:with-param name="zones" select="normalize-space(@zone)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="id">
        <xsl:call-template name="object.id"/>
      </xsl:variable>
      <fo:basic-link internal-destination="{$id}">
        <fo:page-number-citation ref-id="{$id}"/>
      </fo:basic-link>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="reference">
  <xsl:param name="zones"/>
  <xsl:choose>
    <xsl:when test="contains($zones, ' ')">
      <xsl:variable name="zone" select="substring-before($zones, ' ')"/>
      <xsl:variable name="target" select="key('sections', $zone)"/>

      <xsl:variable name="id">
        <xsl:call-template name="object.id">
          <xsl:with-param name="object" select="$target[1]"/>
        </xsl:call-template>
      </xsl:variable>

      <fo:basic-link internal-destination="{$id}">
        <fo:page-number-citation ref-id="{$id}"/>
      </fo:basic-link>

      <xsl:if test="$passivetex.extensions = '0'">
        <xsl:text>, </xsl:text>
      </xsl:if>
      <xsl:call-template name="reference">
        <xsl:with-param name="zones" select="substring-after($zones, ' ')"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="zone" select="$zones"/>
      <xsl:variable name="target" select="key('sections', $zone)"/>

      <xsl:variable name="id">
        <xsl:call-template name="object.id">
          <xsl:with-param name="object" select="$target[1]"/>
        </xsl:call-template>
      </xsl:variable>

      <fo:basic-link internal-destination="{$id}">
        <fo:page-number-citation ref-id="{$id}"/>
      </fo:basic-link>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="indexterm" mode="index-see">
   <fo:block><xsl:value-of select="see"/></fo:block>
</xsl:template>

<xsl:template match="indexterm" mode="index-seealso">
   <fo:block><xsl:value-of select="seealso"/></fo:block>
</xsl:template>

</xsl:stylesheet>
