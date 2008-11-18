<?php # $Id: plugin_internal.inc.php 663 2005-11-07 16:56:02Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

class serendipity_calendar_plugin extends serendipity_plugin {
    var $title = CALENDAR;

    function introspect(&$propbag)
    {
        $propbag->add('name',        CALENDAR);
        $propbag->add('description', QUICKJUMP_CALENDAR);
        $propbag->add('configuration', array('beginningOfWeek', 'enableExtEvents'));
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '1.0');
        $propbag->add('groups',        array('FRONTEND_VIEWS'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'beginningOfWeek':
                $options = array();
                for ( $i = 1; $i <= 7; $i++ ) {
                    $options[] = serendipity_formatTime('%A', mktime(0,0,0,3,$i-1,2004));
                }

                $propbag->add('type',        'select');
                $propbag->add('select_values', $options);
                $propbag->add('name',        CALENDAR_BEGINNING_OF_WEEK);
                $propbag->add('description', CALENDAR_BOW_DESC);
                $propbag->add('default',     1);
                break;

            case 'enableExtEvents':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        CALENDAR_ENABLE_EXTERNAL_EVENTS);
                $propbag->add('description', CALENDAR_EXTEVENT_DESC);
                $propbag->add('default',     false);
                break;

            default:
                return false;
        }
        return true;
    }

    function generate_content(&$title)
    {
        global $serendipity;

        $title = $this->title;

        // Usage of serendipity_serverOffsetHour is as follow:
        // * Whenever a date to display needs to be set, apply the timezone offset
        // * Whenever we USE the date anywhere in the database, subtract the timezone offset
        // * Whenever we DISPLAY the date, we do not apply additional timezone addition to it.

        if (!isset($serendipity['GET']['calendarZoom'])) {
            if (!isset($serendipity['range'])) {
                $serendipity['GET']['calendarZoom'] = serendipity_serverOffsetHour(time());
            } else {
                $serendipity['GET']['calendarZoom'] = serendipity_serverOffsetHour($serendipity['range'][0]);
            }
        }


        $month = date('m', serendipity_serverOffsetHour($serendipity['GET']['calendarZoom'], true));
        $year  = date('Y', serendipity_serverOffsetHour($serendipity['GET']['calendarZoom'], true));

        $bow = (int)$this->get_config('beginningOfWeek', 1);
        // Check for faulty input, is so - run the default
        if ($bow > 6) {
            $bow = 1;
        }

        // Catch faulty month
        $month = (int)$month;
        if ($month < 1) {
            $month = 1;
        }

        switch($serendipity['calendar']) {
            default:
            case 'gregorian':
                // How many days does the month have?
                $ts              = strtotime($year . '-' . sprintf('%02d', $month) . '-01');
                $now             = serendipity_serverOffsetHour(time(), true);
                $nrOfDays        = date('t', $ts);
                $firstDayWeekDay = date('w', $ts);
                $firstts = $ts;
                $endts = mktime(0, 0, 0, $month + 1, 1, $year);
                
                break;
            
            case 'jalali-utf8':
                
                require_once S9Y_INCLUDE_PATH . 'include/functions_calendars.inc.php';
                
                list(,$jy, $jm, $jd) = $serendipity['uriArguments'];

                if( isset($jd) && $jd ) list ( $gy, $gm, $gd ) = j2g ($jy, $jm, $jd);
                else if( isset($jm) && $jm ) list ( $gy, $gm, $gd ) = j2g ( $jy, $jm, 1);
                else{
                    $gy = $year;
                    $gm = $month;
                    $gd = (int) date('d');
                }
                
                list ( $year, $month, $day ) = g2j ($gy, $gm, $gd);
                
                // How many days does the month have?
                $ts              = strtotime($gy . '-' . sprintf('%02d', $gm) . '-' . sprintf('%02d', $gd));
                $now             = serendipity_serverOffsetHour(time(), true);
                $nrOfDays = jalali_strftime_utf('%m', $ts);
                $j_days_in_month = array(0, 31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 29);
                if ($year%4 == 3 && $nrOfDays == 12) $nrOfDays = $j_days_in_month[(int)$nrOfDays]+1;
                else $nrOfDays = $j_days_in_month[(int)$nrOfDays];
                
                // Calculate first timestamp of the month
                list ($firstgy, $firstgm, $firstgd ) = j2g ( $year, $month, 1);
                $firstts = mktime (0, 0, 0, $firstgm, $firstgd, $firstgy);
                
                // Calculate first Jalali day, week day name
                $firstDayWeekDay = date('w', $firstts);
                
                // Calculate end timestamp of the month
                list ( $end_year, $end_month, $end_day ) = j2g ($year, $month+1, 1);
                $endts = mktime(0, 0, 0, $end_month, $end_day, $end_year);
                break;
        } // end switch
        
        // Calculate the first day of the week, based on the beginning of the week ($bow)
        if ($bow > $firstDayWeekDay) {
            $firstDayWeekDay = $firstDayWeekDay + 7 - $bow;
        } elseif ( $bow < $firstDayWeekDay ) {
            $firstDayWeekDay = $firstDayWeekDay - $bow;
        } else {
            $firstDayWeekDay = 0;
        }

        // Calculate the number of next/previous month
        if ($month > 1) {
            $previousMonth = $month-1;
            $previousYear  = $year;
        } else {
            $previousMonth = 12;
            $previousYear  = $year-1;
        }

        if ($month < 12) {
            $nextMonth = $month+1;
            $nextYear  = $year;
        } else {
            $nextMonth = 1;
            $nextYear  = $year+1;
        }
        
        // Get first and last entry
        $minmax = serendipity_db_query("SELECT MAX(timestamp) AS max, MIN(timestamp) AS min FROM {$serendipity['dbPrefix']}entries");
        if (!is_array($minmax) || !is_array($minmax[0]) || $minmax[0]['min'] < 1 || $minmax[0]['max'] < 1) {
            // If no entry is available yet, allow scrolling a year back and forth
            $minmax = array(
                        '0' => array(
                                 'min' => mktime(0, 0, 0, 1, 1, date('Y', $now) - 1),
                                 'max' => mktime(0, 0, 0, 1, 1, date('Y', $now) + 1)
                               )
                            );
        }

        // Find out about diary entries
        $add_query   = '';
        $base_query  = '';
        $cond = array();
        $cond['and']     = "WHERE e.timestamp  >= " . serendipity_serverOffsetHour($firstts, true) . "
                              AND e.timestamp  <= " . serendipity_serverOffsetHour($endts, true) . "
                                  " . (!serendipity_db_bool($serendipity['showFutureEntries']) ? " AND e.timestamp  <= " . time() : '') . "
                              AND e.isdraft     = 'false'";

        serendipity_plugin_api::hook_event('frontend_fetchentries', $cond, array('noCache' => false, 'noSticky' => false, 'source' => 'calendar'));

        if (isset($serendipity['GET']['category'])) {
            $base_query   = 'C' . (int)$serendipity['GET']['category'];
            $add_query    = '/' . $base_query;
            $querystring = "SELECT timestamp
                              FROM {$serendipity['dbPrefix']}category c,
                                   {$serendipity['dbPrefix']}entrycat ec,
                                   {$serendipity['dbPrefix']}entries e
                                   {$cond['joins']}
                                   {$cond['and']}
                               AND e.id          = ec.entryid
                               AND c.categoryid  = ec.categoryid
                               AND (" . serendipity_getMultiCategoriesSQL($serendipity['GET']['category']) . ")";
        }

        if (!isset($querystring)) {
            $querystring = "SELECT id, timestamp
                              FROM {$serendipity['dbPrefix']}entries e
                              {$cond['joins']}
                              {$cond['and']}";
        }

        $rows = serendipity_db_query($querystring);
        
        switch($serendipity['calendar']) {
            default:
            case 'gregorian':
                $activeDays = array();
                if (is_array($rows)) {
                    foreach ($rows as $row) {
                        $row['timestamp'] = serendipity_serverOffsetHour($row['timestamp']);
                        $activeDays[date('j', $row['timestamp'])] = $row['timestamp'];
                    }
                }
                $today_day   = date('j', $now);
                $today_month = date('m', $now);
                $today_year  = date('Y', $now);
                break;
            
            case 'jalali-utf8':
                $activeDays = array();
                if (is_array($rows)) {
                    foreach ($rows as $row) {
                        $row['timestamp'] = serendipity_serverOffsetHour($row['timestamp']);
                        $activeDays[(int) jalali_date_utf('j', $row['timestamp'])] = $row['timestamp'];
                    }
                }
                $today_day   = jalali_date_utf('j', $now);
                $today_month = jalali_date_utf('m', $now);
                $today_year  = jalali_date_utf('Y', $now);
                break;
            
        } // end switch
        
        $externalevents = array();
        if (serendipity_db_bool($this->get_config('enableExtEvents', false))) {
            serendipity_plugin_api::hook_event('frontend_calendar', $externalevents, array('Month' => $month, 'Year' => $year,
                                                                                           'TS' => $ts, 'EndTS' => $endts));
        }

        // Print the calendar
        $currDay     = 1;
        $nrOfRows    = ceil(($nrOfDays+$firstDayWeekDay)/7);
                
        for ($x = 0; $x < 6; $x++) {
            // Break out if we are out of days
            if ($currDay > $nrOfDays) {
                break;
            }
            // Prepare row
            for ($y = 0; $y < 7; $y++) {
                $cellProps = array();
                $printDay = '';
                $link = '';
    
                if ($x == 0) {
                    $cellProps['FirstRow'] = 1;
                }
                if ($y == 0) {
                    $cellProps['FirstInRow'] = 1;
                }
                if ($y == 6) {
                    $cellProps['LastInRow'] = 1;
                }
                if ($x == $nrOfRows-1) {
                    $cellProps['LastRow'] = 1;
                }
    
                // If it's not a blank day, we print the day
                if (($x > 0 || $y >= $firstDayWeekDay) && $currDay <= $nrOfDays) {
                    $printDay = $currDay;
    
                    if ($today_day == $currDay && $today_month == $month && $today_year == $year) {
                        $cellProps['Today'] = 1;
                    }

                    if (isset($externalevents[$currDay])) {
                        if (isset($externalevents[$currDay]['Class'])) {
                            $cellProps[$externalevents[$currDay]['Class']] = 1;
                        }
                        if (isset($externalevents[$currDay]['Title'])) {
                            $cellProps['Title'] = htmlspecialchars($externalevents[$currDay]['Title']);
                        }
                        if (isset($externalevents[$currDay]['Extended'])) {
                            foreach($externalevents[$currDay]['Extended'] as $ext_key => $ext_val) {
                                $cellProps[$ext_key] = $ext_val;
                            }
                        }
                    }

                    if (isset($activeDays[$currDay]) && $activeDays[$currDay] > 1) {
                        $cellProps['Active'] = 1;
                        $cellProps['Link']   = serendipity_archiveDateUrl(sprintf('%4d/%02d/%02d', $year, $month, $currDay) . $add_query );
                    }
                    $currDay++;
                }
                $smartyRows[$x]['days'][] = array('name' => $printDay,
                                                  'properties' => $cellProps,
                                                  'classes' => implode(' ', array_keys($cellProps)));
            } // end for
        } // end for
        
        $serendipity['smarty']->assign('plugin_calendar_weeks', $smartyRows);
        
        $dow = array();
        for ($i = 1; $i <= 7; $i++) {
            $dow[] = array('date' => mktime(0, 0, 0, 3, $bow + $i - 1, 2004));
        }
        $serendipity['smarty']->assign('plugin_calendar_dow', $dow);
        
        $serendipity['smarty']->assign('plugin_calendar_head', array('month_date'   => $ts,
            'uri_previous' => serendipity_archiveDateUrl(sprintf('%04d/%02d', $previousYear, $previousMonth). $add_query),
            'uri_month'    => serendipity_archiveDateUrl(sprintf('%04d/%02d', $year, $month) . $add_query),
            'uri_next'     => serendipity_archiveDateUrl(sprintf('%04d/%02d',$nextYear, $nextMonth) . $add_query),
            'minScroll'    => $minmax[0]['min'],
            'maxScroll'    => $minmax[0]['max']));
        echo serendipity_smarty_fetch('CALENDAR', 'plugin_calendar.tpl');
                
    } // end function
} // end class

