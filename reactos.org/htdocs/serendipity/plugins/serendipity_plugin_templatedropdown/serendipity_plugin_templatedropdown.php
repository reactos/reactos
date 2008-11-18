<?php # $Id: serendipity_plugin_templatedropdown.php 346 2005-08-01 17:35:25Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_TEMPLATEDROPDOWN_NAME',     'Template dropdown');
@define('PLUGIN_TEMPLATEDROPDOWN_DESC',     'Show a box to change templates');
@define('PLUGIN_TEMPLATEDROPDOWN_SUBMIT',   'Submit button?');
@define('PLUGIN_TEMPLATEDROPDOWN_SUBMIT_DESC',   'Show a submit button?');

class serendipity_plugin_templatedropdown extends serendipity_plugin {
    var $title = PLUGIN_TEMPLATEDROPDOWN_NAME;

    function introspect(&$propbag)
    {
        $this->title = $this->get_config('title', $this->title);

        $propbag->add('name',          PLUGIN_TEMPLATEDROPDOWN_NAME);
        $propbag->add('description',   PLUGIN_TEMPLATEDROPDOWN_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Evan Nemerson');
        $propbag->add('version',       '1.0');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('configuration', array('title', 'show_submit'));
        $propbag->add('groups', array('FRONTEND_VIEWS'));

        // Register (multiple) dependencies. KEY is the name of the depending plugin. VALUE is a mode of either 'remove' or 'keep'.
        // If the mode 'remove' is set, removing the plugin results in a removal of the depending plugin. 'Keep' meens to
        // not touch the depending plugin.
        $this->dependencies = array('serendipity_event_templatechooser' => 'remove');
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'title':
                $propbag->add('type',        'string');
                $propbag->add('name',        TITLE);
                $propbag->add('description', TITLE);
                $propbag->add('default',     '');
                break;

            case 'show_submit':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        PLUGIN_TEMPLATEDROPDOWN_SUBMIT);
                $propbag->add('description', PLUGIN_TEMPLATEDROPDOWN_SUBMIT_DESC);
                $propbag->add('default',     'false');
                break;

            default:
                    return false;
        }
        return true;
    }

    function generate_content(&$title) {
        global $serendipity;

        $title = $this->get_config('title', $this->title);

        $url = serendipity_currentURL();

        echo '<form id="theme_chooser" action="' . $url . '" method="post">';
        echo '<select name="user_template" onchange="document.getElementById(\'theme_chooser\').submit();">';
        foreach (serendipity_fetchTemplates() as $template) {
            $templateInfo = serendipity_fetchTemplateInfo($template);
            echo '<option value="' . $template . '" ' . (serendipity_get_config_var('template', 'default') == $template ? 'selected="selected"' : '') . '>' . $templateInfo['name'] . '</option>';
        }
        echo '</select>';

        if ($this->get_config('show_submit', 'false') == 'true') {
            echo '<input type="submit" name="submit" value="' . GO . '" size="4" />';
        }
        echo '</form>';
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
