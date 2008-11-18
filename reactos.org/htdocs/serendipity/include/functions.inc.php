<?php # $Id: functions.inc.php 724 2005-11-23 11:30:28Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

$serendipity['imageList'] = array();
include_once(S9Y_INCLUDE_PATH . "include/db/db.inc.php");
include_once(S9Y_INCLUDE_PATH . "include/compat.inc.php");
include_once(S9Y_INCLUDE_PATH . "include/functions_config.inc.php");
include_once(S9Y_INCLUDE_PATH . "include/plugin_api.inc.php");
include_once(S9Y_INCLUDE_PATH . "include/functions_images.inc.php");
include_once(S9Y_INCLUDE_PATH . "include/functions_installer.inc.php");
include_once(S9Y_INCLUDE_PATH . "include/functions_entries.inc.php");
include_once(S9Y_INCLUDE_PATH . "include/functions_comments.inc.php");
include_once(S9Y_INCLUDE_PATH . "include/functions_permalinks.inc.php");
include_once(S9Y_INCLUDE_PATH . "include/functions_smarty.inc.php");

function serendipity_truncateString($s, $len) {
    if ( strlen($s) > ($len+3) ) {
        $s = serendipity_mb('substr', $s, 0, $len) . '...';
    }
    return $s;
}

function serendipity_gzCompression() {
    global $serendipity;
    if (isset($serendipity['useGzip']) && serendipity_db_bool($serendipity['useGzip']) && function_exists('ob_gzhandler') && extension_loaded('zlib') && serendipity_ini_bool(ini_get('zlib.output_compression')) == false && serendipity_ini_bool(ini_get('session.use_trans_sid')) == false) {
        ob_start("ob_gzhandler");
    }
}

function serendipity_serverOffsetHour($timestamp = null, $negative = false) {
    global $serendipity;

    if ($timestamp == null) {
        $timestamp = time();
    }

    if (empty($serendipity['serverOffsetHours']) || !is_numeric($serendipity['serverOffsetHours']) || $serendipity['serverOffsetHours'] == 0) {
        return $timestamp;
    } else {
        return $timestamp + (($negative ? -$serendipity['serverOffsetHours'] : $serendipity['serverOffsetHours']) * 60 * 60);
    }
}

function serendipity_strftime($format, $timestamp = null, $useOffset = true) {
    global $serendipity;
    
    switch($serendipity['calendar']) {
        default:
        case 'gregorian':
            if ($timestamp == null) {
                $timestamp = serendipity_serverOffsetHour();
            } elseif ($useOffset) {
                $timestamp = serendipity_serverOffsetHour($timestamp);
            }
            return strftime($format, $timestamp);
        
        case 'jalali-utf8':
            if ($timestamp == null) {
                $timestamp = serendipity_serverOffsetHour();
            } elseif ($useOffset) {
                $timestamp = serendipity_serverOffsetHour($timestamp);
            }

            require_once S9Y_INCLUDE_PATH . 'include/functions_calendars.inc.php';
            return jalali_strftime_utf($format, $timestamp);
    }
}

function serendipity_formatTime($format, $time, $useOffset = true) {
    static $cache;
    if (!isset($cache)) {
        $cache = array();
    }

    if (!isset($cache[$format])) {
        $cache[$format] = $format;
        if (strtoupper(substr(PHP_OS, 0, 3)) === 'WIN') {
            $cache[$format] = str_replace('%e', '%d', $cache[$format]);
        }
    }
    return serendipity_mb('ucfirst', serendipity_strftime($cache[$format], (int)$time, $useOffset));
}

function serendipity_fetchTemplates($dir = '') {
    global $serendipity;

    $cdir = opendir($serendipity['serendipityPath'] . $serendipity['templatePath'] . $dir);
    $rv  = array();
    while (($file = readdir($cdir)) !== false) {
        if (is_dir($serendipity['serendipityPath'] . $serendipity['templatePath'] . $dir . $file) && !ereg('^(\.|CVS)', $file) && !file_exists($serendipity['serendipityPath'] . $serendipity['templatePath'] . $dir . $file . '/inactive.txt')) {
            if (file_exists($serendipity['serendipityPath'] . $serendipity['templatePath'] . $dir . $file . '/info.txt')) {
                $rv[] = $dir . $file;
            } else {
                $temp = serendipity_fetchTemplates($dir . $file . '/');
                if (count($temp) > 0) {
                    $rv = array_merge($rv, $temp);
                }
            }
        }
    }
    closedir($cdir);
    natcasesort($rv);
    return $rv;
}

