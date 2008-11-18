<?php # $Id: serendipity_event_emoticate.php 346 2005-08-01 17:35:25Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_EVENT_EMOTICATE_NAME', 'Markup: Emoticate');
@define('PLUGIN_EVENT_EMOTICATE_DESC', 'Convert standard emoticons into graphic images');
@define('PLUGIN_EVENT_EMOTICATE_TRANSFORM', 'Standard emoticons like :-) and ;-) are converted to images.');

class serendipity_event_emoticate extends serendipity_event
{
    var $title = PLUGIN_EVENT_EMOTICATE_NAME;

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_EMOTICATE_NAME);
        $propbag->add('description',   PLUGIN_EVENT_EMOTICATE_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '1.1');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('groups', array('MARKUP'));
        $propbag->add('cachable_events', array('frontend_display' => true));
        $propbag->add('event_hooks',   array('frontend_display' => true, 'frontend_comment' => true));

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
        $propbag->add('configuration', $conf_array);

    }

    function install() {
        serendipity_plugin_api::hook_event('backend_cache_entries', $this->title);
    }

    function uninstall() {
        serendipity_plugin_api::hook_event('backend_cache_purge', $this->title);
        serendipity_plugin_api::hook_event('backend_cache_entries', $this->title);
    }

    function getEmoticons() {
        global $serendipity;

        /* Avoid multiple runs of serendipity_getTemplateFile(),
           in other words - if we already have a list of smilies, don't bother looking for another */
        if (isset($this->smilies) && sizeof($this->smilies) != 0) {
            return $this->smilies;
        }

        /* Hijack global variable $serendipity['custom_emoticons'] if it exists */
        $hijack_file = serendipity_getTemplateFile('emoticons.inc.php', 'serendipityPath');
        if (@file_exists($hijack_file)) {
            @include $hijack_file; // This file contains $serendipity['custom_emoticons'] and maybe $serendipity['custom_emoticons_regexp']
            if (isset($serendipity['custom_emoticons']) && is_array($serendipity['custom_emoticons'])) {
                $this->smilies = $serendipity['custom_emoticons'];
                if (is_array($this->smilies) && (!isset($serendipity['custom_emoticons_regexp']) || !$serendipity['custom_emoticons_regexp'])) {
                    foreach($this->smilies AS $key => $val) {
                        unset($this->smilies[$key]);
                        $this->smilies[preg_quote($key, '/')] = $val;
                    }
                }
            }
        }

        if (!isset($this->smilies)) {
            $this->smilies = array(
                "\:'\("    => serendipity_getTemplateFile('img/emoticons/cry.png'),

                '\:\-?\)'  => serendipity_getTemplateFile('img/emoticons/smile.png'),

                '\:\|'     => serendipity_getTemplateFile('img/emoticons/normal.png'),

                '\:\-?O'  => serendipity_getTemplateFile('img/emoticons/eek.png'),

                '\:\-?\('  => serendipity_getTemplateFile('img/emoticons/sad.png'),

                '8\-?\)'  => serendipity_getTemplateFile('img/emoticons/cool.png'),

                '\:\-?D'  => serendipity_getTemplateFile('img/emoticons/laugh.png'),

                '\:\-?P'  => serendipity_getTemplateFile('img/emoticons/tongue.png'),

                ';\-?\)'  => serendipity_getTemplateFile('img/emoticons/wink.png'),
            );
        }

        return $this->smilies;
    }

    function humanReadableEmoticon($key) {
        return str_replace(array('-?', '\\'), array('-', ''), $key);
    }

    function generate_content(&$title) {
        $title = $this->title;
    }

    function example() {
        $s  = '<table cellspacing="5" style="margin-left: auto; margin-right: auto">';
        $s .= '<tr>';
        $i = 1;
        foreach($this->getEmoticons() as $key => $value) {
            $s .= '<td style="text-align: center">' . $this->humanReadableEmoticon($key) . '</td><td><img src="'. $value .'"></td>' . "\n";
            if ($i++ % 7 == 0) $s .= '</tr><tr>';
        }
        $s .= '</tr>';
        $s .= '</table>';

        return $s;
    }

    function introspect_config_item($name, &$propbag)
    {
        $propbag->add('type',        'boolean');
        $propbag->add('name',        constant($name));
        $propbag->add('description', sprintf(APPLY_MARKUP_TO, constant($name)));
        $propbag->add('default', 'true');
        return true;
    }


    function event_hook($event, &$bag, &$eventData) {
        global $serendipity;
        $hooks = &$bag->get('event_hooks');

        if (isset($hooks[$event])) {
            switch($event) {
                case 'frontend_display':

                    foreach ($this->markup_elements as $temp) {
                        if (serendipity_db_bool($this->get_config($temp['name'], true)) && isset($eventData[$temp['element']])) {
                            $element = &$eventData[$temp['element']];

                            foreach ($this->getEmoticons() as $key => $value) {
                                $element = preg_replace("/([\t\s\.\!>]+)" . $key . "([\t\s\!\.\)<]+|\$)/U",
                                    "$1<img src=\"$value\" alt=\"" . $this->humanReadableEmoticon($key) . "\" style=\"display: inline; vertical-align: bottom;\" class=\"emoticon\" />$2",
                                    $element);
                            }
                        }
                    }
                    return true;
                    break;

                case 'frontend_comment':
                    if (serendipity_db_bool($this->get_config('COMMENT', true))) {
                        echo '<div class="serendipity_commentDirection">' . PLUGIN_EVENT_EMOTICATE_TRANSFORM . '</div>';
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

}

/* vim: set sts=4 ts=4 expandtab : */
?>