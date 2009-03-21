<?php
 
// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_ROSCMSACCOUNT_TITLE', 'Account');
@define('PLUGIN_ROSCMSACCOUNT_NAME', 'Roscms-account');
@define('PLUGIN_ROSCMSACCOUNT_DESC', 'Roscms global login system account management');
@define('PLUGIN_ROSCMSACCOUNT_LOGIN', 'Login');
@define('PLUGIN_ROSCMSACCOUNT_LOGOUT', 'Logout');
@define('PLUGIN_ROSCMSACCOUNT_REGISTER', 'Register');
@define('PLUGIN_ROSCMSACCOUNT_MANAGE', 'Manage blog entries');

require_once(ROOT_PATH . "roscms/logon/subsys_login.php");

class serendipity_plugin_roscmsaccount extends serendipity_plugin 
{
    function introspect(&$propbag) 
    {
        $propbag->add('name',          PLUGIN_ROSCMSACCOUNT_NAME);
        $propbag->add('description',   PLUGIN_ROSCMSACCOUNT_DESC);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Ge van Geldorp');
        $propbag->add('version',       '1.0');
        $propbag->add('groups', array('FRONTEND_FEATURES'));
    }

    function generate_content(&$title)
    {
        global $serendipity;

        $title = PLUGIN_ROSCMSACCOUNT_TITLE;
        $roscmsid = roscms_subsys_login('roscms', ROSCMS_LOGIN_OPTIONAL, '');
        if (0 == $roscmsid) {
            echo '<a href="/roscms/?page=login&target=' .
                 $serendipity['serendipityHTTPPath'] . '">' .
                 PLUGIN_ROSCMSACCOUNT_LOGIN . "</a><br />\n";
        } else {
            echo '<a href="/roscms/?page=logout&target=' .
                 $serendipity['serendipityHTTPPath'] . '">' .
                 PLUGIN_ROSCMSACCOUNT_LOGOUT;
            $who_query = "SELECT user_name " .
                         "  FROM roscms.users " .
                         " WHERE user_id = $roscmsid ";
            $row_who = serendipity_db_query($who_query, true);

            if (is_array($row_who)) {
                echo ' [' . $row_who['user_name'] . ']';
            }
            echo "</a><br />\n";
        }
        echo '<a href="/roscms/?page=register&target=' .
             $serendipity['serendipityHTTPPath'] . '">' .
             PLUGIN_ROSCMSACCOUNT_REGISTER . "</a><br />\n";
        $authorid = roscms_subsys_login('blogs', ROSCMS_LOGIN_OPTIONAL, '');
        if (0 != $authorid) {
            $base = $serendipity['serendipityHTTPPath'];

            $link = $serendipity['serendipityHTTPPath'] .
                    ($serendipity['rewrite'] == 'none' ?
                     $serendipity['indexFile'] .'?/' : '') . PATH_ADMIN;
            $text = PLUGIN_ROSCMSACCOUNT_MANAGE;
            echo '<a href="' . $link . '" title="'. $text .'">'. $text .
                 "</a><br />\n";
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
