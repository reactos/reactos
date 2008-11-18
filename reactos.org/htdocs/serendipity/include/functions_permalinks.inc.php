<?php # $Id: functions.inc.php 85 2005-05-10 10:11:05Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

function serendipity_makeFilename($str) {
    static $from = array(
                     ' ',

                     'Ä',
                     'ä',

                     'Ö',
                     'ö',

                     'Ü',
                     'ü',

                     'ß',

                     'é',
                     'è',
                     'ê',

                     'í',
                     'ì',
                     'î',

                     'á',
                     'à',
                     'â',
                     'å',

                     'ó',
                     'ò',
                     'ô',
                     'õ',

                     'ú',
                     'ù',
                     'û',

                     'ç',
                     'Ç',

                     'ñ',

                     'ý');

    static $to   = array(
                     '-',

                     'AE',
                     'ae',

                     'OE',
                     'oe',

                     'UE',
                     'ue',

                     'ss',

                     'e',
                     'e',
                     'e',

                     'i',
                     'i',
                     'i',

                     'a',
                     'a',
                     'a',
                     'a',

                     'o',
                     'o',
                     'o',
                     'o',

                     'u',
                     'u',
                     'u',

                     'c',
                     'C',

                     'n',

                     'y');

    if (LANG_CHARSET == 'UTF-8') {
        // URLs need to be 7bit - since this function takes care of the most common ISO-8859-1
        // characters, try to UTF8-decode the string first.
        $str = utf8_decode($str);
    }

    if (isset($GLOBALS['i18n_filename_from'])) {
        // Replace international chars not detected by every locale.
        // The array of chars is defined in the language file.
        $str = str_replace($GLOBALS['i18n_filename_from'], $GLOBALS['i18n_filename_to'], $str);
    } else {
        // Replace international chars not detected by every locale
        $str = str_replace($from, $to, $str);
    }

    // Nuke chars not allowed in our URI
    $str = preg_replace('#[^' . PAT_FILENAME . ']#i', '', $str);

    // Remove consecutive separators
    $str = preg_replace('#'. $to[0] .'{2,}#s', $to[0], $str);

    // Remove excess separators
    $str = trim($str, $to[0]);

    if (empty($str)) {
        if (isset($GLOBALS['i18n_unknown'])) {
            $str = $GLOBALS['i18n_unknown'];
        } else {
            $str = 'unknown';
        }
    }

    return $str;
}