function serendipity_fetchTemplateInfo($theme, $abspath = null) {
    global $serendipity;

    if ($abspath === null) {
        $abspath = $serendipity['serendipityPath'] . $serendipity['templatePath'];
    }

    $lines = @file($abspath . $theme . '/info.txt');
    if ( !$lines ) {
        return array();
    }

    for($x=0; $x<count($lines); $x++) {
        $j = preg_split('/([^\:]+)\:/', $lines[$x], -1, PREG_SPLIT_DELIM_CAPTURE);
        if ($j[2]) {
            $currSec = $j[1];
            $data[strtolower($currSec)][] = trim($j[2]);
        } else {
            $data[strtolower($currSec)][] = trim($j[0]);
        }
    }

    foreach ($data as $k => $v) {
        $data[$k] = implode("\n", $v);
    }

    return $data;
}

function serendipity_walkRecursive($ary, $child_name = 'id', $parent_name = 'parent_id', $parentid = 0, $depth = 0) {
    global $serendipity;
    static $_resArray;

    if ( sizeof($ary) == 0 ) {
        return array();
    }

    if ($parentid === VIEWMODE_THREADED) {
        $parentid = 0;
    }

    if ( $depth == 0 ) {
        $_resArray = array();
    }

    foreach ($ary as $data) {
        if ($parentid === VIEWMODE_LINEAR || !isset($data[$parent_name]) || $data[$parent_name] == $parentid) {
            $data['depth'] = $depth;
            $_resArray[] = $data;
            if ($data[$child_name] && $parentid !== VIEWMODE_LINEAR ) {
                serendipity_walkRecursive($ary, $child_name, $parent_name, $data[$child_name], ($depth+1));
            }
        }
    }

    /* We are inside a recusive child, and we need to break out */
    if ($depth !== 0) {
        return true;
    }

    return $_resArray;
}

function serendipity_fetchUsers($user = '', $group = null, $is_count = false) {
    global $serendipity;

    $where = '';
    if (!empty($user)) {
        $where = "WHERE a.authorid = '" . (int)$user ."'";
    }

    $query_select   = '';
    $query_join     = '';
    $query_group    = '';
    $query_distinct = '';
    if ($is_count) {
        $query_select = ", count(e.authorid) as artcount";
        $query_join   = "LEFT OUTER JOIN {$serendipity['dbPrefix']}entries AS e
                                      ON a.authorid = e.authorid";
    }
    
    if ($is_count || $group != null) {
        if ($serendipity['dbType'] == 'postgres') {
            // Why does PostgreSQL keep doing this to us? :-)
            $query_group    = 'GROUP BY a.authorid, a.realname, a.username, a.password, a.mail_comments, a.mail_trackbacks, a.email, a.userlevel, a.right_publish';
            $query_distinct = 'DISTINCT';
        } else {
            $query_group    = 'GROUP BY a.authorid';
            $query_distinct = '';
        }
    }


    if ($group === null) {
        $querystring = "SELECT $query_distinct
                               a.authorid, 
                               a.realname,
                               a.username,
                               a.password,
                               a.mail_comments,
                               a.mail_trackbacks,
                               a.email,
                               a.userlevel,
                               a.right_publish
                               $query_select
                          FROM {$serendipity['dbPrefix']}authors AS a
                               $query_join
                               $where 
                               $query_group
                      ORDER BY a.realname ASC";
    } else {
        if (is_array($group)) {
            foreach($group AS $idx => $groupid) {
                $group[$idx] = (int)$groupid;
            }
            $group_sql = implode(', ', $group);
        } else {
            $group_sql = (int)$group;
        }

        $querystring = "SELECT $query_distinct
                               a.authorid, 
                               a.realname,
                               a.username,
                               a.password,
                               a.mail_comments,
                               a.mail_trackbacks,
                               a.email,
                               a.userlevel,
                               a.right_publish
                               $query_select
                          FROM {$serendipity['dbPrefix']}authors AS a 
               LEFT OUTER JOIN {$serendipity['dbPrefix']}authorgroups AS ag
                            ON a.authorid = ag.authorid
               LEFT OUTER JOIN {$serendipity['dbPrefix']}groups AS g
                            ON ag.groupid  = g.id
                               $query_join
                         WHERE g.id IN ($group_sql)
                               $where
                               $query_group
                      ORDER BY a.realname ASC";
    }
    
    return serendipity_db_query($querystring);
}