class serendipity_quicksearch_plugin extends serendipity_plugin {
    var $title = QUICKSEARCH;

    function introspect(&$propbag)
    {
        $propbag->add('name',          QUICKSEARCH);
        $propbag->add('description',   SEARCH_FOR_ENTRY);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '1.0');
        $propbag->add('groups',        array('FRONTEND_ENTRY_RELATED'));
    }

    function generate_content(&$title)
    {
        global $serendipity;

        $title = $this->title;
?>
<form id="searchform" action="<?php echo $serendipity['serendipityHTTPPath'] . $serendipity['indexFile']; ?>" method="get">
    <div>
        <input type="hidden"  name="serendipity[action]" value="search" />
        <input alt="<?php echo QUICKSEARCH; ?>" type="text"   id="serendipityQuickSearchTermField" name="serendipity[searchTerm]" size="13" />
    </div>
    <div id="LSResult" style="display: none;"><div id="LSShadow"></div></div>
</form>
<?php
        serendipity_plugin_api::hook_event('quicksearch_plugin', $serendipity);
    }
}

class serendipity_archives_plugin extends serendipity_plugin {
    var $title = ARCHIVES;

    function introspect(&$propbag)
    {
        $propbag->add('name',          ARCHIVES);
        $propbag->add('description',   BROWSE_ARCHIVES);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '1.0');
        $propbag->add('configuration', array('frequency', 'count'));
        $propbag->add('groups',        array('FRONTEND_VIEWS'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'count' :
                $propbag->add('type', 'string');
                $propbag->add('name', ARCHIVE_COUNT);
                $propbag->add('description', ARCHIVE_COUNT_DESC);
                $propbag->add('default', 3);
                break;

            case 'frequency' :
                $propbag->add('type', 'select');
                $propbag->add('name', ARCHIVE_FREQUENCY);
                $propbag->add('select_values', array('months' => MONTHS, 'weeks' => WEEKS, 'days' => DAYS));
                $propbag->add('description', ARCHIVE_FREQUENCY_DESC);
                $propbag->add('default', 'months');
                break;

            default:
                return false;
        }
        return true;
    }

