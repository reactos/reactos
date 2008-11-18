<?php # $Id: serendipity_event_contentrewrite.php 346 2005-08-01 17:35:25Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_EVENT_CONTENTREWRITE_FROM', 'from');
@define('PLUGIN_EVENT_CONTENTREWRITE_TO', 'to');
@define('PLUGIN_EVENT_CONTENTREWRITE_NAME', 'Content Rewriter');
@define('PLUGIN_EVENT_CONTENTREWRITE_DESCRIPTION', 'Replaces words with a new selected string (useful for acronyms)');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWTITLE', 'New Title');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWTDESCRIPTION', 'Enter the acronym-title for a new item here ({from})');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDTITLE', 'Title #%d');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDTDESCRIPTION', 'Enter the acronym here ({from})');
@define('PLUGIN_EVENT_CONTENTREWRITE_PTITLE', 'Plugin-Title');
@define('PLUGIN_EVENT_CONTENTREWRITE_PDESCRIPTION', 'The name of this plugin');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWDESCRIPTION', 'New Description');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWDDESCRIPTION', 'Enter the description for a new item here ({to})');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDDESCRIPTION', 'Description #%s');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDDDESCRIPTION', 'Enter the description here ({to})');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRING', 'Rewrite string');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRINGDESC', 'The string used to rewrite. Place {from} and {to} anywhere you like to get a rewrite.' . "\n" . 'Example: <acronym title="{to}">{from}</acronym>');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHAR', 'Rewrite char');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHARDESC', 'If there is any char you append to force rewriting, enter it here. If you want to only replace \'serendipity*\' with what you entered for that word but want the \'*\' removed, enter that char here.');

class serendipity_event_contentrewrite extends serendipity_event
{
    var $title = PLUGIN_EVENT_CONTENTREWRITE_NAME;
    var $rewrite_from, $rewrite_to;

    function cleanup() {
        // Cleanup. Remove all empty configs on SAVECONF-Submit.
        serendipity_plugin_api::remove_plugin_value($this->instance, array('title', 'description'));

        return true;
    }

    function introspect(&$propbag)
    {
        global $serendipity;

        $this->title = $this->get_config('title', $this->title);
        $propbag->add('name',          PLUGIN_EVENT_CONTENTREWRITE_NAME);
        $propbag->add('description',   PLUGIN_EVENT_CONTENTREWRITE_DESCRIPTION);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Garvin Hicking');
        $propbag->add('version',       '1.1');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('cachable_events', array('frontend_display' => true));
        $propbag->add('event_hooks',    array(
            'frontend_display' => true
        ));

        $propbag->add('groups', array('BACKEND_EDITOR'));

        if (!empty($_POST['SAVECONF'])) $this->cleanup();
        $rows = serendipity_db_query("SELECT name, value FROM {$serendipity['dbPrefix']}config WHERE name LIKE '" . $this->instance . "/%' ORDER BY name");

        $counter = 0;
        $values  = array();
        $values[] = 'title';
        $values[] = 'rewrite_string';
        $values[] = 'rewrite_char';
        $this->title_values = array();
        $this->descr_values = array();
        $this->rewrite_from = array();
        $this->rewrite_to   = array();

        if (is_array($rows)) {
            foreach ($rows as $row) {
                $plugin_value = str_replace($this->instance . '/', '', $row['name']);
                preg_match('|^(.+)_([0-9]+)$|msU', $plugin_value, $reg_counter);

                $found = false;
                if ($reg_counter[1] == 'title') {
                    $this->title_values[] = $plugin_value;
                    $this->rewrite_from[] = $row['value'];
                    $found = true;
                } elseif ($reg_counter[1] == 'description') {
                    $this->descr_values[] = $plugin_value;
                    $this->rewrite_to[]   = $row['value'];
                    $found = true;
                }

                if ($found) {
                    $next_counter = $reg_counter[2] + 1;
                    $counter      = ($next_counter > $counter ? $next_counter : $counter);
                }
            }
        }

        $counter        = ($counter > 0 ? $counter + 1 : $counter);

        if (count($this->title_values) == count($this->descr_values)) {
            foreach($this->title_values AS $key => $val) {
                $values[] = $this->title_values[$key];
                $values[] = $this->descr_values[$key];
            }
        }

        $values[] = 'title'       . '_' . $counter;
        $values[] = 'description' . '_' . $counter;

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

        foreach($this->markup_elements as $element) {
            $values[] = $element['name'];
        }

        $propbag->add('configuration', $values);
        $this->counter = $counter;
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
        foreach($this->markup_elements as $element) {
            if ($name==$element['name']) {
                $propbag->add('type',        'boolean');
                $propbag->add('name',        constant($name));
                $propbag->add('description', sprintf(APPLY_MARKUP_TO, constant($name)));
                return true;
            }
        }

        preg_match('|^(.+)_([0-9]+)$|msU', $name, $switch);

        if (!isset($switch[2])) {
            $switch[1] = $name;
            $switch[2] = 'empty';
        }

        switch($switch[1]) {
            case 'title':

                    if ($switch[2] != 'empty') {
                        $propbag->add('type',        'string');

                        if ($this->counter == $switch[2]) {
                            $propbag->add('name',        PLUGIN_EVENT_CONTENTREWRITE_NEWTITLE);
                            $propbag->add('description', PLUGIN_EVENT_CONTENTREWRITE_NEWTDESCRIPTION);
                        } else {
                            $propbag->add('name',        sprintf(PLUGIN_EVENT_CONTENTREWRITE_OLDTITLE, $switch[2]));
                            $propbag->add('description', PLUGIN_EVENT_CONTENTREWRITE_OLDTDESCRIPTION);
                        }
                    } else {
                        $propbag->add('type',        'string');
                        $propbag->add('name',        PLUGIN_EVENT_CONTENTREWRITE_PTITLE);
                        $propbag->add('description', PLUGIN_EVENT_CONTENTREWRITE_PDESCRIPTION);
                    }
                    break;

            case 'description':
                    $propbag->add('type',        'string');

                    if ($this->counter == $switch[2]) {
                        $propbag->add('name',        PLUGIN_EVENT_CONTENTREWRITE_NEWDESCRIPTION);
                        $propbag->add('description', PLUGIN_EVENT_CONTENTREWRITE_NEWDDESCRIPTION);
                    } else {
                        $propbag->add('name',        sprintf(PLUGIN_EVENT_CONTENTREWRITE_OLDDESCRIPTION, $switch[2]));
                        $propbag->add('description', PLUGIN_EVENT_CONTENTREWRITE_OLDDDESCRIPTION);
                    }
                    break;

            case 'rewrite_string':
                    $propbag->add('type',         'string');
                    $propbag->add('name',         PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRING);
                    $propbag->add('description',  PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRINGDESC);
                    break;

            case 'rewrite_char':
                    $propbag->add('type',         'string');
                    $propbag->add('name',         PLUGIN_EVENT_CONTENTREWRITE_REWRITECHAR);
                    $propbag->add('description',  PLUGIN_EVENT_CONTENTREWRITE_REWRITECHARDESC);
                    break;

            default:
                    return false;
        }
        return true;
    }

