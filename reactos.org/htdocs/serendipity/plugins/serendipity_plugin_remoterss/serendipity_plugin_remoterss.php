<?php # $Id: serendipity_plugin_remoterss.php 567 2005-10-18 08:07:40Z garvinhicking $

// Contributed by Udo Gerhards <udo@babyblaue-seiten.de>
// OPML Contributed by Richard Thomas Harrison <rich@mibnet.plus.com>

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_REMOTERSS_TITLE', 'Remote RSS/OPML-Blogroll Feed');
@define('PLUGIN_REMOTERSS_BLAHBLAH', 'Show items of a remote RSS/OPML feed (e.g. Blogroll)');
@define('PLUGIN_REMOTERSS_NUMBER', 'Number of entries');
@define('PLUGIN_REMOTERSS_NUMBER_BLAHBLAH', 'How many entries should be displayed? (Default: every entry of the feed)');
@define('PLUGIN_REMOTERSS_SIDEBARTITLE', 'Feed-Title');
@define('PLUGIN_REMOTERSS_SIDEBARTITLE_BLAHBLAH', 'Title of the feed in the blog sidebar');
@define('PLUGIN_REMOTERSS_RSSURI', 'RSS/OPML URI');
@define('PLUGIN_REMOTERSS_RSSURI_BLAHBLAH', 'URI of the RSS/OPML feed which you want to display');
@define('PLUGIN_REMOTERSS_NOURI', 'No RSS/OPML feed selected');
@define('PLUGIN_REMOTERSS_RSSTARGET', 'RSS/OPML linktarget');
@define('PLUGIN_REMOTERSS_RSSTARGET_BLAHBLAH', 'Target of the link to one of the displayed RSS items (Default: _blank)');
@define('PLUGIN_REMOTERSS_CACHETIME', 'When to update the feed?');
@define('PLUGIN_REMOTERSS_CACHETIME_BLAHBLAH', 'The contents of a feed are stored in a cache which will be updated as soon as its older than X seconds (Default: 3 hours)');
@define('PLUGIN_REMOTERSS_FEEDTYPE', 'Feedtype');
@define('PLUGIN_REMOTERSS_FEEDTYPE_BLAHBLAH', 'Choose the format of the remote Feed');
@define('PLUGIN_REMOTERSS_BULLETIMG', 'Bullet Image');
@define('PLUGIN_REMOTERSS_BULLETIMG_BLAHBLAH', 'Image to display before each headline.');
@define('PLUGIN_REMOTERSS_DISPLAYDATE', 'Display Date');
@define('PLUGIN_REMOTERSS_DISPLAYDATE_BLAHBLAH', 'Display the date below the headline?');

class s9y_remoterss_XMLTree {
    function GetChildren($vals, &$i) {
        $children = array();
        $cnt = sizeof($vals);
        while (++$i < $cnt) {
            // compare type
            switch ($vals[$i]['type']) {
                case 'cdata':
                    $children[] = $vals[$i]['value'];
                    break;

                case 'complete':
                    $children[] = array(
                        'tag'        => $vals[$i]['tag'],
                        'attributes' => $vals[$i]['attributes'],
                        'value'      => $vals[$i]['value']
                    );
                    break;

                case 'open':
                    $children[] = array(
                        'tag'        => $vals[$i]['tag'],
                        'attributes' => $vals[$i]['attributes'],
                        'value'      => $vals[$i]['value'],
                        'children'   => $this->GetChildren($vals, $i)
                    );
                    break;

                case 'close':
                    return $children;
            }
        }
    }