    function generate_content(&$title)
    {
        global $serendipity;

        $title = $this->title;

        $ts = mktime(0, 0, 0);

        require_once S9Y_INCLUDE_PATH . 'include/functions_calendars.inc.php';

        $add_query = '';
        if (isset($serendipity['GET']['category'])) {
            $base_query   = 'C' . (int)$serendipity['GET']['category'];
            $add_query    = '/' . $base_query;
        }

        $max_x = $this->get_config('count', 3);
        
        for($x = 0; $x < $max_x; $x++) {
            
            switch($this->get_config('frequency', 'months')) {
                case 'months' :
                    switch($serendipity['calendar']) {
                        default:
                        case 'gregorian':
                            $linkStamp = date('Y/m', $ts);
                            $ts_title = serendipity_formatTime("%B %Y", $ts, false);
                            $ts = mktime(0, 0, 0, date('m', $ts)-1, 1, date('Y', $ts)); // Must be last in 'case' statement
                            break;
                        case 'jalali-utf8':
                            $linkStamp = jalali_date_utf('Y/m', $ts);
                            $ts_title = serendipity_formatTime("%B %Y", $ts, false);
                            $ts = jalali_mktime(0, 0, 0, jalali_date_utf('m', $ts)-1, 1, jalali_date_utf('Y', $ts)); // Must be last in 'case' statement
                            break;
                    }
                    break;
                case 'weeks' :
                    switch($serendipity['calendar']) {
                        default:
                        case 'gregorian':
                            $linkStamp = date('Y/\WW', $ts);
                            $ts_title = WEEK . ' '. date('W, Y', $ts);
                            $ts = mktime(0, 0, 0, date('m', $ts), date('d', $ts)-7, date('Y', $ts));
                            break;
                        case 'jalali-utf8':
                            $linkStamp = jalali_date_utf('Y/\WW', $ts);
                            $ts_title = WEEK . ' '. jalali_date_utf('W، Y', $ts);
                            $ts = jalali_mktime(0, 0, 0, jalali_date_utf('m', $ts), jalali_date_utf('d', $ts)-7, jalali_date_utf('Y', $ts));
                            break;
                    }
                    break;
                case 'days' :
                    switch($serendipity['calendar']) {
                        default:
                        case 'gregorian':
                            $linkStamp = date('Y/m/d', $ts);
                            $ts_title = serendipity_formatTime("%B %e. %Y", $ts, false);
                            $ts = mktime(0, 0, 0, date('m', $ts), date('d', $ts)-1, date('Y', $ts)); // Must be last in 'case' statement
                            break;
                        case 'jalali-utf8':
                            $linkStamp = jalali_date_utf('Y/m/d', $ts);
                            $ts_title = serendipity_formatTime("%e %B %Y", $ts, false);
                            $ts = jalali_mktime(0, 0, 0, jalali_date_utf('m', $ts), jalali_date_utf('d', $ts)-1, jalali_date_utf('Y', $ts)); // Must be last in 'case' statement
                            break;
                    }
                    break;
            }
            $link = serendipity_rewriteURL(PATH_ARCHIVES . '/' . $linkStamp . $add_query . '.html', 'serendipityHTTPPath');

            echo '<a href="' . $link . '" title="' . $ts_title . '">' . $ts_title . '</a><br />' . "\n";

        }

        echo '<a href="'. $serendipity['serendipityHTTPPath'] .'">' . RECENT . '</a><br />' . "\n";
        echo '<a href="'. serendipity_rewriteURL(PATH_ARCHIVE . $add_query) .'">' . OLDER . '</a>'. "\n";
    }
}

class serendipity_topreferrers_plugin extends serendipity_plugin {
    var $title = TOP_REFERRER;

    function introspect(&$propbag)
    {
        $propbag->add('name',          TOP_REFERRER);
        $propbag->add('description',   SHOWS_TOP_SITES);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '1.0');
        $propbag->add('configuration', array('limit', 'use_links', 'interval'));
        $propbag->add('groups',        array('STATISTICS'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'limit':
                $propbag->add('type',        'string');
                $propbag->add('name',        LIMIT_TO_NUMBER);
                $propbag->add('description', LIMIT_TO_NUMBER);
                $propbag->add('default',     10);
                break;

	    case 'interval':
	        $propbag->add('type',        'string');
	        $propbag->add('name',        ARCHIVE_FREQUENCY);
		$propbag->add('description', ARCHIVE_FREQUENCY_DESC);
                $propbag->add('default',     7);
                break;
 
            case 'use_links':
                $propbag->add('type',        'tristate');
                $propbag->add('name',        INSTALL_TOP_AS_LINKS);
                $propbag->add('description', INSTALL_TOP_AS_LINKS_DESC);
                $propbag->add('default',     false);
                break;

            default:
                return false;
        }
        return true;
    }

    function generate_content(&$title)
    {
        global $serendipity;

        $title = $this->title;

        // get local configuration (default, true, false)
        $use_links = $this->get_config('use_links', 'default');
        // get global configuration (true, false)
        $global_use_link = serendipity_get_config_var('top_as_links', false, true);

        // if local configuration say to use global default, do so
        if ($use_links == 'default') {
            $use_links = serendipity_db_bool($global_use_link);
        } else {
            $use_links = serendipity_db_bool($use_links);
        }

        echo serendipity_displayTopReferrers($this->get_config('limit', 10), $use_links, $this->get_config('interval', 7));
    }
}

class serendipity_topexits_plugin extends serendipity_plugin {
    var $title = TOP_EXITS;

    function introspect(&$propbag)
    {
        $propbag->add('name',          TOP_EXITS);
        $propbag->add('description',   SHOWS_TOP_EXIT);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '1.0');
        $propbag->add('configuration', array('limit', 'use_links', 'interval'));
        $propbag->add('groups',        array('STATISTICS'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'limit':
                $propbag->add('type',        'string');
                $propbag->add('name',        LIMIT_TO_NUMBER);
                $propbag->add('description', LIMIT_TO_NUMBER);
                $propbag->add('default',     10);
                break;

            case 'interval':
                $propbag->add('type',        'string');
                $propbag->add('name',        ARCHIVE_FREQUENCY);
                $propbag->add('description', ARCHIVE_FREQUENCY_DESC);
                $propbag->add('default',     7);
                break;
                                                                            
            case 'use_links':
                $propbag->add('type',        'tristate');
                $propbag->add('name',        INSTALL_TOP_AS_LINKS);
                $propbag->add('description', INSTALL_TOP_AS_LINKS_DESC);
                $propbag->add('default',     true);
                break;

            default:
                return false;
        }
        return true;
    }