function serendipity_initPermalinks() {
    global $serendipity;

    if (!isset($serendipity['permalinkStructure'])) {
        $serendipity['permalinkStructure'] = 'archives/%id%-%title%.html';
    }
    
    if (!isset($serendipity['permalinkFeedAuthorStructure'])) {
        $serendipity['permalinkFeedAuthorStructure'] = 'feeds/authors/%id%-%realname%.rss';
    }
    
    if (!isset($serendipity['permalinkFeedCategoryStructure'])) {
        $serendipity['permalinkFeedCategoryStructure'] = 'feeds/categories/%id%-%name%.rss';
    }
    
    if (!isset($serendipity['permalinkCategoryStructure'])) {
        $serendipity['permalinkCategoryStructure'] = 'categories/%id%-%name%';
    }
    
    if (!isset($serendipity['permalinkAuthorStructure'])) {
        $serendipity['permalinkAuthorStructure'] = 'authors/%id%-%realname%';
    }
    
    if (!isset($serendipity['permalinkArchivesPath'])) {
        $serendipity['permalinkArchivesPath'] = 'archives';
    }
    
    if (!isset($serendipity['permalinkArchivePath'])) {
        $serendipity['permalinkArchivePath'] = 'archive';
    }
    
    if (!isset($serendipity['permalinkCategoriesPath'])) {
        $serendipity['permalinkCategoriesPath'] = 'categories';
    }
    
    if (!isset($serendipity['permalinkAuthorsPath'])) {
        $serendipity['permalinkAuthorsPath'] = 'authors';
    }
    
    if (!isset($serendipity['permalinkUnsubscribePath'])) {
        $serendipity['permalinkUnsubscribePath'] = 'unsubscribe';
    }
    
    if (!isset($serendipity['permalinkDeletePath'])) {
        $serendipity['permalinkDeletePath'] = 'delete';
    }
    
    if (!isset($serendipity['permalinkApprovePath'])) {
        $serendipity['permalinkApprovePath'] = 'approve';
    }
    
    if (!isset($serendipity['permalinkFeedsPath'])) {
        $serendipity['permalinkFeedsPath'] = 'feeds';
    }
    
    if (!isset($serendipity['permalinkPluginPath'])) {
        $serendipity['permalinkPluginPath'] = 'plugin';
    }
    
    if (!isset($serendipity['permalinkAdminPath'])) {
        $serendipity['permalinkAdminPath'] = 'admin';
    }
    
    if (!isset($serendipity['permalinkSearchPath'])) {
        $serendipity['permalinkSearchPath'] = 'search';
    }
    
    /* URI paths
     * These could be defined in the language headers, except that would break
     * backwards URL compatibility
     */
    @define('PATH_ARCHIVES',    $serendipity['permalinkArchivesPath']);
    @define('PATH_ARCHIVE',     $serendipity['permalinkArchivePath']);
    @define('PATH_CATEGORIES',  $serendipity['permalinkCategoriesPath']);
    @define('PATH_UNSUBSCRIBE', $serendipity['permalinkUnsubscribePath']);
    @define('PATH_DELETE',      $serendipity['permalinkDeletePath']);
    @define('PATH_APPROVE',     $serendipity['permalinkApprovePath']);
    @define('PATH_FEEDS',       $serendipity['permalinkFeedsPath']);
    @define('PATH_PLUGIN',      $serendipity['permalinkPluginPath']);
    @define('PATH_ADMIN',       $serendipity['permalinkAdminPath']);
    @define('PATH_SEARCH',      $serendipity['permalinkSearchPath']);
    
    /* URI patterns
     * Note that it's important to use @ as the pattern delimiter. DO NOT use shortcuts
     * like \d or \s, since mod_rewrite will use the regexps as well and chokes on them.
     * If you add new patterns, remember to add the new rules to the *.tpl files and
     * function serendipity_installFiles().
     */
    @define('PAT_FILENAME',   '0-9a-z\.\_!;,\+\-');
    @define('PAT_CSS',        '@/(serendipity\.css|serendipity_admin\.css)@');
    @define('PAT_FEED',       '@/(index|atom[0-9]*|rss|b2rss|b2rdf).(rss|rdf|rss2|xml)@');
    @define('PAT_COMMENTSUB', '@/([0-9]+)[_\-][' . PAT_FILENAME . ']*\.html@i');

    return true;
}

function &serendipity_permalinkPatterns($return = false) {
    global $serendipity;
    
    $PAT = array();

    $PAT['UNSUBSCRIBE']              = '@/'  . $serendipity['permalinkUnsubscribePath'].'/(.*)/([0-9]+)@';
    $PAT['APPROVE']                  = '@/'  . $serendipity['permalinkApprovePath'].'/(.*)/(.*)/([0-9]+)@';
    $PAT['DELETE']                   = '@/'  . $serendipity['permalinkDeletePath'].'/(.*)/(.*)/([0-9]+)@';
    $PAT['ARCHIVES']                 = '@/'  . $serendipity['permalinkArchivesPath'].'([/A-Za-z0-9]+)\.html@';
    $PAT['FEEDS']                    = '@/'  . $serendipity['permalinkFeedsPath'].'/@';
    $PAT['ADMIN']                    = '@/(' . $serendipity['permalinkAdminPath'] . '|entries)(/.+)?@';
    $PAT['ARCHIVE']                  = '@/'  . $serendipity['permalinkArchivePath'] . '/?@';
    $PAT['CATEGORIES']               = '@/'  . $serendipity['permalinkCategoriesPath'].'/([0-9;]+)@';
    $PAT['PLUGIN']                   = '@/('  . $serendipity['permalinkPluginPath'] . '|plugin)/(.*)@';
    $PAT['SEARCH']                   = '@/'  . $serendipity['permalinkSearchPath'] . '/(.*)@';
    $PAT['PERMALINK']                = '@'   . serendipity_makePermalinkRegex($serendipity['permalinkStructure'], 'entry') . '@i';
    $PAT['PERMALINK_CATEGORIES']     = '@'   . serendipity_makePermalinkRegex($serendipity['permalinkCategoryStructure'], 'category') . '@i';
    $PAT['PERMALINK_FEEDCATEGORIES'] = '@'   . serendipity_makePermalinkRegex($serendipity['permalinkFeedCategoryStructure'], 'category') . '@i';
    $PAT['PERMALINK_FEEDAUTHORS']    = '@'   . serendipity_makePermalinkRegex($serendipity['permalinkFeedAuthorStructure'], 'author') . '@i';
    $PAT['PERMALINK_AUTHORS']        = '@'   . serendipity_makePermalinkRegex($serendipity['permalinkAuthorStructure'], 'author') . '@i';

    if ($return) {
        return $PAT;
    } else {
        foreach($PAT AS $constant => $value) {
            define('PAT_' . $constant, $value);
        }

        $return = true;
        return $return;
    }
}

