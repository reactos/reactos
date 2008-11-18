<?php # $Id: serendipity_event_textwiki.php 586 2005-10-23 22:38:34Z jmatos $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_EVENT_TEXTWIKI_NAME',     'Markup: Wiki');
@define('PLUGIN_EVENT_TEXTWIKI_DESC',     'Markup text using Text_Wiki');
@define('PLUGIN_EVENT_TEXTWIKI_TRANSFORM', '<a href="http://c2.com/cgi/wiki">Wiki</a> format allowed');

// Currently only english available

@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_PREFILETER', 'Converts different OS linebreaks (Unix/DOS) to unified format and concates lines ending with \. Default is on. Not recommended to switch off.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_DELIMITER', 'Converts the Text_Wiki internal delimiter "\xFF" (255) to avoid conflicts while parsing. Default is on. Not recommended to switch off.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_CODE', 'Marks text between <code> and </code> as code. Using <code type=".."> you can switch highlighting on (e.g. for PHP). Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_PHPCODE', 'Marks and highlights text between <php> and </php> as PHP code and adds PHP open tags. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_HTML', 'Allows you to use real HTML between <html> and </html>. Beware JS is possible, too! If you use this, switch off markup for comments! Default is off. Not recommended to switch on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_RAW', 'Text between `` and `` is not touched by other markup rules. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_INCLUDE', 'Allows you to include and run PHP code with the syntax [[include /path/to/script.php]]. Resulting output is parsed by markup rules. Beware, security risk! If you use this, switch off markup for comments! Default is off. Not recommended to switch on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_INCLUDE_DESC_BASE', 'The base directory to your scripts. Default for this is set to "/path/to/scripts/". If you leave this blank and switch include on you can only use absolute paths.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_HEADING', 'Lines starting with "+ " are marked as headlines (+ = <h1>, ++++++ = <h6>). Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_HORIZ', '---- is converted to a horizontal line (<hr>). Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_BREAK', 'Line endings marked with " _" define explicit linebreaks. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_BLOCKQUOTE', 'Enables to use email style quoting ("> ", ">> ",...). Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_LIST', 'Allows creation of lists ("* " = undefined, "# " = numbered). Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_DEFLIST', 'Enables to create definition lists. Syntax: ": Topic : Definition". Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_TABLE', 'Allows you to create tables. Only used for complete lines. Syntax: "|| Cell 1 || Cell 2 || Cell 3 ||". Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_EMBED', 'Allows you to include and run PHP code with the syntax [[embed /path/to/script.php]]. Resulting output is not parsed by markup rules. Beware, security risk! If you use this, switch off markup for comments! Default is off. Not recommended to switch on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_EMBED_DESC_BASE', 'The base directory to your scripts. Default for this is set to "/path/to/scripts/". If you leave this blank and switch embed on you can only use absolute paths.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_IMAGE', 'Enables the inclusion of images. ([[image  /path/to/image.ext [HTML attributes] ]] or [[image  path/to/image.ext [link="PageName"] [HTML attributes] ]] for linked images). Por omissão ligado.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_IMAGE_DESC_BASE', 'Base directory to your images. Default for this is set to "/path/to/images". If you leave this blank you can only use absolute paths or URLs.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_PHPLOOKUP', 'Creates lookup links to the PHP manual with [[php function-name]]. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_TOC', 'Generates a table of contents over all used headlines with [[toc]]. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_NEWLINE', 'Converts single newlines ("\n") to line breaks. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_CENTER', 'Lines starting with "= " are centered. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_PARAGRAPH', 'Double newlines are converted to paragraphs (<p></p>). Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_URL', 'Normal converts http://example.com to links, [http://example.com] to footnotes and [http://example.com Example] to descriptive links. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_URL_DESC_TARGET', 'Defines the target for your URLs. This is default set to "_blank", what is mostly feasible.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_FREELINK', 'Enables definition of non-standard wiki links using "((Non-standard link format))" and "((Non-standard link|Description))". Default is off.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_PAGES', 'The freelink rule (as well as the wikilink rule) must know, which pages exist and which have to be marked as "new". This specifies a file (local or remote) which has to contain 1 pagename per line. If the file is remote, it will be cached for the specified time.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_VIEWURL', 'This URL is specified to view the freelinks. You have to specify a "%s" inside this URL which will be replaced with the name of the freelink page.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_NEWURL', 'This URL is specified to create new freelinks. You have to specify a "%s" inside this URL which will be replaced with the name of the freelink page.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_NEWTEXT', 'This text will be added to undefined freelinks to link to the create page. Initially this is set to "?".');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_CACHETIME', 'If you specify a remote file (URL) for your freelink pages, this file will be cached for as many seconds you specify here. Default is 1 hour.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_INTERWIKI', 'Allows inter wiki linking to MeatBall, Advogato and Wiki using SiteName:PageName or [SiteName:PageName Show this text instead]. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_INTERWIKI_DESC_TARGET', '');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_WIKILINK', 'Enables usage of standard WikiWords (2-X x uppercase) as wiki links. Default is off.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_PAGES', 'The wikilink rule must know, which pages exist and which have to be marked as "new". This specifies a file (local or remote) which has to contain 1 pagename per line. If the file is remote, it will be cached for the specified time.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_VIEWURL', 'This URL is specified to view the wikilinks. You have to specify a "%s" inside this URL which will be replaced with the name of the wikilink page.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_NEWURL', 'This URL is specified to create new wikilinks. You have to specify a "%s" inside this URL which will be replaced with the name of the wikilink page.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_NEWTEXT', 'This text will be added to undefined wikilinks to link to the create page. Initially this is set to "?".');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_CACHETIME', 'If you specify a remote file (URL) for your wikilink pages, this file will be cached for as many seconds you specify here. Default is 1 hour.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_COLORTEXT', 'Colorize text using ##color|text##. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_STRONG', '**Text** is marked strong. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_BOLD', '\'\'\'Text\'\'\' is marked bold. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_EMPHASIS', '//Text// is marked emphasised. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_ITALIC', '\'\'Text\'\' is marked italic. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_TT', '{{Text}} is writen in teletext (monotype). Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_SUPERSCRIPT', '^^Text^^ is written in superscript. Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_REVISE', 'Enables marking texts as revisions using "@@---delete this text+++insert this text@@". Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_TIGHTEN', 'Finds more than 3 newlines and reduces them to 2 newlines (paragraph). Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_ENTITIES', 'Escapes HTML entities. Default is on.');