    function generate_content(&$title)
    {
        global $serendipity;

        $title = $this->title;

        // get local configuration (default, true, false)
        $use_links = $this->get_config('use_links', 'default');
        // get global configuration (true, false)
        $global_use_link = serendipity_get_config_var('top_as_links', false, true);

        // if local configuration say to use global default, do so
        if ($use_links == 'default') {
            $use_links = serendipity_db_bool($global_use_link);
        } else {
            $use_links = serendipity_db_bool($use_links);
        }

        echo serendipity_displayTopExits($this->get_config('limit', 10), $use_links, $this->get_config('interval', 7));
    }
}

class serendipity_syndication_plugin extends serendipity_plugin {
    var $title = SYNDICATE_THIS_BLOG;

    function introspect(&$propbag)
    {
        $propbag->add('name',          SYNDICATION);
        $propbag->add('description',   SHOWS_RSS_BLAHBLAH);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '1.0');
        $propbag->add('configuration', array(
                                        'fullfeed',
                                        'show_0.91',
                                        'show_1.0',
                                        'show_2.0',
                                        'show_2.0c',
                                        'show_atom0.3',
                                        'show_atom1.0',
                                        'show_opml1.0',
                                        'show_feedburner',
                                        'seperator',
                                        'show_mail',
                                        'field_managingEditor',
                                        'field_webMaster',
                                        'field_ttl',
                                        'field_pubDate',
                                        'seperator',
                                        'bannerURL',
                                        'bannerWidth',
                                        'bannerHeight',
                                        'seperator',
                                        'fb_id',
                                        'fb_title',
                                        'fb_alt',
                                        'fb_img',
                                       )
        );
        $propbag->add('groups',        array('FRONTEND_VIEWS'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'fullfeed':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        SYNDICATION_PLUGIN_FULLFEED);
                $propbag->add('description', '');
                $propbag->add('default',     false);
                break;

            case 'show_0.91':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        SYNDICATION_PLUGIN_091);
                $propbag->add('description', '');
                $propbag->add('default',     'true');
                break;