function serendipity_sendMail($to, $subject, $message, $fromMail, $headers = NULL, $fromName = NULL) {
    global $serendipity;

    if ( !is_null($headers) && !is_array($headers) ) {
        trigger_error(__FUNCTION__ . ': $headers must be either an array or null', E_USER_ERROR);
    }

    if ( is_null($fromName) || empty($fromName)) {
        $fromName = $serendipity['blogTitle'];
    }

    if ( is_null($fromMail) || empty($fromMail)) {
        $fromMail = $to;
    }

    // Fix special characters
    $fromName = str_replace(array('"', "\r", "\n"), array("'", '', ''), $fromName);
    $fromMail = str_replace(array("\r","\n"), array('', ''), $fromMail);

    /* Prefix all mail with weblog title */
    $subject = '['. $serendipity['blogTitle'] . '] '.  $subject;

    /* Append signature to every mail */
    $message .= "\n" . sprintf(SIGNATURE, $serendipity['blogTitle']);


    /* Check for mb_* function, and use it to encode headers etc. */
    if ( function_exists('mb_encode_mimeheader') ) {
        // Funky mb_encode_mimeheader function insertes linebreaks after 74 chars.
        // Most MTA I've personally spoken with told me they don't like this at all. ;)
        // Regards to Mark Kronsbein for finding this issue!
        $subject = str_replace(array("\n", "\r"), array('', ''), mb_encode_mimeheader($subject, LANG_CHARSET));
        $fromName = str_replace(array("\n", "\r"), array('', ''), mb_encode_mimeheader($fromName, LANG_CHARSET));
    }


    /* Always add these headers */
    if (!empty($serendipity['blogMail'])) {
        $headers[] = 'From: "'. $fromName .'" <'. $serendipity['blogMail'] .'>';
    }
    $headers[] = 'Reply-To: "'. $fromName .'" <'. $fromMail .'>';
    $headers[] = 'X-Mailer: Serendipity/'. $serendipity['version'];
    $headers[] = 'X-Engine: PHP/'. phpversion();
    $headers[] = 'Message-ID: <'. md5(microtime() . uniqid(time())) .'@'. $_SERVER['HTTP_HOST'] .'>';
    $headers[] = 'MIME-Version: 1.0';
    $headers[] = 'Content-Type: text/plain; charset="' . LANG_CHARSET .'"';

    return mail($to, $subject, $message, implode("\n", $headers));
}

function serendipity_fetchReferences($id) {
    global $serendipity;

    $query = "SELECT name,link FROM {$serendipity['dbPrefix']}references WHERE entry_id = '" . (int)$id . "'";

    return serendipity_db_query($query);
}


function serendipity_utf8_encode($string) {
    if (strtolower(LANG_CHARSET) != 'utf-8') {
        if (function_exists('iconv')) {
            return iconv(LANG_CHARSET, 'UTF-8', $string);
        } else if (function_exists('mb_convert_encoding')) {
            return mb_convert_encoding($string, 'UTF-8', LANG_CHARSET);
        } else {
            return utf8_encode($string);
        }
    } else {
        return $string;
    }
}