class serendipity_event_textwiki extends serendipity_event
{

    var $wikiRules  = array(
        'prefilter' => array(
            'file' => 'Text/Wiki/Rule/prefilter.php',
            'name' => 'Text_Wiki_Rule_prefilter',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_PREFILETER
        ),

        'delimiter' => array(
            'file' => 'Text/Wiki/Rule/delimiter.php',
            'name' => 'Text_Wiki_Rule_delimiter',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_DELIMITER
        ),

        'code' => array(
            'file' => 'Text/Wiki/Rule/code.php',
            'name' => 'Text_Wiki_Rule_code',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_CODE
        ),

        'phpcode' => array(
            'file' => 'Text/Wiki/Rule/phpcode.php',
            'name' => 'Text_Wiki_Rule_phpcode',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_PHPCODE
        ),

        'html' => array(
            'file' => 'Text/Wiki/Rule/html.php',
            'name' => 'Text_Wiki_Rule_html',
            'flag' => false,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_HTML
        ),

        'raw' => array(
            'file' => 'Text/Wiki/Rule/raw.php',
            'name' => 'Text_Wiki_Rule_raw',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_RAW
        ),

        'include' => array(
            'file' => 'Text/Wiki/Rule/include.php',
            'name' => 'Text_Wiki_Rule_include',
            'flag' => false,
            'conf' => array(
                'base' => '/path/to/scripts/'
            ),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_INCLUDE,
            's9yc' => array(
                 'base' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_INCLUDE_DESC_BASE))
        ),

        'heading' => array(
            'file' => 'Text/Wiki/Rule/heading.php',
            'name' => 'Text_Wiki_Rule_heading',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_HEADING
        ),

        'horiz' => array(
            'file' => 'Text/Wiki/Rule/horiz.php',
            'name' => 'Text_Wiki_Rule_horiz',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_HORIZ
        ),

        'break' => array(
            'file' => 'Text/Wiki/Rule/break.php',
            'name' => 'Text_Wiki_Rule_break',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_BREAK
        ),

        'blockquote' => array(
            'file' => 'Text/Wiki/Rule/blockquote.php',
            'name' => 'Text_Wiki_Rule_blockquote',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_BLOCKQUOTE
        ),

        'list' => array(
            'file' => 'Text/Wiki/Rule/list.php',
            'name' => 'Text_Wiki_Rule_list',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_LIST
        ),

        'deflist' => array(
            'file' => 'Text/Wiki/Rule/deflist.php',
            'name' => 'Text_Wiki_Rule_deflist',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_DEFLIST
        ),