function serendipity_searchPermalink($struct, $url, $default, $type = 'entry') {
    global $serendipity;

    if (stristr($struct, '%id%') === FALSE) {
        $url = preg_replace('@^(' . preg_quote($serendipity['serendipityHTTPPath'], '@') . '(' . preg_quote($serendipity['indexFile'], '@') . ')?\??(url=)?/?)([^&?]+).*@', '\4', $url);

        // If no entryid is submitted, we rely on a new DB call to fetch the permalink.
        $pq = "SELECT entry_id, data
                 FROM {$serendipity['dbPrefix']}permalinks
                WHERE permalink = '" . serendipity_db_escape_string($url) . "'
                  AND type      = '" . serendipity_db_escape_string($type) . "'
                  AND entry_id  > 0
                LIMIT 1";
// echo $pq; // DEBUG
// die($pq); // DEBUG
        $permalink = serendipity_db_query($pq, true, 'both', false, false, false, true);

        if (is_array($permalink)) {
            return $permalink['entry_id'];
        }
    }    

    return $default;
}

function serendipity_getPermalink(&$data, $type = 'entry') {
    switch($type) {
        case 'entry':
            return serendipity_archiveURL(
                        $data['id'],
                        $data['title'],
                        '',
                        false,
                        array('timestamp' => $data['timestamp'])
            );
            break;

        case 'category':
            return serendipity_categoryURL($data, '', false);
            break;
            
        case 'author':
            return serendipity_authorURL($data, '', false);
            break;
    }
    
    return false;
}

function serendipity_updatePermalink(&$data, $type = 'entry') {
    global $serendipity;

    $link = serendipity_getPermalink($data, $type);
    return(serendipity_db_query(sprintf("UPDATE {$serendipity['dbPrefix']}permalinks
                                            SET permalink = '%s'
                                          WHERE entry_id  = %s
                                            AND type      = '%s'",

                                            serendipity_db_escape_string($link),
                                            (int)$data['id'],
                                            serendipity_db_escape_string($type))));
}

function serendipity_insertPermalink(&$data, $type = 'entry') {
    global $serendipity;

    $link = serendipity_getPermalink($data, $type);

    switch($type) {
        case 'entry':
            $idfield = 'id';
            break;
            
        case 'author':
            $idfield = 'authorid';
            break;
            
        case 'category':
            $idfield = 'categoryid';
            break;
    }
    
    return(serendipity_db_query(sprintf("INSERT INTO {$serendipity['dbPrefix']}permalinks
                                                    (permalink, entry_id, type)
                                             VALUES ('%s', '%s', '%s')",

                                            serendipity_db_escape_string($link),
                                            (int)$data[$idfield],
                                            serendipity_db_escape_string($type))));
}