function serendipity_rss_getguid($entry, $comments = false) {
    global $serendipity;

    $id = (isset($entry['entryid']) && $entry['entryid'] != '' ? $entry['entryid'] : $entry['id']);

    // When using %id%, we can make the GUID shorter and independent from the title.
    // If not using %id%, the entryid needs to be used for uniqueness.
    if (stristr($serendipity['permalinkStructure'], '%id%') !== FALSE) {
        $title = 'guid';
    } else {
        $title = $id;
    }

    $guid = serendipity_archiveURL(
        $id, 
        $title, 
        'baseURL', 
        true, 
        array('timestamp' => $entry['timestamp'])
    );

    if ($comments == true) {
        $guid .= '#c' . $entry['commentid'];
    }

    return $guid;
}

// jbalcorn: starter function to clean up xhtml for atom feed.  Add things to this as we find common
//      mistakes, unless someone finds a better way to do this.
//      DONE:
//          since someone encoded all the urls, we can now assume any amp followed by
//              whitespace or a HTML tag (i.e. &<br /> )should be
//              encoded and most not with a space are intentional
//      TODO:
//          check ALL ampersands, find out if it's a valid code, and encode if not
function xhtml_cleanup($html) {
    static $p = array(
        '/\&([\s\<])/',       // ampersand followed by whitespace or tag
        '/\&$/',              // ampersand at end of body
        '/<(br|hr|img)([^\/>]*)>/i',    // unclosed br tag - attributes included
        '/\&nbsp;/'
    );

    static $r = array(
        '&amp;\1',
        '&amp;',
        '<\1\2 />',
        '&#160;'
    );

    return preg_replace($p, $r, $html);
}

function serendipity_fetchAuthor($author) {
    global $serendipity;

    return serendipity_db_query("SELECT * FROM {$serendipity['dbPrefix']}authors WHERE " . (is_numeric($author) ? "authorid={$author};" : "username='" . serendipity_db_escape_string($author) . "';"));
}

/**
* Split up a filename
**/
function serendipity_parseFileName($file) {
    $x = explode('.', $file);
    $suf = array_pop($x);
    $f   = @implode('.', $x);
    return array($f, $suf);
}

function serendipity_track_referrer($entry = 0) {
    global $serendipity;

    if (isset($_SERVER['HTTP_REFERER'])) {
        if (stristr($_SERVER['HTTP_REFERER'], $serendipity['baseURL']) !== false) {
            return;
        }

        if (!isset($serendipity['_blockReferer']) || !is_array($serendipity['_blockReferer'])) {
            // Only generate an array once per call
            $serendipity['_blockReferer'] = array();
            $serendipity['_blockReferer'] = @explode(';', $serendipity['blockReferer']);
        }

        $url_parts  = parse_url($_SERVER['HTTP_REFERER']);
        $host_parts = explode('.', $url_parts['host']);
        if (!$url_parts['host'] ||
            strstr($url_parts['host'], $_SERVER['SERVER_NAME'])) {
            return;
        }

        foreach($serendipity['_blockReferer'] AS $idx => $hostname) {
            if (@strstr($url_parts['host'], $hostname)) {
                return;
            }
        }

        if (rand(0, 100) < 1) {
            serendipity_track_referrer_gc();
        }

        $ts       = serendipity_db_get_interval('ts');
        $interval = serendipity_db_get_interval('interval', 900);

        $suppressq = "SELECT count(1)
                      FROM $serendipity[dbPrefix]suppress
                      WHERE ip = '" . serendipity_db_escape_string($_SERVER['REMOTE_ADDR']) . "'
                      AND scheme = '" . serendipity_db_escape_string($url_parts['scheme']) . "'
                      AND port = '" . serendipity_db_escape_string($url_parts['port']) . "'
                      AND host = '" . serendipity_db_escape_string($url_parts['host']) . "'
                      AND path = '" . serendipity_db_escape_string($url_parts['path']) . "'
                      AND query = '" . serendipity_db_escape_string($url_parts['query']) . "'
                      AND last > $ts - $interval";

        $suppressp = "DELETE FROM $serendipity[dbPrefix]suppress
                      WHERE ip = '" . serendipity_db_escape_string($_SERVER['REMOTE_ADDR']) . "'
                      AND scheme = '" . serendipity_db_escape_string($url_parts['scheme']) . "'
                      AND host = '" . serendipity_db_escape_string($url_parts['host']) . "'
                      AND port = '" . serendipity_db_escape_string($url_parts['port']) . "'
                      AND query = '" . serendipity_db_escape_string($url_parts['query']) . "'
                      AND path = '" . serendipity_db_escape_string($url_parts['path']) . "'";
        $suppressu = "INSERT INTO $serendipity[dbPrefix]suppress
                      (ip, last, scheme, host, port, path, query)
                      VALUES (
                      '" . serendipity_db_escape_string($_SERVER['REMOTE_ADDR']) . "',
                      $ts,
                      '" . serendipity_db_escape_string($url_parts['scheme']) . "',
                      '" . serendipity_db_escape_string($url_parts['host']) . "',
                      '" . serendipity_db_escape_string($url_parts['port']) . "',
                      '" . serendipity_db_escape_string($url_parts['path']) . "',
                      '" . serendipity_db_escape_string($url_parts['query']) . "'
                      )";

        $count = serendipity_db_query($suppressq, true);

        if ($count[0] == 0) {
            serendipity_db_query($suppressu);
            return;
        }

        serendipity_db_query($suppressp);
        serendipity_db_query($suppressu);

        serendipity_track_url('referrers', $_SERVER['HTTP_REFERER'], $entry);
    }
}