            case 'show_1.0':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        SYNDICATION_PLUGIN_10);
                $propbag->add('description', '');
                $propbag->add('default',     'true');
                break;

            case 'show_2.0':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        SYNDICATION_PLUGIN_20);
                $propbag->add('description', '');
                $propbag->add('default',     'true');
                break;

            case 'show_2.0c':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        SYNDICATION_PLUGIN_20c);
                $propbag->add('description', '');
                $propbag->add('default',     'true');
                break;

            case 'show_atom0.3':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        SYNDICATION_PLUGIN_ATOM03);
                $propbag->add('description', '');
                $propbag->add('default',     'false');
                break;

            case 'show_atom1.0':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        SYNDICATION_PLUGIN_ATOM10);
                $propbag->add('description', '');
                $propbag->add('default',     'true');
                break;

            case 'show_opml1.0':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        sprintf(SYNDICATION_PLUGIN_GENERIC_FEED, 'OPML 1.0'));
                $propbag->add('description', '');
                $propbag->add('default',     'false');
                break;

            case 'show_feedburner':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        sprintf(SYNDICATION_PLUGIN_GENERIC_FEED, 'FeedBurner'));
                $propbag->add('description', '');
                $propbag->add('default',     'false');
                break;

            case 'seperator':
                $propbag->add('type',        'seperator');
                break;

            case 'show_mail':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        SYNDICATION_PLUGIN_SHOW_MAIL);
                $propbag->add('description', '');
                $propbag->add('default',     false);
                break;

            case 'field_managingEditor':
                $propbag->add('type',        'string');
                $propbag->add('name',        SYNDICATION_PLUGIN_MANAGINGEDITOR);
                $propbag->add('description', SYNDICATION_PLUGIN_MANAGINGEDITOR_DESC);
                $propbag->add('default',     '');
                break;

            case 'field_webMaster':
                $propbag->add('type',        'string');
                $propbag->add('name',        SYNDICATION_PLUGIN_WEBMASTER);
                $propbag->add('description', SYNDICATION_PLUGIN_WEBMASTER_DESC);
                $propbag->add('default',     '');
                break;

            case 'field_ttl':
                $propbag->add('type',        'string');
                $propbag->add('name',        SYNDICATION_PLUGIN_TTL);
                $propbag->add('description', SYNDICATION_PLUGIN_TTL_DESC);
                $propbag->add('default',     '');
                break;

            case 'field_pubDate':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        SYNDICATION_PLUGIN_PUBDATE);
                $propbag->add('description', SYNDICATION_PLUGIN_PUBDATE_DESC);
                $propbag->add('default',     '');
                break;

            case 'bannerURL':
                $propbag->add('type',        'string');
                $propbag->add('name',        SYNDICATION_PLUGIN_BANNERURL);
                $propbag->add('description', SYNDICATION_PLUGIN_BANNERURL_DESC);
                $propbag->add('default',     '');
                break;

            case 'bannerWidth':
                $propbag->add('type',        'string');
                $propbag->add('name',        SYNDICATION_PLUGIN_BANNERWIDTH);
                $propbag->add('description', SYNDICATION_PLUGIN_BANNERWIDTH_DESC);
                $propbag->add('default',     '');
                break;

            case 'bannerHeight':
                $propbag->add('type',        'string');
                $propbag->add('name',        SYNDICATION_PLUGIN_BANNERHEIGHT);
                $propbag->add('description', SYNDICATION_PLUGIN_BANNERHEIGHT_DESC);
                $propbag->add('default',     '');
                break;

            case 'fb_id':
                $propbag->add('type',        'string');
                $propbag->add('name',        SYNDICATION_PLUGIN_FEEDBURNERID);
                $propbag->add('description', SYNDICATION_PLUGIN_FEEDBURNERID_DESC);
                $propbag->add('default',     '');
                break;

            case 'fb_img':
                $propbag->add('type',        'string');
                $propbag->add('name',        SYNDICATION_PLUGIN_FEEDBURNERIMG);
                $propbag->add('description', SYNDICATION_PLUGIN_FEEDBURNERIMG_DESC);
                $propbag->add('default',     'fbapix.gif');
                break;

            case 'fb_title':
                $propbag->add('type',        'string');
                $propbag->add('name',        SYNDICATION_PLUGIN_FEEDBURNERTITLE);
                $propbag->add('description', SYNDICATION_PLUGIN_FEEDBURNERTITLE_DESC);
                $propbag->add('default',     '');
                break;

            case 'fb_alt':
                $propbag->add('type',        'string');
                $propbag->add('name',        SYNDICATION_PLUGIN_FEEDBURNERALT);
                $propbag->add('description', SYNDICATION_PLUGIN_FEEDBURNERALT_DESC);
                $propbag->add('default',     '');
                break;


            default:
                return false;
        }
        return true;
    }

    function generate_content(&$title)
    {
        global $serendipity;

        $title = $this->title;

        if (serendipity_db_bool($this->get_config('show_0.91', true))) {
?>
        <div style="padding-bottom: 2px;">
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/index.rss', 'serendipityHTTPPath') ?>"><img src="<?php echo serendipity_getTemplateFile('img/xml.gif'); ?>" alt="XML" style="border: 0px" /></a>
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/index.rss', 'serendipityHTTPPath') ?>">RSS 0.91 feed</a>
        </div>
<?php
        }

        if (serendipity_db_bool($this->get_config('show_1.0', true))) {
?>
        <div style="padding-bottom: 2px;">
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/index.rss1', 'serendipityHTTPPath') ?>"><img src="<?php echo serendipity_getTemplateFile('img/xml.gif'); ?>" alt="XML" style="border: 0px" /></a>
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/index.rss1', 'serendipityHTTPPath') ?>">RSS 1.0 feed</a>
        </div>
<?php
        }

        if (serendipity_db_bool($this->get_config('show_2.0', true))) {
?>
        <div style="padding-bottom: 2px;">
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/index.rss2', 'serendipityHTTPPath') ?>"><img src="<?php echo serendipity_getTemplateFile('img/xml.gif'); ?>" alt="XML" style="border: 0px" /></a>
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/index.rss2', 'serendipityHTTPPath') ?>">RSS 2.0 feed</a>
        </div>
<?php
        }

        if (serendipity_db_bool($this->get_config('show_atom0.3', true))) {
?>
        <div style="padding-bottom: 2px;">
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/atom03.xml', 'serendipityHTTPPath') ?>"><img src="<?php echo serendipity_getTemplateFile('img/xml.gif'); ?>" alt="ATOM/XML" style="border: 0px" /></a>
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/atom03.xml', 'serendipityHTTPPath') ?>">ATOM 0.3 feed</a>
        </div>
<?php
        }

        if (serendipity_db_bool($this->get_config('show_atom1.0', true))) {
?>
        <div style="padding-bottom: 2px;">
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/atom10.xml', 'serendipityHTTPPath') ?>"><img src="<?php echo serendipity_getTemplateFile('img/xml.gif'); ?>" alt="ATOM/XML" style="border: 0px" /></a>
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/atom10.xml', 'serendipityHTTPPath') ?>">ATOM 1.0 feed</a>
        </div>
<?php
        }

        if (serendipity_db_bool($this->get_config('show_2.0c', true))) {
?>
        <div style="padding-bottom: 2px;">
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/comments.rss2', 'serendipityHTTPPath') ?>"><img src="<?php echo serendipity_getTemplateFile('img/xml.gif'); ?>" alt="XML" style="border: 0px" /></a>
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/comments.rss2', 'serendipityHTTPPath') ?>"><?php echo ($serendipity['XHTML11'] ? '<span style="white-space: nowrap">' : '<nobr>'); ?>RSS 2.0 <?php echo COMMENTS; ?><?php echo ($serendipity['XHTML11'] ? '</span>' : '</nobr>'); ?></a>
        </div>
<?php
        }

        if (serendipity_db_bool($this->get_config('show_opml1.0', false))) {
?>
        <div style="padding-bottom: 2px;">
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/opml.xml', 'serendipityHTTPPath') ?>"><img src="<?php echo serendipity_getTemplateFile('img/xml.gif'); ?>" alt="XML" style="border: 0px" /></a>
            <a href="<?php echo serendipity_rewriteURL(PATH_FEEDS .'/opml.xml', 'serendipityHTTPPath') ?>">OPML 1.0 feed</a>
        </div>
<?php
        }

        if (serendipity_db_bool($this->get_config('show_feedburner', false))) {
			$alt = $this->get_config('fb_alt');
			$url = 'http://feeds.feedburner.com/' . $this->get_config('fb_id');
			$img = $this->get_config('fb_img');
			if (strlen($img) == 0) {
				$img = 'http://feeds.feedburner.com/~fc/'.$this->get_config('fb_id').'?bg=99CCFF&amp;fg=444444&amp;anim=0';
			} else {
				$img = 'http://www.feedburner.com/fb/images/pub/'.$img;
			}
?>
        <div style="padding-bottom: 2px;">
			<a href="<?php echo $url; ?>"<?php if (strlen($alt) > 0) echo " title=\"$alt\""; ?>><img src="<?php echo $img; ?>" alt="" style="border:0"/></a>
            <?php
			$mytitle = $this->get_config('fb_title');
			if (strlen($mytitle) > 0) { ?>
			<a href="<?php echo $url; ?>"><?php echo $mytitle; ?></a>
            <?php } ?>
		</div>
<?php
		}
    }

    function generate_rss_fields(&$title, &$description, &$entries) {
        global $serendipity;
        // Check for a logo to use for an RSS feed. Can either be set by configuring the
        // syndication plugin OR by just placing a rss_banner.png file in the template directory.
        // If both is not set, we will display our happy own branding. :-)

        $bag    = new serendipity_property_bag;
        $this->introspect($bag);
        $additional_fields = array();

        if ($this->get_config('bannerURL') != '') {
            $img   = $this->get_config('bannerURL');
            $w     = $this->get_config('bannerWidth');
            $h     = $this->get_config('bannerHeight');
        } elseif (($banner = serendipity_getTemplateFile('img/rss_banner.png', 'serendipityPath'))) {
            $img = serendipity_getTemplateFile('img/rss_banner.png', 'baseURL');
            $i = getimagesize($banner);
            $w = $i[0];
            $h = $i[1];
        } else {
            $img = serendipity_getTemplateFile('img/s9y_banner_small.png', 'baseURL');
            $w = 100;
            $h = 21;
        }

        $additional_fields['image'] = <<<IMAGE
<image>
        <url>$img</url>
        <title>RSS: $title - $description</title>
        <link>{$serendipity['baseURL']}</link>
        <width>$w</width>
        <height>$h</height>
    </image>
IMAGE;

        $additional_fields['image_atom1.0'] = <<<IMAGE
<icon>$img</icon>
IMAGE;

        $additional_fields['image_rss1.0_channel'] = '<image rdf:resource="' . $img . '" />';
        $additional_fields['image_rss1.0_rdf'] = <<<IMAGE
<image rdf:about="$img">
        <url>$img</url>
        <title>RSS: $title - $description</title>
        <link>{$serendipity['baseURL']}</link>
        <width>$w</width>
        <height>$h</height>
    </image>
IMAGE;

        // Now, if set, stitch together any fields that have been configured in the syndication plugin.
        // First, do some sanity checks
        $additional_fields['channel'] = '';
        foreach($bag->get('configuration') AS $bag_index => $bag_value) {
            if (preg_match('|^field_(.*)$|', $bag_value, $match)) {
                $bag_content = $this->get_config($bag_value);

                switch($match[1]) {
                    case 'pubDate':
                        if ($bag_content != 'false') {
                            $bag_content = gmdate('D, d M Y H:i:s \G\M\T', serendipity_serverOffsetHour($entries[0]['last_modified']));
                        } else {
                            $bag_content = '';
                        }
                        break;

                    // Each new RSS-field which needs rewrite of its content should get its own case here.

                    default:
                        break;
                }

                if ($bag_content != '') {
                    $additional_fields['channel'] .= '<' . $match[1] . '>' . $bag_content . '</' . $match[1] . '>' . "\n";
                }

            }
        }

        return $additional_fields;
    }
}

