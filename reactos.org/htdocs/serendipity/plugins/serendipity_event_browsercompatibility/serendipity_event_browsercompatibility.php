<?php # $Id: serendipity_event_browsercompatibility.php 346 2005-08-01 17:35:25Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_EVENT_BROWSERCOMPATIBILITY_TITLE', 'Browser Compatibility');
@define('PLUGIN_EVENT_BROWSERCOMPATIBILITY_DESC', 'Uses different (CSS) methods to enforce maximum browser compatibility');

class serendipity_event_browsercompatibility extends serendipity_event
{
    var $title = PLUGIN_EVENT_BROWSERCOMPATIBILITY_TITLE;

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_BROWSERCOMPATIBILITY_TITLE);
        $propbag->add('description',   PLUGIN_EVENT_BROWSERCOMPATIBILITY_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Garvin Hicking');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('version',       '1.0');
        $propbag->add('event_hooks',    array(
            'css' => true,
            'css_backend' => true,
            'external_plugin'  => true,
        ));
        $propbag->add('groups', array('BACKEND_TEMPLATES'));
    }

    function generate_content(&$title) {
        $title = PLUGIN_EVENT_BROWSERCOMPATIBILITY_TITLE;
    }

    function event_hook($event, &$bag, &$eventData) {
        global $serendipity;

        $hooks = &$bag->get('event_hooks');
        if (isset($hooks[$event])) {
            switch($event) {
                case 'css_backend':
                case 'css':
?>
img {
   behavior: url("<?php echo $serendipity['baseURL'] . ($serendipity['rewrite'] == 'none' ? $serendipity['indexFile'] . '?/' : ''); ?>plugin/pngbehavior.htc");
}
<?php
                    return true;
                    break;

                case 'external_plugin':
                    switch($eventData) {
                        case 'pngbehavior.htc':
                            header('Content-Type: text/x-component');
                            echo str_replace('{blanksrc}', serendipity_getTemplateFile('img/blank.gif'), file_get_contents(dirname(__FILE__) . '/pngbehavior.htc'));
                            return true;
                    }
                    return true;
                    break;

                default:
                    return false;
                    break;
            }
        } else {
            return false;
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>