function serendipity_buildPermalinks() {
    global $serendipity;

    $entries = serendipity_db_query("SELECT id, title, timestamp FROM {$serendipity['dbPrefix']}entries");

    if (is_array($entries)) {
        serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}permalinks WHERE type = 'entry'");

        foreach($entries AS $entry) {
            serendipity_insertPermalink($entry, 'entry');
        }
    }

    $authors = serendipity_db_query("SELECT * FROM {$serendipity['dbPrefix']}authors");

    if (is_array($authors)) {
        serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}permalinks WHERE type = 'author'");

        foreach($authors AS $author) {
            serendipity_insertPermalink($author, 'author');
        }
    }

    $categories = serendipity_db_query("SELECT * FROM {$serendipity['dbPrefix']}category");

    if (is_array($categories)) {
        serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}permalinks WHERE type = 'category'");

        foreach($categories AS $category) {
            serendipity_insertPermalink($category, 'category');
        }
    }
}

/* Uses logic to figure out how the URI should look, based on current rewrite rule */
function serendipity_rewriteURL($path, $key='baseURL', $forceNone = false) {
    global $serendipity;
    return $serendipity[$key] . ($serendipity['rewrite'] == 'none' || ($serendipity['rewrite'] != 'none' && $forceNone) ? $serendipity['indexFile'] . '?/' : '') . $path;
}

function serendipity_makePermalink($format, $data, $type = 'entry') {
    global $serendipity;
    static $entryKeys    = array('%id%', '%title%', '%day%', '%month%', '%year%');
    static $authorKeys   = array('%id%', '%username%', '%realname%', '%email%');
    static $categoryKeys = array('%id%', '%name%', '%description%');

    switch($type) {
        case 'entry':
            if (!isset($data['entry']['timestamp']) && preg_match('@(%day%|%month%|%year%)@', $format)) {
                // We need the timestamp to build the URI, but no timestamp has been submitted. Thus we need to fetch the data.
                $ts = serendipity_db_query("SELECT timestamp FROM {$serendipity['dbPrefix']}entries WHERE id = " . (int)$data['id'], true);
                if (is_array($ts)) {
                    $data['entry']['timestamp'] = $ts['timestamp'];
                } else {
                    $data['entry']['timestamp'] = time();
                }
            }
        
            $ts = serendipity_serverOffsetHour($data['entry']['timestamp']);
        
            $replacements =
                array(
                    (int)$data['id'],
                    serendipity_makeFilename($data['title']),
                    date('d', $ts),
                    date('m', $ts),
                    date('Y', $ts)
                );
            return str_replace($entryKeys, $replacements, $format);
            break;
            
        case 'author':
            $replacements =
                array(
                    (int)$data['authorid'],
                    serendipity_makeFilename($data['username']),
                    serendipity_makeFilename($data['realname']),
                    serendipity_makeFilename($data['email'])
                );
            return str_replace($authorKeys, $replacements, $format);
            break;
            
        case 'category':
            $replacements =
                array(
                    (int)$data['categoryid'],
                    serendipity_makeFilename($data['category_name']),
                    serendipity_makeFilename($data['category_description'])
                );
            return str_replace($categoryKeys, $replacements, $format);
            break;
    }

    return false;
}

function serendipity_makePermalinkRegex($format, $type = 'entry') {
    static $entryKeys           = array('%id%',     '%title%',              '%day%',      '%month%',    '%year%');
    static $entryRegexValues    = array('([0-9]+)', '[0-9a-z\.\_!;,\+\-]+', '[0-9]{1,2}', '[0-9]{1,2}', '[0-9]{4}');

    static $authorKeys          = array('%id%',     '%username%',           '%realname%',           '%email%');
    static $authorRegexValues   = array('([0-9]+)', '[0-9a-z\.\_!;,\+\-]+', '[0-9a-z\.\_!;,\+\-]+', '[0-9a-z\.\_!;,\+\-]+');

    static $categoryKeys        = array('%id%',     '%name%',               '%description%');
    static $categoryRegexValues = array('([0-9;]+)', '[0-9a-z\.\_!;,\+\-]+', '[0-9a-z\.\_!;,\+\-]+');

    switch($type) {
        case 'entry':
            return str_replace($entryKeys, $entryRegexValues, preg_quote($format));
            break;

        case 'author':
            return str_replace($authorKeys, $authorRegexValues, preg_quote($format));
            break;

        case 'category':
            return str_replace($categoryKeys, $categoryRegexValues, preg_quote($format));
            break;
    }
}

