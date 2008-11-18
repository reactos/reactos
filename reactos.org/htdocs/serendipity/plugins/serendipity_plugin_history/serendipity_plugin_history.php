<?php # $Id: serendipity_plugin_history.php 612 2005-10-26 19:07:25Z garvinhicking $

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_HISTORY_NAME', 'History');
@define('PLUGIN_HISTORY_DESC', 'Displays ancient entries of an adjustable age.');
@define('PLUGIN_HISTORY_MIN_AGE', 'Min age');
@define('PLUGIN_HISTORY_MIN_AGE_DESC', 'Minimum age of entries (in days).');
@define('PLUGIN_HISTORY_MAX_AGE', 'Max age');
@define('PLUGIN_HISTORY_MAX_AGE_DESC','Maximum age of entries (in days).');
@define('PLUGIN_HISTORY_MAX_ENTRIES', 'Maximum entries');
@define('PLUGIN_HISTORY_MAX_ENTRIES_DESC', 'Number of entries to display');
@define('PLUGIN_HISTORY_SHOWFULL', 'Full entries');
@define('PLUGIN_HISTORY_SHOWFULL_DESC', 'Display full entries instead of linked headlines.');
@define('PLUGIN_HISTORY_INTRO', 'Intro');
@define('PLUGIN_HISTORY_INTRO_DESC', 'A short intro like \'One year ago I said:\'.');
@define('PLUGIN_HISTORY_OUTRO', 'Outro');
@define('PLUGIN_HISTORY_OUTRO_DESC', 'A short outro like \'Nice, eh?\'.');
@define('PLUGIN_HISTORY_DISPLAYDATE', 'Display date');
@define('PLUGIN_HISTORY_DISPLAYDATE_DESC', 'Display the date of each entry?');
@define('PLUGIN_HISTORY_MAXLENGTH', 'Title-Length');
@define('PLUGIN_HISTORY_MAXLENGTH_DESC', 'After how many characters to cut the titles (0 for full titles)?');
@define('PLUGIN_HISTORY_SPECIALAGE', 'Ready-made age?');
@define('PLUGIN_HISTORY_SPECIALAGE_DESC', 'If you want to define your own timerange instead of a ready-made, select \'I\'ll define one\' here and adjust the two settings below.');
@define('PLUGIN_HISTORY_SPECIALAGE_YEAR', 'Display items of exactly one year ago');
@define('PLUGIN_HISTORY_CUSTOMAGE', 'Let me define the age');
@define('PLUGIN_HISTORY_OYA', 'One year ago');
@define('PLUGIN_HISTORY_MYSELF', 'I\'ll define one');

class serendipity_plugin_history extends serendipity_plugin
{
    var $title = PLUGIN_HISTORY_NAME;

