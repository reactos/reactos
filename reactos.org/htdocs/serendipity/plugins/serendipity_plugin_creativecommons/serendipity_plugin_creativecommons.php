<?php # $Id: serendipity_plugin_creativecommons.php 346 2005-08-01 17:35:25Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_SIDEBAR_CREATIVECOMMONS_NAME', 'Creative Commons');
@define('PLUGIN_SIDEBAR_CREATIVECOMMONS_DESC', 'Display a creative commons notification in the sidebar.');

class serendipity_plugin_creativecommons extends serendipity_plugin {
    var $title = PLUGIN_SIDEBAR_CREATIVECOMMONS_NAME;

    function introspect(&$propbag)
    {
        $this->title = $this->get_config('title', $this->title);

        $propbag->add('name',          PLUGIN_SIDEBAR_CREATIVECOMMONS_NAME);
        $propbag->add('description',   PLUGIN_SIDEBAR_CREATIVECOMMONS_DESC);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Evan Nemerson');
        $propbag->add('version',       '1.0');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('configuration', array('title'));
        $propbag->add('groups', array('FRONTEND_EXTERNAL_SERVICES'));

        $this->dependencies = array('serendipity_event_creativecommons' => 'remove');
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'title':
                $propbag->add('type',          'string');
                $propbag->add('name',          TITLE);
                $propbag->add('description',   TITLE);
                $propbag->add('default', '');
                break;
        }
        return true;
    }

    function generate_content(&$title) {
      global $serendipity;

      $title = $this->get_config('title', $this->title);

      $eventData = array('display_dat' => '');
      serendipity_plugin_api::hook_event('frontend_display:html_layout', $eventData);
      echo $eventData['display_dat'];
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