    function GetXMLTree($file) {
        require_once S9Y_PEAR_PATH . 'HTTP/Request.php';
        $req = &new HTTP_Request($file);

        if (PEAR::isError($req->sendRequest()) || $req->getResponseCode() != '200') {
            $data = file_get_contents($file);
        } else {
            // Fetch file
            $data = $req->getResponseBody();
        }

        // Global replacements
        // by: waldo@wh-e.com - trim space around tags not within
        $data = eregi_replace('>[[:space:]]+<', '><', $data);

        // Flatten the input opml file to not have nested categories
        $data = preg_replace('@<outline[^>]+[^/]>@imsU', '', $data);
        $data = str_replace('</outline>', '', $data);

        // XML functions
        $xml_string = '<?xml version="1.0" encoding="UTF-8" ?>';
        if (preg_match('@(<\?xml.+\?>)@imsU', $data, $xml_head)) {
            $xml_string = $xml_head[1];
        }

        $encoding = 'UTF-8';
        if (preg_match('@encoding="([^"]+)"@', $xml_string, $xml_encoding)) {
            $encoding = $xml_encoding[1];
        }

        $p = xml_parser_create($encoding);
        // by: anony@mous.com - meets XML 1.0 specification
        @xml_parser_set_option($p, XML_OPTION_CASE_FOLDING, 0);
        xml_parser_set_option($p, XML_OPTION_TARGET_ENCODING, LANG_CHARSET);
        xml_parse_into_struct($p, $data, $vals, $index);
        xml_parser_free($p);

        $i = 0;
        $tree = array();
        $tree[] = array(
            'tag'        => $vals[$i]['tag'],
            'attributes' => $vals[$i]['attributes'],
            'value'      => $vals[$i]['value'],
            'children'   => $this->GetChildren($vals, $i)
        );

        return $tree;
    }
}

define('OPMLDEBUG', '0');

class s9y_remoterss_OPML {
    var $cacheOPMLHead;
    var $cacheOPMLBody;
    var $cacheOPMLOutline;

    function s9y_remoterss_OPML() {
        $this->cacheOPMLHead    = array();
        $this->cacheOPMLBody    = array();
        $this->cacheOPMLOutline = array();
    }

    function parseOPML($file) {
        $xmltree  = new s9y_remoterss_XMLTree();
        $opmltree = $xmltree->GetXMLTree($file);

        return $opmltree[0];
    }

    function findOPMLTag($arr, $tag) {
        $i = 0;
        $tagindex = false;
        $children = $arr['children'];
        $cnt = count($children);

        while ($i < $cnt) {

            if ($children[$i]['tag'] == $tag) {
                $tagindex = $i;
                break;
            }

            ++$i;
        }

        return $tagindex !== false ? $tagindex : false;
    }

    function getOPMLTag($tree, $tag) {
        $tagindex = $this->findOPMLTag($tree, $tag);

        if (OPMLDEBUG == 1) {
            echo "\ngetOPMLTag('" . $tag . "') = " . $tagindex . "<pre>\n";
            print_r($tree['children'][$tagindex]);
            echo "\n</pre>\n";
        }

        return $tagindex !== false ? $tree['children'][$tagindex] : false;
    }

    function getOPMLHead($tree) {
        $head = array();

        if (isset($this->cacheOPMLHead) && count($this->cacheOPMLHead) != 0) {
            $head = $this->cacheOPMLHead;
        } else {

            if (OPMLDEBUG == 1) {
                echo "\ngetOPMLHead<br />\n";
            }

            $head = $this->getOPMLTag($tree, 'head');

            if ($head !== false) {
                $this->cacheOPMLHead = $head;

                if (OPMLDEBUG == 1) {
                    echo "\nCaching head<pre>\n";
                    print_r($this->cacheOPMLHead);
                    echo "\n</pre>\n";
                }
            } elseif (OPMLDEBUG == 1) {
                echo "\nfalse<br />\n";
            }

        }

        return $head['tag'] == 'head' ? $head : false;
    }

    function getOPMLBody($tree) {
        $body = array();

        if (isset($this->cacheOPMLBody) && count($this->cacheOPMLBody) != 0) {
                $body = $this->cacheOPMLBody;
        } else {

            if (OPMLDEBUG == 1) {
                echo "\ngetOPMLBody<br />\n";
            }

            $body = $this->getOPMLTag($tree, 'body');

            if ($body !== false) {
                $this->cacheOPMLBody = $body;

                if (OPMLDEBUG == 1) {
                    echo "\nCaching body<pre>\n";
                    print_r($this->cacheOPMLBody);
                    echo "\n</pre>\n";
                }

            } elseif (OPMLDEBUG == 1) {
                echo "\nfalse<br />\n";
            }
        }

        return $body['tag'] == 'body' ? $body : false;
    }