class serendipity_superuser_plugin extends serendipity_plugin {
    var $title = SUPERUSER;

    function introspect(&$propbag)
    {
        $propbag->add('name',          SUPERUSER);
        $propbag->add('description',   ALLOWS_YOU_BLAHBLAH);
        $propbag->add('stackable',     false);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '1.0');
        $propbag->add('configuration', array('https'));
        $propbag->add('groups',        array('FRONTEND_FEATURES'));
    }

    function generate_content(&$title)
    {
        global $serendipity;

        $title = $this->title;
        if ($this->get_config('https', 'false') == 'true') {
            $base = str_replace('http://', 'https://', $serendipity['baseURL']);
        } else {
            $base = $serendipity['serendipityHTTPPath'];
        }

        $link = $base . ($serendipity['rewrite'] == 'none' ? $serendipity['indexFile'] .'?/' : '') . PATH_ADMIN;
        $text = (($_SESSION['serendipityAuthedUser'] === true) ? SUPERUSER_OPEN_ADMIN : SUPERUSER_OPEN_LOGIN);
        echo '<a href="' . $link . '" title="'. $text .'">'. $text .'</a>';
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'https':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        PLUGIN_SUPERUSER_HTTPS);
                $propbag->add('description', PLUGIN_SUPERUSER_HTTPS_DESC);
                $propbag->add('default',     'false');
                break;

            default:
                return false;
        }
        return true;
    }
}

class serendipity_plug_plugin extends serendipity_plugin {
    var $title = POWERED_BY;

    function introspect(&$propbag)
    {
        $propbag->add('name',          POWERED_BY);
        $propbag->add('description',   ADVERTISES_BLAHBLAH);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '1.0');
        $propbag->add('configuration', array(
                                        'image',
                                        'text'));
        $propbag->add('groups',        array('FRONTEND_VIEWS'));
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'image':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        POWERED_BY_SHOW_IMAGE);
                $propbag->add('description', POWERED_BY_SHOW_IMAGE_DESC);
                $propbag->add('default',     'true');
                break;

            case 'text':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        POWERED_BY_SHOW_TEXT);
                $propbag->add('description', POWERED_BY_SHOW_TEXT_DESC);
                $propbag->add('default',     'false');
                break;
            default:
                return false;
        }
        return true;
    }

    function generate_content(&$title)
    {
        global $serendipity;

        $title = $this->title;
?>
<div class="serendipityPlug">
<?php if ( $this->get_config('image', 'true') == 'true' ) { ?>
    <a title="<?php echo $title ?> Serendipity" href="http://www.s9y.org/"><img src="<?php echo serendipity_getTemplateFile('img/s9y_banner_small.png') ?>" alt="Serendipity PHP Weblog" style="border: 0px" /></a>
<?php } ?>
<?php if ( $this->get_config('text', 'false') == 'true' ) { ?>
    <div>
        <a title="<?php echo $title ?> Serendipity" href="http://www.s9y.org/">Serendipity PHP Weblog</a>
    </div>
<?php } ?>
</div>
<?php
    }
}

class serendipity_html_nugget_plugin extends serendipity_plugin {
    var $title = HTML_NUGGET;

    function introspect(&$propbag)
    {
        $this->title = $this->get_config('title', $this->title);
        $subtitle    = $this->get_config('backend_title', '');
        if (!empty($subtitle)) {
            $desc    = '(' . $subtitle . ') ' . HOLDS_A_BLAHBLAH;
        } else {
            $desc        = HOLDS_A_BLAHBLAH;
        }

        $propbag->add('name',          HTML_NUGGET);
        $propbag->add('description',   $desc);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '1.0');
        $propbag->add('configuration', array(
                                        'title',
                                        'backend_title',
                                        'content',
                                        'markup',
                                        'show_where'
                                       )
        );
        $propbag->add('groups',        array('FRONTEND_VIEWS'));

        $this->protected = TRUE; // If set to TRUE, only allows the owner of the plugin to modify its configuration
    }

    function introspect_config_item($name, &$propbag)
    {
        switch($name) {
            case 'title':
                $propbag->add('type',        'string');
                $propbag->add('name',        TITLE);
                $propbag->add('description', TITLE_FOR_NUGGET);
                $propbag->add('default',     '');
                break;

            case 'backend_title':
                $propbag->add('type',        'string');
                $propbag->add('name',        BACKEND_TITLE);
                $propbag->add('description', BACKEND_TITLE_FOR_NUGGET);
                $propbag->add('default',     '');
                break;

            case 'content':
                $propbag->add('type',        'html');
                $propbag->add('name',        CONTENT);
                $propbag->add('description', THE_NUGGET);
                $propbag->add('default',     '');
                break;

            case 'markup':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        DO_MARKUP);
                $propbag->add('description', DO_MARKUP_DESCRIPTION);
                $propbag->add('default',     'true');
                break;

            case 'show_where':
                $select = array('extended' => PLUGIN_ITEM_DISPLAY_EXTENDED, 'overview' => PLUGIN_ITEM_DISPLAY_OVERVIEW, 'both' => PLUGIN_ITEM_DISPLAY_BOTH);
                $propbag->add('type',        'select');
                $propbag->add('select_values', $select);
                $propbag->add('name',        PLUGIN_ITEM_DISPLAY);
                $propbag->add('description', '');
                $propbag->add('default',     'both');
                break;

            default:
                return false;
        }
        return true;
    }

    function generate_content(&$title)
    {
        global $serendipity;

        $title = $this->get_config('title');
        $show_where = $this->get_config('show_where', 'both');

        if ($show_where == 'extended' && (!isset($serendipity['GET']['id']) || !is_numeric($serendipity['GET']['id']))) {
            return false;
        } else if ($show_where == 'overview' && isset($serendipity['GET']['id']) && is_numeric($serendipity['GET']['id'])) {
            return false;
        }

        if ($this->get_config('markup', 'true') == 'true') {
            $entry = array('html_nugget' => $this->get_config('content'));
            serendipity_plugin_api::hook_event('frontend_display', $entry);
            echo $entry['html_nugget'];
        } else {
            echo $this->get_config('content');
        }
    }
}