    function introspect(&$propbag)
    {
        global $serendipity;

        $this->title = $this->get_config('title', $this->title);

        $propbag->add('name',          PLUGIN_HISTORY_NAME);
        $propbag->add('description',   PLUGIN_HISTORY_DESC);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Jannis Hermanns');
        $propbag->add('version',       '1.1');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('groups', array('FRONTEND_VIEWS'));
        $propbag->add('configuration', array('title',
                                             'intro',
                                             'outro',
                                             'maxlength',
                                             'specialage',
                                             'min_age',
                                             'max_age',
                                             'max_entries',
                                             'full',
                                             'amount',
                                             'displaydate',
                                             'dateformat'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'title':
                $propbag->add('type', 'string');
                $propbag->add('name', TITLE);
                $propbag->add('description', '');
                $propbag->add('default', PLUGIN_HISTORY_NAME);
            break;
            case 'intro':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_HISTORY_INTRO);
                $propbag->add('description', PLUGIN_HISTORY_INTRO_DESC);
                $propbag->add('default', '');
            break;
            case 'outro':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_HISTORY_OUTRO);
                $propbag->add('description', PLUGIN_HISTORY_OUTRO_DESC);
                $propbag->add('default', '');
            break;
            case 'maxlength':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_HISTORY_MAXLENGTH);
                $propbag->add('description', PLUGIN_HISTORY_MAXLENGTH_DESC);
                $propbag->add('default', 30);
            break;
            case 'specialage':
                $propbag->add('type', 'select');
                $propbag->add('name', PLUGIN_HISTORY_SPECIALAGE);
                $propbag->add('description', PLUGIN_HISTORY_SPECIALAGE_DESC);
                $propbag->add('default', 'year');
                $propbag->add('select_values', array('year'=>PLUGIN_HISTORY_OYA,'custom'=>PLUGIN_HISTORY_MYSELF));
            break;
            case 'min_age':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_HISTORY_MIN_AGE);
                $propbag->add('description', PLUGIN_HISTORY_MIN_AGE_DESC);
                $propbag->add('default', 365);
            break;
            case 'max_age':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_HISTORY_MAX_AGE);
                $propbag->add('description', PLUGIN_HISTORY_MAX_AGE_DESC);
                $propbag->add('default', 365);
            break;
            case 'max_entries':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_HISTORY_MAX_ENTRIES);
                $propbag->add('description', PLUGIN_HISTORY_MAX_ENTRIES_DESC);
                $propbag->add('default', 5);
            break;
            case 'full':
                $propbag->add('type',         'select');
                $propbag->add('name',         PLUGIN_HISTORY_SHOWFULL);
                $propbag->add('description',  PLUGIN_HISTORY_SHOWFULL_DESC);
                $propbag->add('select_values', array('false','true'));
                $propbag->add('default',     'false');
            break;
            case 'displaydate':
                $propbag->add('type',         'select');
                $propbag->add('name',         PLUGIN_HISTORY_DISPLAYDATE);
                $propbag->add('description',  PLUGIN_HISTORY_DISPLAYDATE_DESC);
                $propbag->add('select_values', array('false','true'));
                $propbag->add('default',     'true');
            break;
            case 'dateformat':
                $propbag->add('type', 'string');
                $propbag->add('name', GENERAL_PLUGIN_DATEFORMAT);
                $propbag->add('description', sprintf(GENERAL_PLUGIN_DATEFORMAT_BLAHBLAH, '%a, %d.%m.%Y %H:%M'));
                $propbag->add('default', '%a, %d.%m.%Y %H:%M');
            break;
            default:
                    return false;
        }
        return true;
    }

    function generate_content(&$title)
    {
     global $serendipity;
        $title       = $this->get_config('title', $this->title);
        $intro       = $this->get_config('intro');
        $outro       = $this->get_config('outro');
        $maxlength   = $this->get_config('maxlength');
        $max_entries = $this->get_config('max_entries');
        $min_age     = $this->get_config('min_age');
        $max_age     = $this->get_config('max_age');
        $specialage  = $this->get_config('specialage');
        $displaydate = $this->get_config('displaydate', 'true');
        $dateformat  = $this->get_config('dateformat');
        $full        = ($this->get_config('full', 'false') != 'true') ? false : true;

        if (!is_numeric($min_age) || $min_age < 0 || $specialage == 'year') {
            $min_age = 365 + date('L', serendipity_serverOffsetHour());
        }

        if (!is_numeric($max_age) || $max_age < 1 || $specialage == 'year') {
            $max_age = 365 + date('L', serendipity_serverOffsetHour());
        }

        if (!is_numeric($max_entries) || $max_entries < 1) {
            $max_entries = 5;
        }

        if (!is_numeric($maxlength) ||$maxlength <0)
            $maxlength = 30;

        if (strlen($dateformat) < 1) {
            $dateformat = '%a, %d.%m.%Y %H:%M';
        }

        $nowts = serendipity_serverOffsetHour();
        $maxts = mktime(23, 59, 59,  date('m', $nowts), date('d', $nowts), date('Y', $nowts));
        $mints = mktime(0, 0, 0, date('m', $nowts), date('d', $nowts), date('Y', $nowts));
        $e     = serendipity_fetchEntries(array(($mints-$max_age*86400),
                                            ($maxts-$min_age*86400)), $full, $max_entries);
        echo (empty($intro)) ? '' : "$intro<br />";

        if (!is_array($e)) {
            return false;
        }

        if ( ($e_c = count($e)) == 0 ) {
            return false;
        }

        for($x=0; $x < $e_c; $x++) {
            $url = serendipity_archiveURL($e[$x]['id'],
                                          $e[$x]['title'],
                                          'serendipityHTTPPath',
                                          true,
                                          array('timestamp' => $e[$x]['timestamp'])
            );
            $date = ($displaydate=='0') ? '' : serendipity_strftime($dateformat,$e[$x]['timestamp']);
            $t = ($maxlength==0 || strlen($e[$x]['title'])<=$maxlength) ?
                    $e[$x]['title'] :
                    (trim(serendipity_mb('substr', $e[$x]['title'], 0, $maxlength-3)).' [...]');
            echo $date . "<a href='$url' title='".str_replace("'", '`', $e[$x][title])."'>".$t."</a> " .
                 strip_tags($e[$x]['body']) . '<br />';
        }
        echo $outro;
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