function serendipity_archiveURL($id, $title, $key = 'baseURL', $checkrewrite = true, $entryData = null) {
    global $serendipity;
    $path = serendipity_makePermalink($serendipity['permalinkStructure'], array('id'=> $id, 'title' => $title, 'entry' => $entryData));
    if ($checkrewrite) {
        $path = serendipity_rewriteURL($path, $key);
    }
    return $path;
}

function serendipity_authorURL(&$data, $key = 'baseURL', $checkrewrite = true) {
    global $serendipity;
    $path = serendipity_makePermalink($serendipity['permalinkAuthorStructure'], $data, 'author');
    if ($checkrewrite) {
        $path = serendipity_rewriteURL($path, $key);
    }
    return $path;
}

function serendipity_categoryURL(&$data, $key = 'baseURL', $checkrewrite = true) {
    global $serendipity;
    $path = serendipity_makePermalink($serendipity['permalinkCategoryStructure'], $data, 'category');
    if ($checkrewrite) {
        $path = serendipity_rewriteURL($path, $key);
    }
    return $path;
}

function serendipity_feedCategoryURL(&$data, $key = 'baseURL', $checkrewrite = true) {
    global $serendipity;
    $path = serendipity_makePermalink($serendipity['permalinkFeedCategoryStructure'], $data, 'category');
    if ($checkrewrite) {
        $path = serendipity_rewriteURL($path, $key);
    }
    return $path;
}

function serendipity_feedAuthorURL(&$data, $key = 'baseURL', $checkrewrite = true) {
    global $serendipity;
    $path = serendipity_makePermalink($serendipity['permalinkFeedAuthorStructure'], $data, 'author');
    if ($checkrewrite) {
        $path = serendipity_rewriteURL($path, $key);
    }
    return $path;
}

function serendipity_archiveDateUrl($range, $summary=false, $key='baseURL') {
    return serendipity_rewriteURL(PATH_ARCHIVES . '/' . $range . ($summary ? '/summary' : '') . '.html', $key);
}

function serendipity_currentURL() {
    global $serendipity;

    // All that URL getting humpty-dumpty is necessary to allow a user to change the template in the
    // articles view. POSTing data to that page only works with mod_rewrite and not the ErrorDocument
    // redirection, so we need to generate the ErrorDocument-URI here.

    $uri = parse_url($_SERVER['REQUEST_URI']);
    $qst = '';
    if (!empty($uri['query'])) {
        $qst = '&amp;' . str_replace('&', '&amp;', $uri['query']);
    }
    $uri['path'] = preg_replace('@^' . preg_quote($serendipity['serendipityHTTPPath']) . '@i', '', $uri['path']);
    $url = $serendipity['serendipityHTTPPath'] . $serendipity['indexFile'] . '?' . $uri['path'] . $qst;
    $url = str_replace(
        array(
            $serendipity['indexFile'] . '&amp;',
            '"',
            "'",
            '<',
            '>'
        ),

        array(
            '',
            '',
            '',
            ''
        ),

        $url); // Kill possible looped repitions and bad characters which could occur

    return $url;
}

function serendipity_getUriArguments($uri, $wildcard = false) {
global $serendipity;

    /* Explode the path into sections, to later be able to check for arguments and add our own */
    preg_match('/^'. preg_quote($serendipity['serendipityHTTPPath'], '/') . '(' . preg_quote($serendipity['indexFile'], '/') . '\?\/)?(' . ($wildcard ? '.+' : '[;a-z0-9\-*\/%\+]+') . ')/i', $uri, $_res);
    if (strlen($_res[2]) != 0) {
        $args = explode('/', $_res[2]);
        if ($args[0] == 'index') {
            unset($args[0]);
        }
        return $args;
    } else {
        return array();
    }
}