class serendipity_categories_plugin extends serendipity_plugin {
    var $title = CATEGORIES;

    function introspect(&$propbag) {
        global $serendipity;

        $propbag->add('name',        CATEGORIES);
        $propbag->add('description', CATEGORY_PLUGIN_DESC);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '2.0');
        $propbag->add('configuration', array('authorid', 'parent_base', 'image', 'sort_order', 'sort_method', 'allow_select', 'hide_parallel', 'show_count', 'smarty'));
        $propbag->add('groups',        array('FRONTEND_VIEWS'));
    }

    function introspect_config_item($name, &$propbag)
    {
        global $serendipity;
        switch($name) {
            case 'authorid':
                $row_authors = serendipity_db_query("SELECT realname, authorid FROM {$serendipity['dbPrefix']}authors");
                $authors     = array('all' => ALL_AUTHORS);
                if (is_array($row_authors)) {
                    foreach($row_authors as $row) {
                        $authors[$row['authorid']] = $row['realname'];
                    }
                }

                $propbag->add('type',         'select');
                $propbag->add('name',         CATEGORIES_TO_FETCH);
                $propbag->add('description',  CATEGORIES_TO_FETCH_DESC);
                $propbag->add('select_values', $authors);
                $propbag->add('default',     'all');
                break;

            case 'parent_base':
                $categories = array('all' => ALL_CATEGORIES);
                $cats       = serendipity_fetchCategories(); 
                
                if (is_array($cats)) {
                    $cats = serendipity_walkRecursive($cats, 'categoryid', 'parentid', VIEWMODE_THREADED);
                    foreach($cats as $cat) {
                        $categories[$cat['categoryid']] = str_repeat(' . ', $cat['depth']) . $cat['category_name'];
                    }
                }

                $propbag->add('type',         'select');
                $propbag->add('name',         CATEGORIES_PARENT_BASE);
                $propbag->add('description',  CATEGORIES_PARENT_BASE_DESC);
                $propbag->add('select_values', $categories);
                $propbag->add('default',      'all');
                break;

            case 'hide_parallel':
                $propbag->add('type',         'boolean');
                $propbag->add('name',         CATEGORIES_HIDE_PARALLEL);
                $propbag->add('description',  CATEGORIES_HIDE_PARALLEL_DESC);
                $propbag->add('default',      false);
                break;

            case 'allow_select':
                $propbag->add('type',         'boolean');
                $propbag->add('name',         CATEGORIES_ALLOW_SELECT);
                $propbag->add('description',  CATEGORIES_ALLOW_SELECT_DESC);
                $propbag->add('default',      true);
                break;

            case 'sort_order':
                $select = array();
                $select['category_name']        = CATEGORY;
                $select['category_description'] = DESCRIPTION;
                $select['categoryid']           = 'ID';
                $select['none']                 = NONE;
                $propbag->add('type',         'select');
                $propbag->add('name',         SORT_ORDER);
                $propbag->add('description',  '');
                $propbag->add('select_values', $select);
                $propbag->add('default',     'category_name');
                break;

            case 'sort_method':
                $select = array();
                $select['ASC'] = SORT_ORDER_ASC;
                $select['DESC'] = SORT_ORDER_DESC;
                $propbag->add('type',         'select');
                $propbag->add('name',         SORT_ORDER);
                $propbag->add('description',  '');
                $propbag->add('select_values', $select);
                $propbag->add('default',     'ASC');
                break;

            case 'image':
                $propbag->add('type',         'string');
                $propbag->add('name',         XML_IMAGE_TO_DISPLAY);
                $propbag->add('description',  XML_IMAGE_TO_DISPLAY_DESC);
                $propbag->add('default',     serendipity_getTemplateFile('img/xml.gif'));
                break;

            case 'smarty':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        CATEGORY_PLUGIN_TEMPLATE);
                $propbag->add('description', CATEGORY_PLUGIN_TEMPLATE_DESC);
                $propbag->add('default',     false);
                break;

            case 'show_count':
                $propbag->add('type',        'boolean');
                $propbag->add('name',        CATEGORY_PLUGIN_SHOWCOUNT);
                $propbag->add('description', '');
                $propbag->add('default',     false);
                break;

            default:
                return false;
        }
        return true;
    }

    function generate_content(&$title) {
        global $serendipity;

        $smarty = serendipity_db_bool($this->get_config('smarty', false));

        $which_category = $this->get_config('authorid');
        $sort = $this->get_config('sort_order');
        if ($sort == 'none') {
            $sort = '';
        } else {
            $sort .= ' ' . $this->get_config('sort_method');
        }
        $is_form = serendipity_db_bool($this->get_config('allow_select'));
        $categories = serendipity_fetchCategories(empty($which_category) ? 'all' : $which_category, '', $sort);
        $title = $this->title;

        $cat_count = array();
        if (serendipity_db_bool($this->get_config('show_count'))) {
            $cat_sql        = "SELECT c.categoryid, c.category_name, count(e.id) as postings
                                            FROM {$serendipity['dbPrefix']}entrycat ec,
                                                 {$serendipity['dbPrefix']}category c,
                                                 {$serendipity['dbPrefix']}entries e
                                            WHERE ec.categoryid = c.categoryid AND ec.entryid = e.id AND e.isdraft = 'false'
                                            GROUP BY c.categoryid, c.category_name
                                            ORDER BY postings DESC";
            $category_rows  = serendipity_db_query($cat_sql);
            if (is_array($category_rows)) {
                foreach($category_rows AS $cat) {
                    $cat_count[$cat['categoryid']] = $cat['postings'];
                }
            }
            
        }

        $html       = '';

        if (!$smarty && $is_form) {
            $html .= '<form action="' . $serendipity['baseURL'] . $serendipity['indexFile'] . '" method="post"><div>';
        }

        $image = $this->get_config('image', serendipity_getTemplateFile('img/xml.gif'));
        $image = (($image == "'none'" || $image == 'none') ? '' : $image);

        $use_parent  = $this->get_config('parent_base');
        $parentdepth = 0;

        $hide_parallel = serendipity_db_bool($this->get_config('hide_parallel'));
        $hidedepth     = 0;

        if (is_array($categories) && count($categories)) {
            $categories = serendipity_walkRecursive($categories, 'categoryid', 'parentid', VIEWMODE_THREADED);
            foreach ($categories as $cid => $cat) {
                // Hide parents not wanted
                if ($use_parent && $use_parent != 'all') {
                    if ($parentdepth == 0 && $cat['parentid'] != $use_parent && $cat['categoryid'] != $use_parent) {
                        continue;
                    } else {
                        if ($cat['depth'] < $parentdepth) {
                            $parentdepth = 0;
                            continue;
                        }

                        if ($parentdepth == 0) {
                            $parentdepth = $cat['depth'];
                        }
                    }
                }
                
                // Hide parents outside of our tree
                if ($hide_parallel && $serendipity['GET']['category']) {
                    if ($hidedepth == 0 && $cat['parentid'] != $serendipity['GET']['category'] && $cat['categoryid'] != $serendipity['GET']['category']) {
                        continue;
                    } else {
                        if ($cat['depth'] < $hidedepth) {
                            $hidedepth = 0;
                            continue;
                        }

                        if ($hidedepth == 0) {
                            $hidedepth = $cat['depth'];
                        }
                    }
                }

                $categories[$cid]['feedCategoryURL'] = serendipity_feedCategoryURL($cat, 'serendipityHTTPPath');
                $categories[$cid]['categoryURL']     = serendipity_categoryURL($cat, 'serendipityHTTPPath');
                $categories[$cid]['paddingPx']       = $cat['depth']*6;
                
                if (!empty($cat_count[$cat['categoryid']])) {
                    $categories[$cid]['true_category_name'] = $cat['category_name'];
                    $categories[$cid]['category_name'] .= ' (' . $cat_count[$cat['categoryid']] . ')';
                }

                if (!$smarty) {
                    $html .= '<div style="padding-bottom: 2px;">';
    
                    if ($is_form) {
                        $html .= '<input style="width: 15px" type="checkbox" name="serendipity[multiCat][]" value="' . $cat['categoryid'] . '" />';
                    }
    
                    if ( !empty($image) ) {
                        $html .= '<a href="'. $categories[$cid]['feedCategoryURL'] .'"><img src="'. $image .'" alt="XML" style="border: 0px" /></a> ';
                    }
                    $html .= '<a href="'. $categories[$cid]['categoryURL'] .'" title="'. htmlspecialchars($cat['category_description']) .'" style="padding-left: '. $categories[$cid]['paddingPx'] .'px">'. htmlspecialchars($categories[$cid]['category_name']) .'</a>';
                    $html .= '</div>' . "\n";
                }
            }
        }

        if (!$smarty && $is_form) {
            $html .= '<br /><input type="submit" name="serendipity[isMultiCat]" value="' . GO . '" /><br />';
        }

        if (!$smarty) {
            $html .= sprintf(
                '<br /><a href="%s" title="%s">%s</a>',
    
                $serendipity['serendipityHTTPPath'] . $serendipity['indexFile'],
                ALL_CATEGORIES,
                ALL_CATEGORIES
            );
        }

        if (!$smarty && $is_form) {
            $html .= '</div></form>';
        }

        if (!$smarty) {
            echo $html;
        } else {
            $serendipity['smarty']->assign(array(
                'is_form'           => $is_form,
                'category_image'    => $image,
                'form_url'          => $serendipity['baseURL'] . $serendipity['indexFile'],
                'categories'        => is_array($categories) ? $categories : array()
            ));
            echo serendipity_smarty_fetch('CATEGORIES', 'plugin_categories.tpl');
        }
    }
}