function serendipity_track_referrer_gc() {
    global $serendipity;

    $ts       = serendipity_db_get_interval('ts');
    $interval = serendipity_db_get_interval('interval', 900);
    $gc = "DELETE FROM $serendipity[dbPrefix]suppress WHERE last <= $ts - $interval";
    serendipity_db_query($gc);
}

function serendipity_track_url($list, $url, $entry_id = 0) {
    global $serendipity;

    $url_parts = parse_url($url);

    serendipity_db_query(
      @sprintf(
        "UPDATE %s%s
            SET count = count + 1
          WHERE scheme = '%s'
            AND host   = '%s'
            AND port   = '%s'
            AND path   = '%s'
            AND query  = '%s'
            AND day    = '%s'
            %s",

        $serendipity['dbPrefix'],
        $list,
        serendipity_db_escape_string($url_parts['scheme']),
        serendipity_db_escape_string($url_parts['host']),
        serendipity_db_escape_string($url_parts['port']),
        serendipity_db_escape_string($url_parts['path']),
        serendipity_db_escape_string($url_parts['query']),
        date('Y-m-d'),
        ($entry_id != 0) ? "AND entry_id = '". (int)$entry_id ."'" : ''
      )
    );

    if (serendipity_db_affected_rows() == 0) {
        serendipity_db_query(
          sprintf(
            "INSERT INTO %s%s
                    (entry_id, day, count, scheme, host, port, path, query)
             VALUES (%d, '%s', 1, '%s', '%s', '%s', '%s', '%s')",

            $serendipity['dbPrefix'],
            $list,
            (int)$entry_id,
            date('Y-m-d'),
            serendipity_db_escape_string($url_parts['scheme']),
            serendipity_db_escape_string($url_parts['host']),
            serendipity_db_escape_string($url_parts['port']),
            serendipity_db_escape_string($url_parts['path']),
            serendipity_db_escape_string($url_parts['query'])
          )
        );
    }
}

function serendipity_displayTopReferrers($limit = 10, $use_links = true, $interval = 7) {
    serendipity_displayTopUrlList('referrers', $limit, $use_links, $interval);
}

function serendipity_displayTopExits($limit = 10, $use_links = true, $interval = 7) {
    serendipity_displayTopUrlList('exits', $limit, $use_links, $interval);
}

