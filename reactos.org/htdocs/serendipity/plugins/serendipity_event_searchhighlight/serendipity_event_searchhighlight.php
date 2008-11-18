<?php # $Id: serendipity_event_searchhighlight.php 346 2005-08-01 17:35:25Z garvinhicking $

/**********************************/
/*  Authored by Tom Sommer, 2004  */
/**********************************/

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
} 

@define('PLUGIN_EVENT_SEARCHHIGHLIGHT_NAME',     'Highlight search queries');
@define('PLUGIN_EVENT_SEARCHHIGHLIGHT_DESC',     'Highlights queries used in the referring search engine to locate your page');

class serendipity_event_searchhighlight extends serendipity_event
{
    var $title = PLUGIN_EVENT_SEARCHHIGHLIGHT_NAME;

    function introspect(&$propbag)
    {
        global $serendipity;

        $propbag->add('name',          PLUGIN_EVENT_SEARCHHIGHLIGHT_NAME);
        $propbag->add('description',   PLUGIN_EVENT_SEARCHHIGHLIGHT_DESC);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Tom Sommer');
        $propbag->add('version',       '1.1');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('event_hooks',   array('frontend_display' => true, 'css' => true));
        $propbag->add('groups', array('FRONTEND_EXTERNAL_SERVICES'));

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


    function generate_content(&$title) {
        $title = $this->title;
    }


    function introspect_config_item($name, &$propbag)
    {
        $propbag->add('type',        'boolean');
        $propbag->add('name',        constant($name));
        $propbag->add('description', sprintf(APPLY_MARKUP_TO, constant($name)));
        $propbag->add('default',     'true');
        return true;
    }

    function loadConstants() {
        define('PLUGIN_EVENT_SEARCHHIGHLIGHT_NONE', 0);
        define('PLUGIN_EVENT_SEARCHHIGHLIGHT_GOOGLE', 1);
        define('PLUGIN_EVENT_SEARCHHIGHLIGHT_YAHOO', 2);
        define('PLUGIN_EVENT_SEARCHHIGHLIGHT_LYCOS', 3);
        define('PLUGIN_EVENT_SEARCHHIGHLIGHT_MSN', 4);
        define('PLUGIN_EVENT_SEARCHHIGHLIGHT_ALTAVISTA', 5);
        define('PLUGIN_EVENT_SEARCHHIGHLIGHT_AOL_DE', 6);
        define('PLUGIN_EVENT_SEARCHHIGHLIGHT_AOL_COM', 7);
    }

    function getSearchEngine() {
        $url = parse_url($this->uri);

        /* Patterns should be placed in the order in which they are most likely to occur */
        if ( preg_match('@^(www\.)?google\.@i', $url['host']) ) {
            return PLUGIN_EVENT_SEARCHHIGHLIGHT_GOOGLE;
        }
        if ( preg_match('@^search\.yahoo\.@i', $url['host']) ) {
            return PLUGIN_EVENT_SEARCHHIGHLIGHT_YAHOO;
        }
        if ( preg_match('@^search\.lycos\.@i', $url['host']) ) {
            return PLUGIN_EVENT_SEARCHHIGHLIGHT_LYCOS;
        }
        if ( preg_match('@^search\.msn\.@i', $url['host']) ) {
            return PLUGIN_EVENT_SEARCHHIGHLIGHT_MSN;
        }
        if ( preg_match('@^(www\.)?altavista\.@i', $url['host']) ) {
            return PLUGIN_EVENT_SEARCHHIGHLIGHT_ALTAVISTA;
        }
        if ( preg_match('@^suche\.aol\.de@i', $url['host']) ) {
            return PLUGIN_EVENT_SEARCHHIGHLIGHT_AOL_DE;
        }
        if ( preg_match('@^search\.aol\.com@i', $url['host']) ) {
            return PLUGIN_EVENT_SEARCHHIGHLIGHT_AOL_COM;
        }



        return false;
    }

    function getQuery() {
        if ( empty($this->uri) ) {
            return false;
        }

        $this->loadConstants();
        $url = parse_url($this->uri);
        parse_str($url['query'], $pStr);

        switch ( $this->getSearchEngine() ) {
            case PLUGIN_EVENT_SEARCHHIGHLIGHT_GOOGLE :
                $query = $pStr['q'];
                break;

            case PLUGIN_EVENT_SEARCHHIGHLIGHT_YAHOO :
                $query = $pStr['p'];
                break;

            case PLUGIN_EVENT_SEARCHHIGHLIGHT_LYCOS :
                $query = $pStr['query'];
                break;

            case PLUGIN_EVENT_SEARCHHIGHLIGHT_MSN :
                $query = $pStr['q'];
                break;

            case PLUGIN_EVENT_SEARCHHIGHLIGHT_ALTAVISTA :
                $query = $pStr['q'];
                break;

            case PLUGIN_EVENT_SEARCHHIGHLIGHT_AOL_DE :
                $query = $pStr['q'];
                break;

            case PLUGIN_EVENT_SEARCHHIGHLIGHT_AOL_COM :
                $query = $pStr['query'];
                break;

            default:
                return false;
        }

        /* Clean the query */
        $query = trim($query);
        $query = preg_replace('/(\"|\')/i', '', $query);

        /* Split by search engine chars or spaces */
        $words = preg_split('/[\s\,\+\.\-\/\=]+/', $query);

        /* Strip search engine keywords or common words we don't bother to highlight */
        $words = array_diff($words, array('AND', 'OR', 'FROM', 'IN'));

        return $words;
    }


    function event_hook($event, &$bag, &$eventData) {
        global $serendipity;

        $this->uri = $_SERVER['HTTP_REFERER'];
        $hooks = &$bag->get('event_hooks');

        if (!isset($hooks[$event])) {
            return false;
        }

        if ( $event == 'frontend_display' ) {
            if ( ($queries = $this->getQuery()) === false ) {
                return;
            }

            foreach ($this->markup_elements as $temp) {
                if ( ! (serendipity_db_bool($this->get_config($temp['name'])) && isset($eventData[$temp['element']])) ) {
                    continue;
                }

                $element = &$eventData[$temp['element']];

                foreach ( $queries as $word ) {
                    /* If the data contains HTML tags, we have to be careful not to break URIs and use a more complex preg */
                    if ( preg_match('/\<.+\>/', $element) ) {
                        $_pattern =  '/(?!<.*?)(\b'. preg_quote($word, '/') .'\b)(?![^<>]*?>)/im';
                    } else {
                        $_pattern = '/(\b'. preg_quote($word, '/') .'\b)/im';
                    }
                    $element = preg_replace($_pattern, '<span class="serendipity_searchQuery">$1</span>', $element);
                } // end foreach
            } // end foreach
            return;
        } // end if


        if ( $event == 'css' ) {
            /* If the user hasn't added a CSS Class called serendipity_searchQuery, we add a pretty one for him */
            if ( strstr($eventData, '.serendipity_searchQuery') === false ) {
                $eventData .= "\n";
                $eventData .= '.serendipity_searchQuery {' . "\n";
                $eventData .= '    background-color: #D81F2A;' . "\n";
                $eventData .= '    color: #FFFFFF;' . "\n";
                $eventData .= '}' . "\n";
            }
            return;
        }

    } // end function
}

/* vim: set sts=4 ts=4 expandtab : */
?>