    function getOPMLOutline($tree, $index) {

        if (isset($this->cacheOPMLOutline[$index])) {
            return $this->cacheOPMLOutline[$index];
        }

        $body = $this->getOPMLBody($tree);

        if (!$body) {
            return false;
        }

        $outline = $body['children'][$index];

        if ($outline['tag'] == 'outline') {
            $this->cacheOPMLOutline[$index] = $outline;

            if (OPMLDEBUG == 1) {
                echo "\ngetOPMLOutline[" . $index . "]<br />\n";
                echo "\nCaching outline[" . $index . "]<pre>\n";
                print_r($this->cacheOPMLOutline[$index]);
                echo "\n</pre>\n";
            }

            return $outline;
        } else {
            return false;
        }
    }

    function getOPMLOutlineAttr($tree, $index) {
        $outline = $this->getOPMLOutline($tree, $index);

        return $outline != false ? $outline['attributes'] : false;
    }

}

class serendipity_plugin_remoterss extends serendipity_plugin {
    var $title = PLUGIN_REMOTERSS_TITLE;
    var $encoding = null;

    function introspect(&$propbag) {
        $this->title = $this->get_config('sidebartitle', $this->title);

        $propbag->add('name',          PLUGIN_REMOTERSS_TITLE);
        $propbag->add('description',   PLUGIN_REMOTERSS_BLAHBLAH);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Udo Gerhards, Richard Thomas Harrison');
        $propbag->add('version',       '1.5');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('configuration', array('number', 'displaydate', 'dateformat', 'sidebartitle', 'rssuri', 'charset', 'target', 'cachetime', 'feedtype', 'bulletimg', 'markup'));
        $propbag->add('groups', array('FRONTEND_EXTERNAL_SERVICES'));
    }

    function introspect_config_item($name, &$propbag) {
        switch($name) {
            case 'markup':
                $propbag->add('type', 'boolean');
                $propbag->add('name', DO_MARKUP);
                $propbag->add('description', DO_MARKUP_DESCRIPTION);
                $propbag->add('default', 'false');
                break;

            case 'charset':
                $propbag->add('type', 'radio');
                $propbag->add('name', CHARSET);
                $propbag->add('description', CHARSET);
                $propbag->add('default', 'native');

                $charsets = array();
                if (LANG_CHARSET != 'UTF-8') {
                    $charsets['value'][] = $charsets['desc'][] = 'UTF-8';
                }
                if (LANG_CHARSET != 'ISO-8859-1') {
                    $charsets['value'][] = $charsets['desc'][] = 'ISO-8859-1';
                }

                $charsets['value'][] = 'native';
                $charsets['desc'][]  = LANG_CHARSET;
                $propbag->add('radio', $charsets);
                break;

            case 'feedtype':
                $select = array('rss' => 'RSS', 'opml' => 'OPML');
                $propbag->add('type', 'select');
                $propbag->add('name', PLUGIN_REMOTERSS_FEEDTYPE);
                $propbag->add('description', PLUGIN_REMOTERSS_FEEDTYPE_BLAHBLAH);
                $propbag->add('select_values', $select);
                $propbag->add('default', 'rss');
                break;

            case 'number':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_REMOTERSS_NUMBER);
                $propbag->add('description', PLUGIN_REMOTERSS_NUMBER_BLAHBLAH);
                $propbag->add('default', '0');
                break;

            case 'dateformat':
                $propbag->add('type', 'string');
                $propbag->add('name', GENERAL_PLUGIN_DATEFORMAT);
                $propbag->add('description', sprintf(GENERAL_PLUGIN_DATEFORMAT_BLAHBLAH, '%A, %B %e. %Y'));
                $propbag->add('default', '%A, %B %e. %Y');
                break;

            case 'sidebartitle':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_REMOTERSS_SIDEBARTITLE);
                $propbag->add('description', PLUGIN_REMOTERSS_SIDEBARTITLE_BLAHBLAH);
                $propbag->add('default', '');
                break;