class serendipity_authors_plugin extends serendipity_plugin {
    var $title = AUTHORS;

    function introspect(&$propbag) {
        global $serendipity;

        $propbag->add('name',        AUTHORS);
        $propbag->add('description', AUTHOR_PLUGIN_DESC);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Serendipity Team');
        $propbag->add('version',       '2.0');
        $propbag->add('configuration', array('image', 'allow_select', 'title', 'showartcount'));
        $propbag->add('groups',        array('FRONTEND_VIEWS'));
    }

    function introspect_config_item($name, &$propbag)
    {
        global $serendipity;
        switch($name) {
            case 'title':
                $propbag->add('type',          'string');
                $propbag->add('name',          TITLE);
                $propbag->add('description',   TITLE);
                $propbag->add('default',       AUTHORS);
                break;

            case 'allow_select':
                $propbag->add('type',         'boolean');
                $propbag->add('name',         AUTHORS_ALLOW_SELECT);
                $propbag->add('description',  AUTHORS_ALLOW_SELECT_DESC);
                $propbag->add('default',      true);
                break;

            case 'image':
                $propbag->add('type',         'string');
                $propbag->add('name',         XML_IMAGE_TO_DISPLAY);
                $propbag->add('description',  XML_IMAGE_TO_DISPLAY_DESC);
                $propbag->add('default',     serendipity_getTemplateFile('img/xml.gif'));
                break;

            case 'showartcount':
                $propbag->add('type',         'boolean');
                $propbag->add('name',         AUTHORS_SHOW_ARTICLE_COUNT);
                $propbag->add('description',  AUTHORS_SHOW_ARTICLE_COUNT_DESC);
                $propbag->add('default',      false);
                break;

            default:
                return false;
        }
        return true;
    }

    function generate_content(&$title) {
        global $serendipity;

    	$title = $this->get_config('title', $this->title);

        $sort = $this->get_config('sort_order');
        if ($sort == 'none') {
            $sort = '';
        } else {
            $sort .= ' ' . $this->get_config('sort_method');
        }
        $is_form  = serendipity_db_bool($this->get_config('allow_select'));
        $is_count = serendipity_db_bool($this->get_config('showartcount'));
        $authors  = serendipity_fetchUsers(null, null, $is_count);

        $html       = '';

        if ($is_form) {
            $html .= '<form action="' . $serendipity['baseURL'] . $serendipity['indexFile'] . '" method="post"><div>';
        }

        $image = $this->get_config('image', serendipity_getTemplateFile('img/xml.gif'));
        $image = (($image == "'none'" || $image == 'none') ? '' : $image);

        if (is_array($authors) && count($authors)) {
            foreach ($authors as $auth) {

                if ($is_count) {
                    $entrycount = " ({$auth['artcount']})";
                } else {
                    $entrycount = "";
                }

                $html .= '<div style="padding-bottom: 2px;">';

                if ($is_form) {
                    $html .= '<input style="width: 15px" type="checkbox" name="serendipity[multiAuth][]" value="' . $auth['authorid'] . '" />';
                }

                if ( !empty($image) ) {
                    $html .= '<a href="'. serendipity_feedAuthorURL($auth, 'serendipityHTTPPath') .'"><img src="'. $image .'" alt="XML" style="border: 0px" /></a> ';
                }
                $html .= '<a href="'. serendipity_authorURL($auth, 'serendipityHTTPPath') .'" title="'. htmlspecialchars($auth['realname']) .'">'. htmlspecialchars($auth['realname']) . $entrycount . '</a>';
                $html .= '</div>' . "\n";
            }
        }

        if ($is_form) {
            $html .= '<br /><input type="submit" name="serendipity[isMultiAuth]" value="' . GO . '" /><br />';
        }

        $html .= sprintf(
            '<br /><a href="%s" title="%s">%s</a>',

            $serendipity['serendipityHTTPPath'] . $serendipity['indexFile'],
            ALL_AUTHORS,
            ALL_AUTHORS
        );

        if ($is_form) {
            $html .= '</div></form>';
        }
        print $html;
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