    function generate_content(&$title)
    {
        $title = $this->get_config('title', $this->title);
?>
<table>
<?php
    if (is_array($this->rewrite_from)) {
            foreach($this->rewrite_from AS $key => $val) {
?>
    <tr>
        <th style="font-size: 8pt; font-color: white;"><?php echo htmlspecialchars($val); ?></th>
        <td><?php echo htmlspecialchars($this->rewrite_to[$key]); ?></td>
    </tr>
<?php
            }
        }
?>
</table>
<?php
    }

    function event_hook($event, &$bag, &$eventData) {
        global $serendipity;

        $hooks = &$bag->get('event_hooks');
        if (isset($hooks[$event])) {
            switch($event) {
                case 'frontend_display':
                    $char   = &$this->get_config('rewrite_char');
                    $string = &$this->get_config('rewrite_string');
                    foreach($this->rewrite_from AS $nr => $v_from) {
                        $v_to   = $this->rewrite_to[$nr];
                        if ($v_from != '' && $v_to != '') {
                            // Use the supplied rewrite string and replace the {from} and {to} values with the ones we got from the plugin
                            $new =
                              str_replace(
                                array(
                                  '{' . PLUGIN_EVENT_CONTENTREWRITE_FROM . '}',
                                  '{' . PLUGIN_EVENT_CONTENTREWRITE_TO . '}'
                                ),

                                array(
                                  str_replace(
                                    $char,
                                    '',
                                    $v_from
                                  ),

                                  $v_to
                                ),

                                $string
                            );

                            // Build a regular expression (ungreedy, multiline) with our quoted value. $val here is the word needing the replacement
                            $regex = '°([^\d\w])(' . preg_quote($v_from) . ')([^\d\w])°msU';

                            // \1 and \3 are the prepend/append strings (usually whitespaces) and $new containing the new value
                            foreach ($this->markup_elements as $temp) {
                                if (serendipity_db_bool($this->get_config($temp['name'], true)) && isset($eventData[$temp['element']])) {
                                    $element = $temp['element'];
                                    $eventData[$element] = preg_replace($regex, '\1' . $new . '\3', $eventData[$element]);
                                }
                            }

                        }
                    }

                    return true;
                    break;
            }
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>