            case 'rssuri':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_REMOTERSS_RSSURI);
                $propbag->add('description', PLUGIN_REMOTERSS_RSSURI_BLAHBLAH);
                $propbag->add('default', '');
                break;

            case 'target':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_REMOTERSS_RSSTARGET);
                $propbag->add('description', PLUGIN_REMOTERSS_RSSTARGET_BLAHBLAH);
                $propbag->add('default', '_blank');
                break;

            case 'cachetime':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_REMOTERSS_CACHETIME);
                $propbag->add('description', PLUGIN_REMOTERSS_CACHETIME_BLAHBLAH);
                $propbag->add('default', 10800);
                break;

            case 'bulletimg':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_REMOTERSS_BULLETIMG);
                $propbag->add('description', PLUGIN_REMOTERSS_BULLETIMG_BLAHBLAH);
                $propbag->add('default', '');
                break;


            case 'displaydate':
                $propbag->add('type', 'boolean');
                $propbag->add('name', PLUGIN_REMOTERSS_DISPLAYDATE);
                $propbag->add('description', PLUGIN_REMOTERSS_BLAHBLAH);
                $propbag->add('default', 'true');
                break;

            default:
                return false;
        }
        return true;
    }

    // Check if a given URI is readable.
    function urlcheck($uri) {

        // These two substring comparisons are faster than one regexp.
        if ('http://' != substr($uri, 0, 7) && 'https://' != substr($uri, 0, 8)) {
            return false;
        }

        // Disabled by now. May get enabled in the future, but for now the extra HTTP call isn't worth trying.
        return true;
        require_once S9Y_PEAR_PATH . 'HTTP/Request.php';
        $req = &new HTTP_Request($uri);
        
        if (PEAR::isError($req->sendRequest()) || !preg_match('@^[23]..@', $req->getResponseCode())) {
            return false;
        } else {
            return true;
        }
    }

    function generate_content(&$title) {
        global $serendipity;

        $number       = $this->get_config('number');
        $displaydate  = $this->get_config('displaydate','true');
        $dateformat   = $this->get_config('dateformat');
        $sidebartitle = $title = $this->get_config('sidebartitle', $this->title);
        $rssuri       = $this->get_config('rssuri');
        $target       = $this->get_config('target');
        $cachetime    = $this->get_config('cachetime');
        $feedtype     = $this->get_config('feedtype', 'rss');
        $markup       = $this->get_config('markup', 'false');
        $bulletimg    = $this->get_config('bulletimg');
        $charset      = $this->get_config('charset', 'native');

        if (!$number || !is_numeric($number) || $number < 1) {
            $showAll = true;
        } else {
            $showAll = false;
        }

        if (!$dateformat || strlen($dateformat) < 1) {
            $dateformat = '%A, %B %e. %Y';
        }

        if (!$cachetime || !is_numeric($cachetime)) {
            $cachetime = 10800; // 3 hours in seconds
        }

        if (trim($rssuri)) {
            $feedcache = $serendipity['serendipityPath'] . 'templates_c/remoterss_cache_' . preg_replace('@[^a-z0-9]*@i', '', $rssuri) . '.dat';
            if (!file_exists($feedcache) || filesize($feedcache) == 0 || filemtime($feedcache) < (time() - $cachetime)) {
                if (!$this->urlcheck($rssuri)) {
                    echo '<!-- No valid URL! -->';
                } elseif ($feedtype == 'rss') {
                    // Touching the feedcache file will prevent loops of death when the RSS target is the same URI than our blog.
                    @touch($feedcache);
                    require_once S9Y_PEAR_PATH . 'Onyx/RSS.php';
                    $c = &new Onyx_RSS($charset);
                    $c->parse($rssuri);
                    $this->encoding = $c->rss['encoding'];

                    $i = 0;
                    $content = '';
                    while (($showAll || ($i < $number)) && ($item = $c->getNextItem())) {
                        if (empty($item['title'])) {
                            continue;
                        }
                        $content .= '<a href="' . $this->decode($item['link']) . '" ' . (!empty($target) ? 'target="'.$target.'"' : '') . '>';
                        if (!empty($bulletimg)) {
                            $content .= '<img src="' . $bulletimg . '" border="0" alt="*" /> ';
                        }
                        $content .= $this->decode($item['title']) . "</a><br />\n";
                        $item['timestamp'] = @strtotime(isset($item['pubdate']) ? $item['pubdate'] : $item['dc:date']);
                        if (!($item['timestamp'] == -1) AND ($displaydate == 'true')) {
                            $content .= '<div class="serendipitySideBarDate">'
                                      . htmlspecialchars(serendipity_formatTime($dateformat, $item['timestamp'], false))
                                      . '</div><br />';

                        }
                        ++$i;
                    }

                    $fp = @fopen($feedcache, 'w');
                    if (trim($content) != '' && $fp) {
                        fwrite($fp, $content);
                        fclose($fp);
                    } else {
                        echo '<!-- Cache failed to ' . $feedcache . ' in ' . getcwd() . ' --><br />';
                        if (trim($content) == '') {
                            $content = @file_get_contents($feedcache);
                        }
                    }
                } elseif ($feedtype == 'opml') {
                    // Touching the feedcache file will prevent loops of death when the RSS target is the same URI than our blog.
                    @touch($feedcache);

                    $opml = new s9y_remoterss_OPML();
                    $opmltree = $opml->parseOPML($rssuri);

                    if (OPMLDEBUG == 1) {
                        echo "\n<pre>\n";
                        print_r($opmltree);
                        echo "\n</pre>\n";
                    }

                    if ($opmltree['tag'] === 'opml') {
                        $head        = $opml->getOPMLHead($opmltree);
                        $ownerName   = $opml->getOPMLTag($head, 'ownerName');
                        $blogrolling = $ownerName != false ? ($ownerName['value'] == 'Blogroll Owner' ? true : false) : false;

                        $i = 0;
                        $content = '';
                        while (($showAll || ($i < $number)) && ($item = $opml->getOPMLOutlineAttr($opmltree, $i))) {
                            if (!empty($item['url'])) {
                                $url = $this->decode($item['url']);
                            } elseif (!empty($item['htmlUrl'])) {
                                $url = $this->decode($item['htmlUrl']);
                            } elseif (!empty($item['xmlUrl'])) {
                                $url = $this->decode($item['xmlUrl']);
                            } elseif (!empty($item['urlHTTP'])) {
                                $url = $this->decode($item['urlHTTP']);
                            } else {
                                $url = '';
                            }

                            if (!empty($item['text'])) {
                                $text = htmlspecialchars($this->decode($item['text']));
                            } elseif (!empty($item['title'])) {
                                $text = htmlspecialchars($this->decode($item['title']));
                            } elseif (!empty($item['description'])) {
                                $text = htmlspecialchars($this->decode($item['description']));
                            } else {
                                $text = '';
                            }

                            if ($blogrolling === true && (!empty($text) || !empty($url))) {
                                $content .= '&bull; <a href="' . $url . (!empty($target) ? 'target="'.$target.'"' : '') . ' title="' . $text . '">' . htmlspecialchars($text) . "</a>";
                                if (isset($item['isRecent'])) {
                                    $content .= ' <span style="color: Red; ">*</span>';
                                }
                                $content .= "<br />";
                            } elseif ((isset($item['type']) && $item['type'] == 'url') || !empty($url)) {
                                $content .= '&bull; <a href="' .$url . (!empty($target) ? 'target="'.$target.'"' : '') . ' title="' . $text . '">' . $text . "</a>";
                                $content .= "<br />";
                            }
                            ++$i;
                        }

                        /* Pretend to be a html_nugget so we can apply markup events. */
                        if ($markup == 'true') {
                            $entry = array('html_nugget' => $content);
                            serendipity_plugin_api::hook_event('frontend_display', $entry);
                            $content = $entry['html_nugget'];
                        }

                        $fp = @fopen($feedcache, 'w');
                        if (trim($content) != '' && $fp) {
                            fwrite($fp, $content);
                            fclose($fp);
                        } else {
                            echo '<!-- Cache failed to ' . $feedcache . ' in ' . getcwd() . ' --><br />';
                            if (trim($content) == '') {
                                $content = @file_get_contents($feedcache);
                            }
                        }
                    } else {
                        echo '<!-- Not a valid OPML feed -->';
                    }
                } else {
                    echo '<!-- no valid feedtype -->';
                }
            } else {
                $content = file_get_contents($feedcache);
            }

            echo $content;
        } else {
           echo PLUGIN_REMOTERSS_NOURI;
        }
    }

    function &decode($string) {
        $target = $this->get_config('charset', 'native');

        // xml_parser_* functions to recoding from ISO-8859-1/UTF-8
        if (LANG_CHARSET == 'ISO-8859-1' || LANG_CHARSET == 'UTF-8') {
            return $string;
        }

        switch($target) {
            case 'native':
                return $string;

            case 'ISO-8859-1':
                if (function_exists('iconv')) {
                    $out = iconv('ISO-8859-1', LANG_CHARSET, $string);
                } elseif (function_exists('recode')) {
                    $out = recode('iso-8859-1..' . LANG_CHARSET, $string);
                } else {
                    return $string;
                }
                
                return $out;
            
            case 'UTF-8':
            default:
                $out = utf8_decode($string);
                return $out;
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>