function serendipity_displayTopUrlList($list, $limit, $use_links = true, $interval = 7) {
    global $serendipity;

    if ($limit){
        $limit = serendipity_db_limit_sql($limit);
    }

    /* HACK */
    if (preg_match('/^mysqli?/', $serendipity['dbType'])) {
        /* Nonportable SQL due to MySQL date functions,
         * but produces rolling 7 day totals, which is more
         * interesting
         */
        $query = "SELECT scheme, host, SUM(count) AS total
                  FROM {$serendipity['dbPrefix']}$list
                  WHERE day > date_sub(current_date, interval " . (int)$interval . " day)
                  GROUP BY host
                  ORDER BY total DESC, host
                  $limit";
    } else {
        /* Portable version of the same query */
        $query = "SELECT scheme, host, SUM(count) AS total
                  FROM {$serendipity['dbPrefix']}$list
                  GROUP BY scheme, host
                  ORDER BY total DESC, host
                  $limit";
    }

    $rows = serendipity_db_query($query);
    echo "<span class='serendipityReferer'>";
    if (is_array($rows)) {
        foreach ($rows as $row) {
            if ($use_links) {
                printf(
                    '<a href="%1$s://%2$s" title="%2$s" >%2$s</a> (%3$s)<br />',
                    $row['scheme'],
                    $row['host'],
                    $row['total']
                );
            } else {
                printf(
                    '%1$s (%2$s)<br />',
                    $row['host'],
                    $row['total']
                );
            }
        }
    }
    echo "</span>";
}

function serendipity_xhtml_target($target) {
    global $serendipity;

    if ($serendipity['enablePopup'] != true)
        return "";

    if ($serendipity['XHTML11']) {
        return ' onclick="window.open(this.href, \'target' . time() . '\'); return false;" ';
    } else {
        return ' target="' . $target . '" ';
    }
}

function serendipity_discover_rss($name, $ext) {
    static $default = '2.0';

    /* Detect type */
    if ( $name == 'comments' ) {
        $type = 'comments';
    } else {
        $type = 'content';
    }

    /* Detect version */
    if ($name == 'atom' || $name == 'atom10' || $ext == 'atom') {
        $ver = 'atom1.0';
    } elseif ($name == 'atom03') {
        $ver = 'atom0.3';
    } elseif ($name == 'opml' || $ext == 'opml') {
        $ver = 'opml1.0';
    } elseif ($ext == 'rss') {
        $ver = '0.91';
    } elseif ($ext == 'rss1') {
        $ver = '1.0';
    } else {
        $ver = $default;
    }

    return array($ver, $type);
}

function serendipity_isResponseClean($d) {
    return (strpos($d, "\r") === false && strpos($d, "\n") === false);
}

function serendipity_addCategory($name, $desc, $authorid, $icon, $parentid) {
    global $serendipity;
    $query = "INSERT INTO {$serendipity['dbPrefix']}category
                    (category_name, category_description, authorid, category_icon, parentid, category_left, category_right)
                  VALUES 
                    ('". serendipity_db_escape_string($name) ."', 
                     '". serendipity_db_escape_string($desc) ."', 
                      ". (int)$authorid .", 
                     '". serendipity_db_escape_string($icon) ."', 
                      ". (int)$parentid .", 
                       0, 
                       0)";

    serendipity_db_query($query);
    $cid = serendipity_db_insert_id('category', 'categoryid');
    serendipity_plugin_api::hook_event('backend_category_addNew', $cid);

    $data = array(
        'categoryid'           => $cid,
        'category_name'        => $name,
        'category_description' => $desc
    );

    serendipity_insertPermalink($data, 'category');
    return $cid;
}

function serendipity_updateCategory($cid, $name, $desc, $authorid, $icon, $parentid) {
    global $serendipity;

    $query = "UPDATE {$serendipity['dbPrefix']}category
                    SET category_name = '". serendipity_db_escape_string($name) ."',
                        category_description = '". serendipity_db_escape_string($desc) ."',
                        authorid = ". (int)$authorid .",
                        category_icon = '". serendipity_db_escape_string($icon) ."',
                        parentid = ". (int)$parentid ."
                    WHERE categoryid = ". (int)$cid ."
                        $admin_category";
    serendipity_db_query($query);
    serendipity_plugin_api::hook_event('backend_category_update', $cid);

    $data = array(
        'id'                   => $cid,
        'categoryid'           => $cid,
        'category_name'        => $name,
        'category_description' => $desc
    );

    serendipity_updatePermalink($data, 'category');

}

define("serendipity_FUNCTIONS_LOADED", true);
/* vim: set sts=4 ts=4 expandtab : */
