<?php # Based on serendipity_plugin_authors.php
 
// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_ACTIVEAUTHORS_NAME', 'Active Authors');
@define('PLUGIN_ACTIVEAUTHORS_DESC', 'Displays active authors.');
@define('PLUGIN_ACTIVEAUTHORS_TITLE', 'Authors');
@define('PLUGIN_ACTIVEAUTHORS_ALL', 'All');

class serendipity_plugin_activeauthors extends serendipity_plugin 
{
    var $title = PLUGIN_ACTIVEAUTHORS_TITLE;

    function introspect(&$propbag) 
    {
        $propbag->add('name',          PLUGIN_ACTIVEAUTHORS_NAME);
        $propbag->add('description',   PLUGIN_ACTIVEAUTHORS_DESC);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Ge van Geldorp/Victor Fusco');
        $propbag->add('version',       '1.0');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('groups', array('FRONTEND_VIEWS'));
        $propbag->add('configuration', array('title'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'title':
                $propbag->add('type',          'string');
                $propbag->add('name',          TITLE);
                $propbag->add('description',   TITLE);
                $propbag->add('default', PLUGIN_ACTIVEAUTHORS_TITLE);
                break;
        }
        return true;
    }

    function generate_content(&$title) {
        global $serendipity;

    	$title = $this->get_config('title', $this->title);

        $alllink = $serendipity['serendipityHTTPPath'];
        echo '<a href="' . $alllink . '">' . PLUGIN_ACTIVEAUTHORS_ALL . "</a><br />\n";


        $authors_query = "SELECT DISTINCT a.realname, a.username, a.authorid " .
                         "  FROM {$serendipity['dbPrefix']}authors a, " .
                         "       {$serendipity['dbPrefix']}entries e " .
                         " WHERE e.authorid = a.authorid " .
                         "   AND e.isdraft = 'false' " .
                         " ORDER BY a.realname ";
        $row_authors = serendipity_db_query($authors_query);

        if (isset($row_authors) && is_array($row_authors)) {
            foreach ($row_authors as $entry) {
                if (function_exists('serendipity_authorURL')) {
                    $entryLink = serendipity_authorURL($entry);
                } else {
                	$entryLink = serendipity_rewriteURL(
                                   PATH_AUTHORS . '/' . 
                                   serendipity_makePermalink(
                                     PERM_AUTHORS, 
                                     array(
                                       'id'    => $entry['authorid'], 
                                       'title' => $entry['realname']
                                     )
                                   )
                                 );
                }        

                echo '<a href="' . $entryLink . '">' . $entry['realname'] .
                     "</a><br />\n";
            }
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
