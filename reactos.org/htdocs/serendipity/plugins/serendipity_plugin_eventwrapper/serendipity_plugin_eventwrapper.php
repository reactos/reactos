<?php # $Id: serendipity_plugin_eventwrapper.php 346 2005-08-01 17:35:25Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
} 

@define('PLUGIN_EVENT_WRAPPER_NAME', 'Event-Output wrapper');
@define('PLUGIN_EVENT_WRAPPER_DESC', 'Displays gathered data by a certain event plugin');
@define('PLUGIN_EVENT_WRAPPER_PLUGIN', 'Source event plugin');
@define('PLUGIN_EVENT_WRAPPER_PLUGINDESC', 'Select the event plugin for which the output should be displayed');
@define('PLUGIN_EVENT_WRAPPER_TITLEDESC', 'Enter the title for this sidebar item (leave empty for inheritance by event plugin)');

class serendipity_plugin_eventwrapper extends serendipity_plugin
{
var $rewrite_from, $rewrite_to;

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_WRAPPER_NAME);
        $propbag->add('description',   PLUGIN_EVENT_WRAPPER_DESC);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Garvin Hicking');
        $propbag->add('version',       '1.0');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('groups', array('FRONTEND_FEATURES'));
        $propbag->add('configuration', array('event_plugin', 'title'));

    }

    function introspect_config_item($name, &$propbag)
    {
        global $serendipity;

        switch($name) {
            case 'event_plugin':
                $plugins = serendipity_plugin_api::get_event_plugins();
                $select = array();

                if (is_array($plugins)) {
                    foreach($plugins AS $plugname => $plugarray) {
                        $select[$plugname] = $plugarray['t'];
                    }
                }

                $propbag->add('type',        'select');
                $propbag->add('name',        PLUGIN_EVENT_WRAPPER_PLUGIN);
                $propbag->add('description', PLUGIN_EVENT_WRAPPER_PLUGINDESC);
                $propbag->add('select_values', $select);
                $propbag->add('default', 'false');
                break;

            case 'title':
                $propbag->add('type',        'string');
                $propbag->add('name',        TITLE);
                $propbag->add('description', PLUGIN_EVENT_WRAPPER_TITLEDESC);
                $propbag->add('default', '');
                break;

            default:
                    return false;
        }
        return true;
    }

    function generate_content(&$title)
    {
        $plug = $this->get_config('event_plugin', 'false');
        if ($plug == 'false') {
            return;
        }

        $wrap = &serendipity_plugin_api::get_event_plugins($plug);
        $faketitle = '';
        if (is_object($wrap)) {
            $wrap->generate_content($faketitle);
        }

        if ($this->get_config('title') != '') {
            $title = $this->get_config('title');
        } else {
            $title = $faketitle;
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>