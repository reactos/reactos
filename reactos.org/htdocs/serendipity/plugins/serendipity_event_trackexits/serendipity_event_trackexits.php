<?php # $Id: serendipity_event_trackexits.php 536 2005-10-06 07:51:34Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
} 

@define('PLUGIN_EVENT_TRACKBACK_NAME', 'Markup: Track exits');
@define('PLUGIN_EVENT_TRACKBACK_DESC', 'Track clicks on external URLs');
@define('PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION', 'Use URL-redirection on URL to commenting user?');
@define('PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION_BLAHBLA', 'Reduces Comment-Abuse but also disables positive effects of linking to the URL of a commenting user. Serendipity\'s internal routines will redirect to the target page and log the exit. The Google PageRank Deflector will prevent the link from gaining rank on Google. An empty value disables this feature entirely (default).');
@define('PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION_NONE', 'None');
@define('PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION_S9Y', 'Serendipity Exit-Tracking Routine');
@define('PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION_GOOGLE', 'Google PageRank Deflector');

class serendipity_event_trackexits extends serendipity_event
{
    var $title = PLUGIN_EVENT_TRACKBACK_NAME;
    var $links;

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_TRACKBACK_NAME);
        $propbag->add('description',   PLUGIN_EVENT_TRACKBACK_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '1.4');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('cachable_events', array('frontend_display' => true));
        $propbag->add('event_hooks',   array('frontend_display' => true));
        $propbag->add('scrambles_true_content', true);
        $propbag->add('groups', array('STATISTICS'));

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
        foreach($this->markup_elements as $element) {
            $conf_array[] = $element['name'];
        }
        $conf_array[] = 'commentredirection';
        $propbag->add('configuration', $conf_array);
    }

    function install() {
        serendipity_plugin_api::hook_event('backend_cache_entries', $this->title);
    }

    function uninstall() {
        serendipity_plugin_api::hook_event('backend_cache_purge', $this->title);
        serendipity_plugin_api::hook_event('backend_cache_entries', $this->title);
    }

    function generate_content(&$title) {
        $title = $this->title;
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'commentredirection':
                $propbag->add('type',        'select');
                $propbag->add('select_values', array('none'     => PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION_NONE,
                                                     's9y'      => PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION_S9Y,
                                                     'google'   => PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION_GOOGLE));
                $propbag->add('name',        PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION);
                $propbag->add('description', PLUGIN_EVENT_TRACKBACK_COMMENTREDIRECTION_BLAHBLA);
                $propbag->add('default', 'none');
                break;

            default:
                $propbag->add('type',        'boolean');
                $propbag->add('name',        constant($name));
                $propbag->add('description', sprintf(APPLY_MARKUP_TO, constant($name)));
                $propbag->add('default', 'true');
        }
        return true;
    }

    function event_hook($event, &$bag, &$eventData, $addData = null) {
        global $serendipity;

        $hooks = &$bag->get('event_hooks');

        if (isset($hooks[$event])) {
            switch($event) {
                case 'frontend_display':
                    if ($bag->get('scrambles_true_content') && is_array($addData) && isset($addData['no_scramble'])) {
                        return true;
                    }

                    $serendipity['encodeExitsCallback_entry_id'] = (int)(isset($eventData['entry_id']) ? $eventData['entry_id'] : $eventData['id']);

                    // Fetch all existing links from the database. They have been inserted there by our trackback-discovery.
                    if (empty($serendipity['encodeExitsCallback_entry_id'])) {
                        $this->links = array();
                    } else {
                        $this->links = serendipity_db_query("SELECT id, link FROM {$serendipity['dbPrefix']}references WHERE entry_id = {$serendipity['encodeExitsCallback_entry_id']}", false, 'both', false, 'link', 'id');
                    }

                    foreach ($this->markup_elements as $temp) {
                        if (serendipity_db_bool($this->get_config($temp['name'], true)) && isset($eventData[$temp['element']])) {
                            $element = $temp['element'];

                            $eventData[$element] = preg_replace_callback(
                                "#<a(.*)href=(\"|')http(s?)://([^\"']+)(\"|')([^>]*)>#isUm",
                                array($this, '_encodeExitsCallback'),
                                $eventData[$element]
                            );

                            if ($temp['element'] == 'comment' && !empty($eventData['url'])) {
                                switch(trim($this->get_config('commentredirection'))) {
                                    case 's9y':
                                        $eventData['url'] = $this->_encodeExitsCallback(
                                                              array(
                                                                1 => ' ',
                                                                2 => '"',
                                                                3 => '',
                                                                4 => $eventData['url'],
                                                                5 => '"'
                                                              ),
                                                              true
                                                            );
                                        break;

                                    case 'google':
                                        $eventData['url'] = 'http://www.google.com/url?sa=D&q=' . $eventData['url'];
                                        break;

                                    default:
                                        break;
                                }
                            }
                        }
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


    /**
    * Transforms '<a href="http://url/">' into
    * '<a href="exit.php?url=encurl" ...>'.
    */
    function _encodeExitsCallback($buffer, $url_only = false) {
        global $serendipity;

        $entry_id = $serendipity['encodeExitsCallback_entry_id'];
        $url      = 'http' . $buffer[3] . '://' . $buffer[4];

        if ($url_only) {
            return sprintf(
                '%sexit.php?url=%s%s',
                $serendipity['baseURL'],
                base64_encode($buffer[4]),
                ($entry_id != 0) ? '&amp;entry_id=' . $entry_id : ''
            );
        }

        $is_title = (stristr($buffer[0], 'title=')       !== false ? true : false);
        $is_over  = (stristr($buffer[0], 'onmouseover=') !== false ? true : false);
        $is_out   = (stristr($buffer[0], 'onmouseout=')  !== false ? true : false);

        $link     = '<a%shref="%sexit.php?url%s=%s%s" ' . (!$is_title ? 'title="%s" ' : '%s') . (!$is_over ? ' onmouseover="window.status=\'%s\';return true;" ' : '%s') . (!$is_out ? 'onmouseout="window.status=\'\';return true;"' : '') . '%s>';
        
        if (is_array($this->links) && !empty($this->links[$url])) {
            return sprintf(
                $link,
                $buffer[1],
                $serendipity['baseURL'],
                '_id',
                $this->links[$url],
                ($entry_id != 0) ? '&amp;entry_id=' . $entry_id : '',
                (!$is_title ? $url : ''),
                (!$is_over  ? $url : ''),
                $buffer[6]
            );
        } else {
            return sprintf(
                $link,
                $buffer[1],
                $serendipity['baseURL'],
                '',
                base64_encode($url),
                ($entry_id != 0) ? '&amp;entry_id=' . $entry_id : '',
                (!$is_title ? $url : ''),
                (!$is_over  ? $url : ''),
                $buffer[6]
            );
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
