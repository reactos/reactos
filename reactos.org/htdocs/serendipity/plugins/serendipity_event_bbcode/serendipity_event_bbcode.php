<?php # $Id: serendipity_event_bbcode.php 523 2005-10-02 21:47:56Z omidmottaghi $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_EVENT_BBCODE_NAME',     'Markup: BBCode');
@define('PLUGIN_EVENT_BBCODE_DESC',     'Markup text using BBCode');
@define('PLUGIN_EVENT_BBCODE_TRANSFORM', '<a href="http://www.phpbb.com/phpBB/faq.php?mode=bbcode">BBCode</a> format allowed');

class serendipity_event_bbcode extends serendipity_event
{
    var $title = PLUGIN_EVENT_BBCODE_NAME;
    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_BBCODE_NAME);
        $propbag->add('description',   PLUGIN_EVENT_BBCODE_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Jez Hancock, Garvin Hicking');
        $propbag->add('version',       '2.0');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('cachable_events', array('frontend_display' => true));
        $propbag->add('event_hooks',   array('frontend_display' => true, 'frontend_comment' => true, 'css' => true));
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
        $conf_array[] = 'info';
        foreach($this->markup_elements as $element) {
            $conf_array[] = $element['name'];
        }
        $propbag->add('configuration', $conf_array);
    }


    function bbcode_callback($matches) {
        $type  = $matches[1];
        $input = trim($matches[2], "\r\n");

        switch ($type) {
            case 'code':
                $search_replace = array(
                    '&'         => '&amp;',
                    ' '         => '&#160;',
                    '&lt;'      => '&#60;',
                    '<'         => '&#60;',
                    '&gt;'      => '&#62;',
                    '>'         => '&#62;',
                    '&quot;'    => '&#34;',
                    ':'         => '&#58;',
                    '['         => '&#91;',
                    ']'         => '&#93;',
                    ')'         => '&#41;',
                    '('         => '&#40;',
                    '*'         => '&#42;',
                    '\t'        => '&#160;&#160;&#160;&#160;',
                    '\\"'       => '"',
                    "\\'"       => "'"
                );

                $input = strtr($input, $search_replace);
                break;

            case 'php':
                if (substr($input, 0, 2) != '<?') {
                    $input = "<?php\n\n$input\n\n?>";
                }

                ob_start();
                highlight_string($input);
                $input = ob_get_contents();
                ob_end_clean();

                $input = str_replace('<br />', "\n", $input);
                break;

            default:
                return false;
        }

        $input = "<div class=\"bb-$type-title\">" . strtoupper($type) . ":</div>"
                . "<div class=\"bb-$type\">$input</div>";
        return($input);
    }

    function generate_content(&$title) {
        $title = $this->title;
    }


    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'info':
                $propbag->add('type',        'info');
                $propbag->add('description',  PLUGIN_EVENT_BBCODE_TRANSFORM);
                break;

            default:
                $propbag->add('type',        'boolean');
                $propbag->add('name',        constant($name));
                $propbag->add('description', sprintf(APPLY_MARKUP_TO, constant($name)));
                $propbag->add('default', 'true');
                break;
        }
        return true;
    }

    function install() {
        serendipity_plugin_api::hook_event('backend_cache_entries', $this->title);
    }

    function uninstall() {
        serendipity_plugin_api::hook_event('backend_cache_purge', $this->title);
        serendipity_plugin_api::hook_event('backend_cache_entries', $this->title);
    }

    function bbcode($input) {
        static $bbcodes = null;

        // Only allow numbers and characters for CSS: "red", "#FF0000", ...
        static $pattern_css   = '([ 0-9a-z#-]+?)';

        // Only allow strings occuring in emails: .-_@, 0-9, a-z
        static $pattern_mail  = '([\.\-\+~@_0-9a-z]+?)';

        // Only allow strings occuring in URLs: &;?:.-_@/, 0-9, a-z
        static $pattern_url   = '([@!=~\?:&;0-9a-z#\.\-_\/]+?)';

        // Disallow possibly evil HTML characters which may lead to Javascript XSS: '"();
        static $pattern_query = '([^"\'\(\);]+?)';
        
        // Note: 
        //  * Anything between <xxx>...</xxx> tags will be caught by htmlspecialchars() and disallows custom HTML tags.
        //  * (?::\w+)? means "non capturing" match on any word character.
        //  * (?<!\\\\) means any bbcode which is not prefixed by \[...]        
        
        if ($bbcodes === null) {
            $bbcodes = array(
              '/(?<!\\\\)\[color(?::\w+)?=' . $pattern_css . '\](.*?)\[\/color(?::\w+)?\]/si'                 => "<span style=\"color:\\1\">\\2</span>",
              '/(?<!\\\\)\[size(?::\w+)?='  . $pattern_css . '\](.*?)\[\/size(?::\w+)?\]/si'                  => "<span style=\"font-size:\\1\">\\2</span>",
              '/(?<!\\\\)\[font(?::\w+)?='  . $pattern_css . '\](.*?)\[\/font(?::\w+)?\]/si'                  => "<span style=\"font-family:\\1\">\\2</span>",
              '/(?<!\\\\)\[align(?::\w+)?=' . $pattern_css . '\](.*?)\[\/align(?::\w+)?\]/si'                 => "<div style=\"text-align:\\1\">\\2</div>",
    
              '/(?<!\\\\)\[b(?::\w+)?\](.*?)\[\/b(?::\w+)?\]/si'                                              => "<span style=\"font-weight:bold\">\\1</span>",
              '/(?<!\\\\)\[i(?::\w+)?\](.*?)\[\/i(?::\w+)?\]/si'                                              => "<span style=\"font-style:italic\">\\1</span>",
              '/(?<!\\\\)\[u(?::\w+)?\](.*?)\[\/u(?::\w+)?\]/si'                                              => "<span style=\"text-decoration:underline\">\\1</span>",
              '/(?<!\\\\)\[center(?::\w+)?\](.*?)\[\/center(?::\w+)?\]/si'                                    => "<div style=\"text-align:center\">\\1</div>",
    
              // [email]
              '/(?<!\\\\)\[email(?::\w+)?\]' . $pattern_mail . '\[\/email(?::\w+)?\]/si'                      => "<a href=\"mailto:\\1\" class=\"bb-email\">\\1</a>",
              '/(?<!\\\\)\[email(?::\w+)?='  . $pattern_mail . '\](.*?)\[\/email(?::\w+)?\]/si'               => "<a href=\"mailto:\\1\" class=\"bb-email\">\\2</a>",
    
              // [url]
              '/(?<!\\\\)\[(google|search)\]'   . $pattern_query . '\[\/(google|search)\]/si'                 => "<a href=\"http://www.google.com/search?q=\\2\" target=\"_blank\" class=\"bb-url\">\\2</a>",
              '/(?<!\\\\)\[url(?::\w+)?\]www\.' . $pattern_url   . '\[\/url(?::\w+)?\]/si'                    => "<a href=\"http://www.\\1\" target=\"_blank\" class=\"bb-url\">\\1</a>",
              '/(?<!\\\\)\[url(?::\w+)?\]'      . $pattern_url   . '\[\/url(?::\w+)?\]/si'                    => "<a href=\"\\1\" target=\"_blank\" class=\"bb-url\">\\1</a>",
              '/(?<!\\\\)\[url(?::\w+)?='       . $pattern_url   . '?\](.*?)\[\/url(?::\w+)?\]/si'            => "<a href=\"\\1\" target=\"_blank\" class=\"bb-url\">\\2</a>",
    
              // [img]
              '/(?<!\\\\)\[img(?::\w+)?\]' . $pattern_url . '\[\/img(?::\w+)?\]/si'                           => "<img src=\"\\1\" alt=\"\\1\" class=\"bb-image\" />",
              '/(?<!\\\\)\[img(?::\w+)?=([0-9]*?)x([0-9]*?)\]' . $pattern_url . '\[\/img(?::\w+)?\]/si'       => "<img width=\"\\1\" height=\"\\2\" src=\"\\3\" alt=\"\\3\" class=\"bb-image\" />",
    
              // [quote]
              '/(?<!\\\\)\[quote(?::\w+)?\](.*?)\[\/quote(?::\w+)?\]/si'                                      => "<div class=\"bb-code-title\">QUOTE:<div class=\"bb-code\">\\1</div></div>",
              '/(?<!\\\\)\[quote(?::\w+)?=(?:&quot;|"|\')?(.*?)["\']?(?:&quot;|"|\')?\](.*?)\[\/quote\]/si'   => "<div class=\"bb-code-title\">QUOTE \\1:<div class=\"bb-code\">\\2</div></div>",
    
              // [list]
              '/(?<!\\\\)(?:\s*<br\s*\/?>\s*)?\[\*(?::\w+)?\](.*?)(?=(?:\s*<br\s*\/?>\s*)?\[\*|(?:\s*<br\s*\/?>\s*)?\[\/?list)/si' => "\n<li class=\"bb-listitem\">\\1</li>",
              '/(?<!\\\\)(?:\s*<br\s*\/?>\s*)?\[\/list(:(?!u|o)\w+)?\](?:<br\s*\/?>)?/si'    => "\n</ul>",
              '/(?<!\\\\)(?:\s*<br\s*\/?>\s*)?\[\/list:u(:\w+)?\](?:<br\s*\/?>)?/si'         => "\n</ul>",
              '/(?<!\\\\)(?:\s*<br\s*\/?>\s*)?\[\/list:o(:\w+)?\](?:<br\s*\/?>)?/si'         => "\n</ol>",
              '/(?<!\\\\)(?:\s*<br\s*\/?>\s*)?\[list(:(?!u|o)\w+)?\]\s*(?:<br\s*\/?>)?/si'   => "\n<ul class=\"bb-list-unordered\">",
              '/(?<!\\\\)(?:\s*<br\s*\/?>\s*)?\[list:u(:\w+)?\]\s*(?:<br\s*\/?>)?/si'        => "\n<ul class=\"bb-list-unordered\">",
              '/(?<!\\\\)(?:\s*<br\s*\/?>\s*)?\[list:o(:\w+)?\]\s*(?:<br\s*\/?>)?/si'        => "\n<ol class=\"bb-list-ordered\">",
              '/(?<!\\\\)(?:\s*<br\s*\/?>\s*)?\[list(?::o)?(:\w+)?=1\]\s*(?:<br\s*\/?>)?/si' => "\n<ol class=\"bb-list-ordered,bb-list-ordered-d\">",
              '/(?<!\\\\)(?:\s*<br\s*\/?>\s*)?\[list(?::o)?(:\w+)?=i\]\s*(?:<br\s*\/?>)?/s'  => "\n<ol class=\"bb-list-ordered,bb-list-ordered-lr\">",
              '/(?<!\\\\)(?:\s*<br\s*\/?>\s*)?\[list(?::o)?(:\w+)?=I\]\s*(?:<br\s*\/?>)?/s'  => "\n<ol class=\"bb-list-ordered,bb-list-ordered-ur\">",
              '/(?<!\\\\)(?:\s*<br\s*\/?>\s*)?\[list(?::o)?(:\w+)?=a\]\s*(?:<br\s*\/?>)?/s'  => "\n<ol class=\"bb-list-ordered,bb-list-ordered-la\">",
              '/(?<!\\\\)(?:\s*<br\s*\/?>\s*)?\[list(?::o)?(:\w+)?=A\]\s*(?:<br\s*\/?>)?/s'  => "\n<ol class=\"bb-list-ordered,bb-list-ordered-ua\">",
    
              // escaped tags like \[b], \[color], \[url], ...
              '/\\\\(\[\/?\w+(?::\w+)*\])/'                                      => "\\1"
            );
        }

        /* Regular expressions taken from http://smarty.incutio.com/?page=BBCodePlugin Wiki (Andre Rabold) */
        $input = preg_replace(array_keys($bbcodes), array_values($bbcodes), $input);

        // [code] & [php]
        $input = preg_replace_callback('/(?<!\\\\)\[(code|php)(?::\w+)?\](.*?)\[\/\\1(?::\w+)?\]/si', array($this, 'bbcode_callback'), $input);
        return $input;

    }

    function event_hook($event, &$bag, &$eventData) {
        global $serendipity;

        $hooks = &$bag->get('event_hooks');

        if (isset($hooks[$event])) {
            switch($event) {
                case 'frontend_display':

                    foreach ($this->markup_elements as $temp) {
                        if (serendipity_db_bool($this->get_config($temp['name'], true)) && isset($eventData[$temp['element']])) {
                            $element = $temp['element'];
                            $eventData[$element] = $this->bbcode($eventData[$element]);
                        }
                    }
                    return true;
                    break;

                case 'frontend_comment':
                    if (serendipity_db_bool($this->get_config('COMMENT', true))) {
                        echo '<div class="serendipity_commentDirection">' . PLUGIN_EVENT_BBCODE_TRANSFORM . '</div>';
                    }
                    return true;
                    break;

                case 'css':
                    if (strpos($eventData, '.bb-code') !== false) {
                        // class exists in CSS, so a user has customized it and we don't need default
                        return true;
                    }
?>
.bb-code, .bb-php, .bb-code-title, .bb-php-title {
    margin-left: 20px;
    margin-right: 20px;
    color: black;
    direction: ltr;
}

.bb-code-title, .bb-php-title {
    margin-bottom: 2px;
    background-color:#CCCCCC;
    font-weight: bold;
    padding-left: 5px;
}

.bb-code, .bb-php {
    font-family: courier, "courier new";
    background-color: #DDDDDD;
    padding: 10px;
}
<?php
                return true;
                break;

                default:
                  return false;
            }
        } else {
            return false;
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