        'table' => array(
            'file' => 'Text/Wiki/Rule/table.php',
            'name' => 'Text_Wiki_Rule_table',
            'flag' => true,
            'conf' => array(
                'border'  => 1,
                'spacing' => 0,
                'padding' => 4
            ),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_TABLE
        ),

        'embed' => array(
            'file' => 'Text/Wiki/Rule/embed.php',
            'name' => 'Text_Wiki_Rule_embed',
            'flag' => false,
            'conf' => array(
                'base' => '/path/to/scripts/'
            ),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_EMBED,
            's9yc' => array(
                 'base' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_EMBED_DESC_BASE))
        ),

        'image' => array(
            'file' => 'Text/Wiki/Rule/image.php',
            'name' => 'Text_Wiki_Rule_image',
            'flag' => true,
            'conf' => array(
                'base' => ''
            ),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_IMAGE,
            's9yc' => array(
                 'base' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_IMAGE_DESC_BASE))
        ),

        'phplookup' => array(
            'file' => 'Text/Wiki/Rule/phplookup.php',
            'name' => 'Text_Wiki_Rule_phplookup',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_PHPLOOKUP
        ),

        'toc' => array(
            'file' => 'Text/Wiki/Rule/toc.php',
            'name' => 'Text_Wiki_Rule_toc',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_TOC
        ),

        'newline' => array(
            'file' => 'Text/Wiki/Rule/newline.php',
            'name' => 'Text_Wiki_Rule_newline',
            'flag' => true,
            'conf' => array(
                'skip' => array(
                    'code',
                    'phpcode',
                    'heading',
                    'horiz',
                    'deflist',
                    'table',
                    'list',
                    'toc'
                )
            ),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_NEWLINE
        ),

        'center' => array(
            'file' => 'Text/Wiki/Rule/center.php',
            'name' => 'Text_Wiki_Rule_center',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_CENTER
        ),

        'paragraph' => array(
            'file' => 'Text/Wiki/Rule/paragraph.php',
            'name' => 'Text_Wiki_Rule_paragraph',
            'flag' => true,
            'conf' => array(
                'skip' => array(
                    'blockquote',
                    'code',
                    'phpcode',
                    'heading',
                    'horiz',
                    'deflist',
                    'table',
                    'list',
                    'toc'
                )
            ),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_PARAGRAPH
        ),

        'url' => array(
            'file' => 'Text/Wiki/Rule/url.php',
            'name' => 'Text_Wiki_Rule_url',
            'flag' => true,
            'conf' => array(
                'target' => '_BLANK'
            ),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_URL,
            's9yc' => array(
                 'target' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_URL_DESC_TARGET)),
        ),

        'freelink' => array(
            'file' => 'Text/Wiki/Rule/freelink.php',
            'name' => 'Text_Wiki_Rule_freelink',
            'flag' => false,
            'conf' => array(
                'pages'       => array(),
                'view_url' => 'http://example.com/index.php?page=%s',
                'new_url'  => 'http://example.com/new.php?page=%s',
                'new_text' => '?'
            ),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_FREELINK,
            's9yc' => array(
                 'pages' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_PAGES),
                 'view_url' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_VIEWURL),
                 'new_url' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_NEWURL),
                 'new_text' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_NEWTEXT),
                 'cachetime' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_CACHETIME))
        ),

        'interwiki' => array(
            'file' => 'Text/Wiki/Rule/interwiki.php',
            'name' => 'Text_Wiki_Rule_interwiki',
            'flag' => true,
            'conf' => array(
                'sites' => array(
                    'MeatBall' => 'http://www.usemod.com/cgi-bin/mb.pl?%s',
                    'Advogato' => 'http://advogato.org/%s',
                    'Wiki'     => 'http://c2.com/cgi/wiki?%s'
                ),
                'target' => '_BLANK'
            ),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_INTERWIKI,
            's9yc' => array(
                 'pages' => array(
                     'target' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_INTERWIKI_DESC_TARGET))
        ),

        'wikilink' => array(
            'file' => 'Text/Wiki/Rule/wikilink.php',
            'name' => 'Text_Wiki_Rule_wikilink',
            'flag' => false,
            'conf' => array(
                'pages'       => array(),
                'view_url' => 'http://example.com/index.php?page=%s',
                'new_url'  => 'http://example.com/new.php?page=%s',
                'new_text' => '?'
            ),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_WIKILINK,
            's9yc' => array(
                 'pages' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_PAGES),
                 'view_url' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_VIEWURL),
                 'new_url' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_NEWURL),
                 'new_text' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_NEWTEXT),
                 'cachetime' => array(
                     'type' => 'string',
                     'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_CACHETIME))
        ),

        'colortext' => array(
            'file' => 'Text/Wiki/Rule/colortext.php',
            'name' => 'Text_Wiki_Rule_colortext',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_COLORTEXT
        ),

        'strong' => array(
            'file' => 'Text/Wiki/Rule/strong.php',
            'name' => 'Text_Wiki_Rule_strong',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_STRONG
        ),

        'bold' => array(
            'file' => 'Text/Wiki/Rule/bold.php',
            'name' => 'Text_Wiki_Rule_bold',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_BOLD
        ),

        'emphasis' => array(
            'file' => 'Text/Wiki/Rule/emphasis.php',
            'name' => 'Text_Wiki_Rule_emphasis',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_EMPHASIS
        ),

        'italic' => array(
            'file' => 'Text/Wiki/Rule/italic.php',
            'name' => 'Text_Wiki_Rule_italic',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_ITALIC
        ),

        'tt' => array(
            'file' => 'Text/Wiki/Rule/tt.php',
            'name' => 'Text_Wiki_Rule_tt',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_TT
        ),

        'superscript' => array(
            'file' => 'Text/Wiki/Rule/superscript.php',
            'name' => 'Text_Wiki_Rule_superscript',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_SUPERSCRIPT
        ),

        'revise' => array(
            'file' => 'Text/Wiki/Rule/revise.php',
            'name' => 'Text_Wiki_Rule_revise',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_REVISE
        ),

        'tighten' => array(
            'file' => 'Text/Wiki/Rule/tighten.php',
            'name' => 'Text_Wiki_Rule_tighten',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_TIGHTEN
        ),

        'entities' => array(
            'file' => 'Text/Wiki/Rule/entities.php',
            'name' => 'Text_Wiki_Rule_entities',
            'flag' => true,
            'conf' => array(),
            'desc' => PLUGIN_EVENT_TEXTWIKI_RULE_DESC_ENTITIES
        )
    );

    var $nonWikiRules = array();
    var $title = PLUGIN_EVENT_TEXTWIKI_NAME;

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_TEXTWIKI_NAME);
        $propbag->add('description',   PLUGIN_EVENT_TEXTWIKI_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Tobias Schlitt');
        $propbag->add('version',       '1.1');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('cachable_events', array('frontend_display' => true));
        $propbag->add('event_hooks',   array('frontend_display' => true, 'frontend_comment' => true));
        $propbag->add('groups', array('MARKUP'));

        $this->markup_elements = array(
            array(
              'name'     => 'ENTRY_BODY',
              'element'  => 'body',
            ),
            array(
              'name'     => 'EXTENDED_BODY',
              'element'  => 'extended',
            ),
            array(
              'name'     => 'COMMENT',
              'element'  => 'comment',
            ),
            array(
              'name'     => 'HTML_NUGGET',
              'element'  => 'html_nugget',
            )
        );

        $conf_array = array();
        // Add markup elements config
        foreach($this->markup_elements as $element) {
            $conf_array[] = $element['name'];
        }
        // Save non wiki-rule configuration
        $this->nonWikiRules = $conf_array;
        // Seperate markup elements from wiki-rule config
        $conf_array[] = "internal_seperator";
        // Add wiki-rule config
        $this->_introspect_rule_config($conf_array);
        $propbag->add('configuration', $conf_array);
    }

    function _introspect_rule_config(&$conf_array) {
        foreach($this->wikiRules as $name => $rule) {
            // If sub configurations exist
            if (isset($rule['s9yc']) && is_array($rule['s9yc'])) {
                if ($conf_array[(count($conf_array) - 1)] != 'internal_seperator') {
                    $conf_array[] = 'internal_seperator';
                }
                // Add wiki-rule config itself
                $conf_array[] = $name;
                foreach ($rule['s9yc'] as $confname => $conf) {
                    $conf_array[] = $name . '_' . $confname;
                }
                $conf_array[] = "internal_seperator";
            } else {
                // Add only wiki-rule config itself
                $conf_array[] = $name;
            }
        }
    }

    function generate_content(&$title) {
        $title = $this->title;
    }

    function install() {
        serendipity_plugin_api::hook_event('backend_cache_entries', $this->title);
    }

    function uninstall() {
        serendipity_plugin_api::hook_event('backend_cache_purge', $this->title);
        serendipity_plugin_api::hook_event('backend_cache_entries', $this->title);
    }

    function introspect_config_item($name, &$propbag)
    {
        if (in_array($name, $this->nonWikiRules)) {
            $propbag->add('type',        'boolean');
            $propbag->add('name',        defined($name) ? constant($name) : $name);
            $propbag->add('description', sprintf(APPLY_MARKUP_TO, defined($name) ? constant($name) : $name));
        } else if ($name == 'internal_seperator') {
            $propbag->add('type',        'seperator');
            $propbag->add('name',        'Seperator');
            $propbag->add('description', 'Seperator');
        } else {
            $this->_introspect_rule_config_item($name, $propbag);
        }
        return true;
    }

    function _introspect_rule_config_item($name, &$propbag) {
        if (strpos($name, '_') === false) {
            $propbag->add('type',        'boolean');
            $propbag->add('name',        ucfirst($name));
            $propbag->add('description', $this->wikiRules[$name]['desc']);
            return true;
        } else {
            $parts = explode('_', $name, 2);
            $conf = $this->wikiRules[$parts[0]]['s9yc'][$parts[1]];
            $propbag->add('type',        $conf['type']);
            $propbag->add('name',        ucfirst($parts[0]).' '.ucwords((str_replace('_', ' ',$parts[1]))));
            $propbag->add('description', $conf['desc']);
            return true;
        }
    }

    function event_hook($event, &$bag, &$eventData) {
        global $serendipity;

        $hooks = &$bag->get('event_hooks');

        if (!isset($this->wiki) || !is_a($this->wiki, 'text_wiki')) {
            $this->_init_wiki($bag);
        }

        if (isset($hooks[$event])) {
            switch($event) {
                case 'frontend_display':
                    foreach ($this->markup_elements as $temp) {
                        if (serendipity_db_bool($this->get_config($temp['name'], true)) && isset($eventData[$temp['element']])) {
                            $element = $temp['element'];
                            $eventData[$element] = $this->wiki->transform($eventData[$element]);
                        }
                    }
                    return true;
                    break;

                case 'frontend_comment':
                    if (serendipity_db_bool($this->get_config('COMMENT', true))) {
                        echo '<div class="serendipity_commentDirection">' . PLUGIN_EVENT_TEXTWIKI_TRANSFORM . '</div>';
                    }
                    return true;
                    break;

                default:
                  return false;
            }
        } else {
            return false;
        }
    }

    function _init_wiki(&$bag) {
        include_once 'Text/Wiki.php';

        if (class_exists('Text_Wiki')) {
            $this->wiki =& new Text_Wiki;
        } else {
            return false;
        }
        foreach ($this->wikiRules as $name => $rule) {
            if ($this->get_config($name, $rule['flag'])) {
                $this->_add_wiki_rule($bag, $name, $rule);
            }
        }
        return true;
    }

    function _add_wiki_rule(&$bag, $name, $rule) {
        $rule_info = $rule;
        $rule_info['flag'] = true;
        if (isset($rule['s9yc']) && is_array($rule['s9yc'])) {
            foreach ($rule['s9yc'] as $confName => $confVals) {
                if ($confName === 'pages') {
                    $rule_info['conf']['pages'] = $this->_get_link_pages($bag, $name);
                } else {
                    $rule_info['conf'][$confName] = $this->get_config($name.'_'.$confName, $rule_info['conf'][$confName]);
                }
            }
        }
        $this->wiki->insertRule($name, $rule_info);
        return true;
    }

    function _get_link_pages(&$bag, $ruleName) {
        global $serendipity;
        if ($this->get_config($ruleName.'_pages') === null) {
            return array();
        }
        $pagesFile = $this->get_config($ruleName.'_pages');
        if (!is_file($pagesFile)) {
            $cacheFile = $serendipity['uploadPath']."serendipity_plugin_event_wiki_".$ruleName.".cache";
            $cacheTime = (int)$this->get_config($ruleName.'_cachetime', 3600);
            if (!is_file($cacheFile) || (filemtime($cacheFile) + $cacheTime) < time()) {
                $pagesArray = @file($pagesFile);
                if (!$pagesArray) { return array(); }
                $putCache = @fopen($cacheFile, 'w');
                if (!$putCache) { return array(); }
                fputs($putCache, implode("", $pagesArray));
                fclose($putCache);
            }
            $pagesFile = $cacheFile;
        }
        $pagesArray = array_map(trim, file($pagesFile));
        return (is_array($pagesArray)) ? $pagesArray : array();
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
