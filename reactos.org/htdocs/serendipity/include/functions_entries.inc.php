<?php # $Id: functions_entries.inc.php 573 2005-10-19 13:24:01Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

function serendipity_deleteCategory($category_range, $admin_category) {
    global $serendipity;
    
    if (!serendipity_checkPermission('adminCategoriesDelete')) {
        return false;
    }
    
    serendipity_plugin_api::hook_event('backend_category_delete', $category_range);

    return serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}category WHERE category_left BETWEEN {$category_range} {$admin_category}");
}

function serendipity_fetchCategoryRange($categoryid) {
    global $serendipity;

    $res = serendipity_db_query("SELECT category_left, category_right FROM {$serendipity['dbPrefix']}category WHERE categoryid='". (int)$categoryid ."'");
    if (!is_array($res) || !isset($res[0]['category_left']) || !isset($res[0]['category_right'])) {
        $res = array(array('category_left' => 0, 'category_right' => 0));
    }

    return array('category_left' => $res[0]['category_left'], 'category_right' => $res[0]['category_right']);
}

function serendipity_getMultiCategoriesSQL($cats) {
    global $serendipity;

    $mcategories   = explode(';', $cats);
    $cat_sql_array = array();
    foreach($mcategories AS $categoryid) {
        $categoryid  = (int)$categoryid;

        if ($categoryid != 0) {
            $cat_sql_array[] = " c.category_left BETWEEN " . implode(' AND ', serendipity_fetchCategoryRange($categoryid));
        }
    }

    return implode(' OR ', $cat_sql_array);
}

function serendipity_fetchCategoryInfo($categoryid, $categoryname = '') {
    global $serendipity;

    if (!empty($categoryname)) {
        $query = "SELECT
                         c.authorid,
                         c.categoryid,
                         c.category_name,
                         c.category_description,
                         c.category_icon,
                         c.parentid
                    FROM {$serendipity['dbPrefix']}category AS c
                   WHERE category_name = '" . serendipity_db_escape_string($categoryname) . "'";

        $ret = serendipity_db_query($query);
        return $ret[0];
    } else {
        $query = "SELECT
                         c.authorid,
                         c.categoryid,
                         c.category_name,
                         c.category_description,
                         c.category_icon,
                         c.parentid
                    FROM {$serendipity['dbPrefix']}category AS c
                   WHERE categoryid = " . (int)$categoryid;

        $ret = serendipity_db_query($query);
        return $ret[0];
    }
}

function serendipity_fetchEntryCategories($entryid) {
  global $serendipity;

    if (is_numeric($entryid)) {
        $query = "SELECT
                         c.categoryid,
                         c.category_name,
                         c.category_description,
                         c.category_icon,
                         c.parentid
                    FROM {$serendipity['dbPrefix']}category AS c
              INNER JOIN {$serendipity['dbPrefix']}entrycat AS ec
                      ON ec.categoryid = c.categoryid
                   WHERE ec.entryid = {$entryid}";

        $cat = serendipity_db_query($query);
        if (!is_array($cat)) {
            return array();
        } else {
            return $cat;
        }
    }
}


/**
* Give it a range in YYYYMMDD format to gather the desired entries
* (For february 2002 you would pass 200202 for all entries withing
* two timestamps you would pass array(timestamp1,timestamp2)
**/
function serendipity_fetchEntries($range = null, $full = true, $limit = '', $fetchDrafts = false, $modified_since = false, $orderby = 'timestamp DESC', $filter_sql = '', $noCache = false, $noSticky = false) {
    global $serendipity;

    $cond = array();
    $cond['orderby'] = $orderby;
    if (isset($serendipity['short_archives']) && $serendipity['short_archives']) {
        // In the short listing of all titles for a month, we don't want to have a limit applied. And we don't need/want toe
        // full article body (consumes memory)
        $limit   = '';
        $full    = false;
    }

    if ($full === true) {
        $noCache = true; // So no entryproperties related to body/extended caching will be loaded
        $body = ', e.body, e.extended';
    } else {
        $body = '';
    }

    if ($fetchDrafts === false) {
        $drafts = "isdraft = 'false'";
    }

    if ($limit != '') {
        $serendipity['fetchLimit'] = $limit;
    }

    /* Attempt to grab range from $serendipity, if $range is not an array or null */
    if (!is_array($range) && !is_null($range) && isset($serendipity['range'])) {
        $range = $serendipity['range'];
    }

    if (is_numeric($range)) {
        $year  = (int)substr($range, 0, 4);
        $month = (int)substr($range, 4, 2);
        $day   = (int)substr($range, 6, 2);

        $startts = serendipity_serverOffsetHour(mktime(0, 0, 0, $month, ($day == 0 ? 1 : $day), $year), true);

        if ($day == 0) {
            $month++;
        } else {
            $day++;
        }

        $endts = serendipity_serverOffsetHour(mktime(0, 0, 0, $month, ($day == 0 ? 1 : $day), $year), true);

        $cond['and'] = " WHERE timestamp >= $startts AND timestamp <= $endts";
    } elseif (is_array($range) && count($range)==2) {
        $startts = serendipity_serverOffsetHour((int)$range[0], true);
        $endts   = serendipity_serverOffsetHour((int)$range[1], true);
        $cond['and'] = " WHERE timestamp >= $startts AND timestamp <= $endts";
    } else {
        if ($modified_since) {
            $unix_modified = strtotime($modified_since);
            if ($unix_modified != -1) {
                $cond['and'] = ' WHERE last_modified >= ' . (int)$unix_modified;
                if (!empty($limit)) {
                    $limit = ($limit > $serendipity['max_fetch_limit'] ? $limit : $serendipity['max_fetch_limit']);
                }
                $cond['orderby'] = 'last_modified DESC';
            }
        }
    }

    if (!empty($drafts)) {
        if (!empty($cond['and'])) {
            $cond['and'] .= " AND $drafts";
        } else {
            $cond['and'] = "WHERE $drafts";
        }
    }

    if (isset($serendipity['GET']['viewAuthor'])) {
        $cond['and'] .= " AND e.authorid = ". (int)$serendipity['GET']['viewAuthor'];
    }


    if (isset($serendipity['GET']['category'])) {
        $cat_sql = serendipity_getMultiCategoriesSQL($serendipity['GET']['category']);
        if (!empty($cond['and'])) {
            $cond['and'] .= " AND ($cat_sql)";
        } else {
            $cond['and'] = "WHERE ($cat_sql)";
        }
    }

    if (!empty($limit)) {
        if (isset($serendipity['GET']['page']) && $serendipity['GET']['page'] > 1 && !strstr($limit, ',')) {
            $limit = serendipity_db_limit(($serendipity['GET']['page']-1) * $limit, $limit);
        }

        $limit = serendipity_db_limit_sql($limit);
    }

    if (isset($serendipity['GET']['adminModule']) && $serendipity['GET']['adminModule'] == 'entries' && !serendipity_checkPermission('adminEntriesMaintainOthers')) {
        if (!empty($cond['and'])) {
            $cond['and'] .= " AND e.authorid = '" . $serendipity['authorid'] . "'";
        } else {
            $cond['and'] = "WHERE e.authorid = '" . $serendipity['authorid'] . "'";
        }
    }

    if (!isset($serendipity['GET']['adminModule']) && !serendipity_db_bool($serendipity['showFutureEntries'])) {
        if (!empty($cond['and'])) {
            $cond['and'] .= " AND e.timestamp <= '" . time() . "'";
        } else {
            $cond['and'] = "WHERE e.timestamp <= '" . time() . "'";
        }
    }

    if (!empty($filter_sql)) {
        if (!empty($cond['and'])) {
            $cond['and'] .= ' AND ' . $filter_sql;
        } else {
            $cond['and'] = 'WHERE ' . $filter_sql;
        }
    }

    serendipity_plugin_api::hook_event('frontend_fetchentries', $cond, array('noCache' => $noCache, 'noSticky' => $noSticky, 'source' => 'entries'));

    if ($serendipity['dbType'] == 'postgres') {
        $group    = '';
        $distinct = 'DISTINCT';
    } else {
        $group    = 'GROUP BY e.id';
        $distinct = '';
    }
    
    serendipity_ACL_SQL($cond);

    // Store the unique query condition for entries for later reference, like getting the total article count.
    $serendipity['fullCountQuery'] = "
                FROM
                    {$serendipity['dbPrefix']}entries AS e
                    LEFT JOIN {$serendipity['dbPrefix']}authors a
                        ON e.authorid = a.authorid
                    LEFT JOIN {$serendipity['dbPrefix']}entrycat ec
                        ON e.id = ec.entryid
                    LEFT JOIN {$serendipity['dbPrefix']}category c
                        ON ec.categoryid = c.categoryid
                    {$cond['joins']}
                    {$cond['and']}";

    $query = "SELECT $distinct
                    {$cond['addkey']}

                    e.id,
                    e.title,
                    e.timestamp,
                    e.comments,
                    e.exflag,
                    e.authorid,
                    e.trackbacks,
                    e.isdraft,
                    e.allow_comments,
                    e.last_modified,

                    a.realname AS author,
                    a.email

                    $body
                    {$serendipity['fullCountQuery']}
                    $group
                    ORDER BY {$cond['orderby']}
                    $limit";

    // DEBUG:
    // die($query);
    $ret = serendipity_db_query($query, false, 'assoc');

    if (is_string($ret)) {
        die("Query failed: $ret");
    }

    if (is_array($ret)) {
        // The article's query LIMIT operates on a flattened entries layer so that
        // an article having 5 associated categories won't count as 5 entries.
        // But to store the expanded list of categories, we need to send a new
        // query once for all entries we have just fetched.
        // First code for this was sending 15 queries for 15 fetched entries,
        // this is now limited to just one query per fetched articles group

        serendipity_fetchEntryData($ret);
    }

    return $ret;
}

/**
 * Attach special entry data to entry
 */
function serendipity_fetchEntryData(&$ret) {
    global $serendipity;

    $search_ids = array(); // An array to hold all ids of the entry we want to fetch.
    $assoc_ids  = array(); // A temporary key association container to not have to loop through the return array once again.

    foreach($ret AS $i => $entry) {
        $search_ids[]            = $entry['id'];
        $ret[$i]['categories']   = array();        // make sure every article gets its category association
        $assoc_ids[$entry['id']] = $i;             // store temporary reference
    }

    serendipity_plugin_api::hook_event('frontend_entryproperties', $ret, $assoc_ids);

    $query = "SELECT
                     ec.entryid,
                     c.categoryid,
                     c.category_name,
                     c.category_description,
                     c.category_icon,
                     c.parentid
                FROM {$serendipity['dbPrefix']}category AS c
           LEFT JOIN {$serendipity['dbPrefix']}entrycat AS ec
                  ON ec.categoryid = c.categoryid
               WHERE " . serendipity_db_in_sql('ec.entryid', $search_ids);

    $search_ret = serendipity_db_query($query, false, 'assoc');

    if (is_array($search_ret)) {
        foreach($search_ret AS $i => $entry) {
            $ret[$assoc_ids[$entry['entryid']]]['categories'][] = $entry;
        }
    }
}

/**
* Fetches a specific entry
**/
function serendipity_fetchEntry($key, $val, $full = true, $fetchDrafts = 'false') {
    global $serendipity;

    $cond = array();
    $cond['and'] = " "; // intentional dummy string to attach dummy AND parts to the WHERE clauses

    if ($fetchDrafts == 'false') {
        $cond['and'] = " AND e.isdraft = 'false' " . (!serendipity_db_bool($serendipity['showFutureEntries']) ? " AND e.timestamp <= " . time() : '');
    }

    if (isset($serendipity['GET']['adminModule']) && $serendipity['GET']['adminModule'] == 'entries' && !serendipity_checkPermission('adminEntriesMaintainOthers')) {
        $cond['and'] = " AND e.authorid = '" . $serendipity['authorid'] . "'";
    }

    serendipity_ACL_SQL($cond, true);

    serendipity_plugin_api::hook_event('frontend_fetchentry', $cond, array('noSticky' => true));

    $querystring = "SELECT  e.id,
                            e.title,
                            e.timestamp,
                            e.body,
                            e.comments,
                            e.trackbacks,
                            e.extended,
                            e.exflag,
                            e.authorid,
                            e.isdraft,
                            e.allow_comments,
                            e.last_modified,
                            e.moderate_comments,

                            a.realname AS author,
                            a.email
                      FROM
                            {$serendipity['dbPrefix']}entries e
                 LEFT JOIN  {$serendipity['dbPrefix']}authors a
                        ON  e.authorid = a.authorid
                            {$cond['joins']}
                     WHERE
                            e.$key LIKE '" . serendipity_db_escape_string($val) . "'
                            {$cond['and']}
                     LIMIT  1";

    $ret = serendipity_db_query($querystring, true, 'assoc');

    if (is_array($ret)) {
        $ret['categories'] = serendipity_fetchEntryCategories($ret['id']);
        $ret['properties'] = serendipity_fetchEntryProperties($ret['id']);
    }

    return $ret;
}

function serendipity_fetchEntryProperties($id) {
    global $serendipity;

    $parts = array();
    serendipity_plugin_api::hook_event('frontend_entryproperties_query', $parts);

    $properties = serendipity_db_query("SELECT property, value FROM {$serendipity['dbPrefix']}entryproperties WHERE entryid = " . (int)$id . " " . $parts['and']);
    if (!is_array($properties)) {
        $properties = array();
    }

    $property = array();
    foreach($properties AS $idx => $row) {
        $property[$row['property']] = $row['value'];
    }

    return $property;
}

/**
* Fetches a users categories
**/
function serendipity_fetchCategories($authorid = null, $name = null, $order = null, $artifact_mode = 'write') {
    global $serendipity;

    if ($name === null) {
        $name = '';
    }

    if ($order === null) {
        $order = 'category_name ASC';
    }

    if (!isset($authorid) || $authorid === null) {
        $authorid = ((isset($serendipity['authorid']) && !empty($serendipity['GET']['adminModule'])) ? $serendipity['authorid'] : 1);
    }

    if (isset($serendipity['authorid']) && !empty($serendipity['GET']['adminModule']) && $authorid != $serendipity['authorid'] && !serendipity_checkPermission('adminCategoriesMaintainOthers')) {
        $authorid = $serendipity['authorid'];
    }

    $where = '';
    if ($authorid != 'all' && is_numeric($authorid)) {
        if (!serendipity_checkPermission('adminCategoriesMaintainOthers', $authorid)) {
            $where = " WHERE (c.authorid = $authorid OR c.authorid = 0)";
            $where .= "OR (
                          acl.artifact_type = 'category'
                          AND acl.artifact_mode = '" . serendipity_db_escape_string($artifact_mode) . "'
                         )";

        }
    } else {
        $where = '';
    }

    if (!empty($name)) {
        if ($where == '') {
            $where = ' WHERE ';
        } else {
            $where = ' AND ';
        }

        $where .= " c.category_name = '" . serendipity_db_escape_string($name) . "'";
    }

    if ($serendipity['dbType'] == 'postgres') {
        $group    = '';
        $distinct = 'DISTINCT';
    } else {
        $group    = 'GROUP BY c.categoryid';
        $distinct = '';
    }

    $querystring = "SELECT $distinct c.categoryid,
                           c.category_name,
                           c.category_icon,
                           c.category_description,
                           c.authorid,
                           c.category_left,
                           c.category_right,
                           c.parentid,

                           a.username,
                           a.realname
                      FROM {$serendipity['dbPrefix']}category AS c
           LEFT OUTER JOIN {$serendipity['dbPrefix']}authors AS a
                        ON c.authorid = a.authorid 
           LEFT OUTER JOIN {$serendipity['dbPrefix']}authorgroups AS ag
                        ON ag.authorid = a.authorid
           LEFT OUTER JOIN {$serendipity['dbPrefix']}access AS acl 
                        ON (ag.groupid = acl.groupid AND acl.artifact_id = c.categoryid)
                           $where
                           $group";

    if (!empty($order)) {
        $querystring .= "\n ORDER BY $order";
    }

    return serendipity_db_query($querystring);
}

function serendipity_rebuildCategoryTree($parent = 0, $left = 0) {
    // Based on http://www.sitepoint.com/article/hierarchical-data-database/1
    global $serendipity;
    $right = $left + 1;

    $result = serendipity_db_query("SELECT categoryid FROM {$serendipity['dbPrefix']}category WHERE parentid = '" . (int)$parent . "'");
    if ( is_array($result) ) {
        foreach ( $result as $category ) {
            $right = serendipity_rebuildCategoryTree($category['categoryid'], $right);
        }
    }
    if ( $parent > 0 ) {
        serendipity_db_query("UPDATE {$serendipity['dbPrefix']}category SET category_left='{$left}', category_right='{$right}' WHERE categoryid='{$parent}'");
    }

    return $right + 1;
}

/**
* Give it a raw searchstring, it'll search
**/
function serendipity_searchEntries($term, $limit = '') {
    global $serendipity;

    if ($limit == '') {
        $limit = $serendipity['fetchLimit'];
    }

    if (isset($serendipity['GET']['page']) && $serendipity['GET']['page'] > 1 && !strstr($limit, ',')) {
        $limit = serendipity_db_limit(($serendipity['GET']['page']-1) * $limit, $limit);
    }

    $limit = serendipity_db_limit_sql($limit);

    $term = serendipity_db_escape_string($term);
    if ($serendipity['dbType'] == 'postgres') {
        $group     = '';
        $distinct  = 'DISTINCT';
        $find_part = "(title ILIKE '%$term%' OR body ILIKE '%$term%' OR extended ILIKE '%$term%')";
    } elseif ($serendipity['dbType'] == 'sqlite') {
        // Very extensive SQLite search. There currently seems no other way to perform fulltext search in SQLite
        // But it's better than no search at all :-D
        $group     = 'GROUP BY e.id';
        $distinct  = '';
        $term      = serendipity_mb('strtolower', $term);
        $find_part = "(lower(title) LIKE '%$term%' OR lower(body) LIKE '%$term%' OR lower(extended) LIKE '%$term%')";
    } else {
        $group     = 'GROUP BY e.id';
        $distinct  = '';
        $term      = str_replace('&quot;', '"', $term);
        if (preg_match('@["\+\-\*~<>\(\)]+@', $term)) {
            $find_part = "MATCH(title,body,extended) AGAINST('$term' IN BOOLEAN MODE)";
        } else {
            $find_part = "MATCH(title,body,extended) AGAINST('$term')";
        }
    }

    $cond = array();
    $cond['and'] = " AND isdraft = 'false' " . (!serendipity_db_bool($serendipity['showFutureEntries']) ? " AND timestamp <= " . time() : '');
    serendipity_plugin_api::hook_event('frontend_fetchentries', $cond, array('source' => 'search'));

    serendipity_ACL_SQL($cond, 'limited');

    $serendipity['fullCountQuery'] = "
                      FROM
                            {$serendipity['dbPrefix']}entries e
                 LEFT JOIN  {$serendipity['dbPrefix']}authors a
                        ON  e.authorid = a.authorid
                 LEFT JOIN  {$serendipity['dbPrefix']}entrycat ec
                        ON  e.id = ec.entryid
                            {$cond['joins']}
                     WHERE
                            $find_part
                            {$cond['and']}";

    $querystring = "SELECT $distinct
                            e.id,
                            e.authorid,
                            a.realname AS author,
                            a.email,
                            ec.categoryid,
                            e.timestamp,
                            e.comments,
                            e.title,
                            e.body,
                            e.extended,
                            e.trackbacks,
                            e.exflag
                    {$serendipity['fullCountQuery']}
                    $group
                  ORDER BY  timestamp DESC
                    $limit";

    $search = serendipity_db_query($querystring);

    if (is_array($search)) {
        serendipity_fetchEntryData($search);
    }

    return $search;
}

function serendipity_printEntryFooter() {
    global $serendipity;

    $totalEntries = serendipity_getTotalEntries();
    $totalPages   = ceil($totalEntries / $serendipity['fetchLimit']);

    if (!isset($serendipity['GET']['page'])) {
        $serendipity['GET']['page'] = 1;
    }

    if ($totalPages <= 0 ) {
        $totalPages = 1;
    }

    if ($serendipity['GET']['page'] > 1) {
        $uriArguments = $serendipity['uriArguments'];
        $uriArguments[] = 'P'. ($serendipity['GET']['page'] - 1);
        $serendipity['smarty']->assign('footer_prev_page', serendipity_rewriteURL(implode('/', $uriArguments) .'.html'));
    }

    $uriArguments = $serendipity['uriArguments'];
    $uriArguments[] = 'P%s';
    $serendipity['smarty']->assign('footer_totalEntries', $totalEntries);
    $serendipity['smarty']->assign('footer_totalPages', $totalPages);
    $serendipity['smarty']->assign('footer_currentPage', $serendipity['GET']['page']);
    $serendipity['smarty']->assign('footer_pageLink', serendipity_rewriteURL(implode('/', $uriArguments) . '.html'));
    $serendipity['smarty']->assign('footer_info', sprintf(PAGE_BROWSE_ENTRIES, (int)$serendipity['GET']['page'], $totalPages, $totalEntries));

    if ($serendipity['GET']['page'] < $totalPages) {
        $uriArguments = $serendipity['uriArguments'];
        $uriArguments[] = 'P'. ($serendipity['GET']['page'] + 1);
        $serendipity['smarty']->assign('footer_next_page', serendipity_rewriteURL(implode('/', $uriArguments) .'.html'));
    }
}

function serendipity_getTotalEntries() {
    global $serendipity;

    // The unique query condition was built previously in serendipity_fetchEntries()
    if ($serendipity['dbType'] == 'sqlite') {
        $querystring  = "SELECT count(e.id) {$serendipity['fullCountQuery']} GROUP BY e.id";
    } else {
        $querystring  = "SELECT count(distinct e.id) {$serendipity['fullCountQuery']}";
    }

    $query = serendipity_db_query($querystring);

    if (is_array($query) && isset($query[0])) {
        if ($serendipity['dbType'] == 'sqlite') {
            return count($query);
        } else {
            return $query[0][0];
        }
    }

    return 0;
}

/**
* Prints the entries you fetched with serendipity_fetchEntries/searchEntries in HTML.
**/
function serendipity_printEntries($entries, $extended = 0, $preview = false) {
    global $serendipity;

    $addData = array('extended' => $extended, 'preview' => $preview);
    serendipity_plugin_api::hook_event('entry_display', $entries, $addData);

    if (isset($entries['clean_page']) && $entries['clean_page'] === true) {
        $serendipity['smarty']->assign('plugin_clean_page', true);
        serendipity_smarty_fetch('ENTRIES', 'entries.tpl', true);
        return; // no display of this item
    }

    // We shouldn't return here, because we want Smarty to handle the output
    if (!is_array($entries) || $entries[0] == false || !isset($entries[0]['timestamp'])) {
        $entries = array();
    }

    $dategroup = array();
    for ($x = 0, $num_entries = count($entries); $x < $num_entries; $x++) {
        if (!empty($entries[$x]['properties']['ep_is_sticky']) && serendipity_db_bool($entries[$x]['properties']['ep_is_sticky'])) {
            $entries[$x]['is_sticky'] = true;
            $key = 'sticky';
        } else {
            $key = date('Ymd', serendipity_serverOffsetHour($entries[$x]['timestamp']));
        }

        if (!empty($entries[$x]['properties']['ep_cache_body'])) {
            $entries[$x]['body']      = &$entries[$x]['properties']['ep_cache_body'];
            $entries[$x]['is_cached'] = true;
        }

        if (!empty($entries[$x]['properties']['ep_cache_extended'])) {
            $entries[$x]['extended']  = &$entries[$x]['properties']['ep_cache_extended'];
            $entries[$x]['is_cached'] = true;
        }

        $dategroup[$key]['date']        = $entries[$x]['timestamp'];
        $dategroup[$key]['is_sticky']   = (isset($entries[$x]['is_sticky']) && serendipity_db_bool($entries[$x]['is_sticky']) ? true : false);
        $dategroup[$key]['entries'][]   = &$entries[$x];
    }

    foreach($dategroup as $properties) {
        foreach($properties['entries'] as $x => $_entry) {
            $entry = &$properties['entries'][$x]; // PHP4 Compat
            if ($preview) {
                $entry['author']   = $entry['realname'];
                $entry['authorid'] = $serendipity['authorid'];
            }

            serendipity_plugin_api::hook_event('frontend_display', $entry);

            if ($preview) {
                $entry['author']   = $entry['realname'];
                $entry['authorid'] = $serendipity['authorid'];
            }

            $authorData = array(
                            'authorid' => $entry['authorid'], 
                            'username' => $entry['author'],
                            'email'    => $entry['email'],
                            'realname' => $entry['author']
            );

            $entry['link']      = serendipity_archiveURL($entry['id'], $entry['title'], 'serendipityHTTPPath', true, array('timestamp' => $entry['timestamp']));
            $entry['commURL']   = serendipity_archiveURL($entry['id'], $entry['title'], 'baseURL', false, array('timestamp' => $entry['timestamp']));
            $entry['rdf_ident'] = serendipity_archiveURL($entry['id'], $entry['title'], 'baseURL', true, array('timestamp' => $entry['timestamp']));
            $entry['title']     = htmlspecialchars($entry['title']);

            $entry['link_allow_comments']    = $serendipity['baseURL'] . 'comment.php?serendipity[switch]=enable&amp;serendipity[entry]=' . $entry['id'];
            $entry['link_deny_comments']     = $serendipity['baseURL'] . 'comment.php?serendipity[switch]=disable&amp;serendipity[entry]=' . $entry['id'];
            $entry['allow_comments']         = serendipity_db_bool($entry['allow_comments']);
            $entry['moderate_comments']      = serendipity_db_bool($entry['moderate_comments']);
            $entry['viewmode']               = ($serendipity['GET']['cview'] == VIEWMODE_LINEAR ? VIEWMODE_LINEAR : VIEWMODE_THREADED);
            $entry['link_popup_comments']    = $serendipity['serendipityHTTPPath'] .'comment.php?serendipity[entry_id]='. $entry['id'] .'&amp;serendipity[type]=comments';
            $entry['link_popup_trackbacks']  = $serendipity['serendipityHTTPPath'] .'comment.php?serendipity[entry_id]='. $entry['id'] .'&amp;serendipity[type]=trackbacks';
            $entry['link_edit']              = $serendipity['baseURL'] .'serendipity_admin.php?serendipity[action]=admin&amp;serendipity[adminModule]=entries&amp;serendipity[adminAction]=edit&amp;serendipity[id]='. $entry['id'];
            $entry['link_trackback']         = $serendipity['baseURL'] .'comment.php?type=trackback&amp;entry_id='. $entry['id'];
            $entry['link_rdf']               = serendipity_rewriteURL(PATH_FEEDS . '/ei_'. $entry['id'] .'.rdf');
            $entry['link_viewmode_threaded'] = $serendipity['serendipityHTTPPath'] . $serendipity['indexFile'] .'?url='. $entry['commURL'] .'&amp;serendipity[cview]='. VIEWMODE_THREADED;
            $entry['link_viewmode_linear']   = $serendipity['serendipityHTTPPath'] . $serendipity['indexFile'] .'?url='. $entry['commURL'] .'&amp;serendipity[cview]='. VIEWMODE_LINEAR;
            $entry['link_author']            = serendipity_authorURL($authorData);

            if (is_array($entry['categories'])) {
                foreach ($entry['categories'] as $k => $v) {
                    $entry['categories'][$k]['category_link'] =  serendipity_categoryURL($entry['categories'][$k]);
                }
            }

            if (strlen($entry['extended'])) {
                $entry['has_extended']      = true;
            }

            if (isset($entry['exflag']) && $entry['exflag'] && ($extended || $preview)) {
                $entry['is_extended']       = true;
            }

            if (serendipity_db_bool($entry['allow_comments']) || !isset($entry['allow_comments']) || $entry['comments'] > 0) {
                $entry['has_comments']      = true;
                $entry['label_comments']    = $entry['comments'] == 1 ? COMMENT : COMMENTS;
            }

            if (serendipity_db_bool($entry['allow_comments']) || !isset($entry['allow_comments']) || $entry['trackbacks'] > 0) {
                $entry['has_trackbacks']    = true;
                $entry['label_trackbacks']  = $entry['trackbacks'] == 1 ? TRACKBACK : TRACKBACKS;
            }

            if ($_SESSION['serendipityAuthedUser'] === true && ($_SESSION['serendipityAuthorid'] == $entry['authorid'] || serendipity_checkPermission('adminEntriesMaintainOthers'))) {
                $entry['is_entry_owner']    = true;
            }

            $entry['display_dat'] = '';
            serendipity_plugin_api::hook_event('frontend_display:html:per_entry', $entry);
            $entry['plugin_display_dat'] =& $entry['display_dat'];

            if ($preview) {
                ob_start();
                serendipity_plugin_api::hook_event('backend_preview', $entry);
                $entry['backend_preview'] = ob_get_contents();
                ob_end_clean();
            }

            /* IF WE ARE DISPLAYING A FULL ENTRY */
            if (isset($serendipity['GET']['id'])) {
                $serendipity['smarty']->assign(
                    array(
                        'comments_messagestack' => (isset($serendipity['messagestack']['comments']) ? (array)$serendipity['messagestack']['comments'] : array()),
                        'is_comment_added'      => (isset($serendipity['GET']['csuccess']) && $serendipity['GET']['csuccess'] == 'true' ? true: false),
                        'is_comment_moderate'   => (isset($serendipity['GET']['csuccess']) && $serendipity['GET']['csuccess'] == 'moderate' ? true: false)
                    )
                );

                serendipity_displayCommentForm(
                    $entry['id'],
                    $serendipity['serendipityHTTPPath'] . $serendipity['indexFile'] . '?url=' . $entry['commURL'],
                    true,
                    $serendipity['POST'],
                    true,
                    serendipity_db_bool($entry['moderate_comments']),
                    $entry
                );
            } // END FULL ENTRY LOGIC
        } // end foreach-loop (entries)
    } // end foreach-loop (dates)

    if (!isset($serendipity['GET']['id']) &&
            (!isset($serendipity['hidefooter']) || $serendipity['hidefooter'] == false) &&
            ($num_entries <= $serendipity['fetchLimit'])) {
        serendipity_printEntryFooter();
    }

    $serendipity['smarty']->assign('entries', $dategroup);
    unset($entries, $dategroup);

    if (isset($serendipity['short_archives']) && $serendipity['short_archives']) {
        serendipity_smarty_fetch('ENTRIES', 'entries_summary.tpl', true);
    } else {
        serendipity_smarty_fetch('ENTRIES', 'entries.tpl', true);
    }

} // end function serendipity_printEntries

/**
/**
 * purge a statically pregenerated entry
 */
function serendipity_purgeEntry($id, $timestamp = null) {
    global $serendipity;

    // If pregenerate is not set, short circuit all this logic
    // and remove nothing.
    if(!isset($serendipity['pregenerate'])) {
        return;
    }

    if (isset($timestamp)) {
        $dated = date('Ymd', serendipity_serverOffsetHour($timestamp));
        $datem = date('Ym',  serendipity_serverOffsetHour($timestamp));

        @unlink("{$serendipity['serendipityPath']}/".PATH_ARCHIVES."/{$dated}.html");
        @unlink("{$serendipity['serendipityPath']}/".PATH_ARCHIVES."/{$datem}.html");
    }

    // Fixme (the _* part) !
    @unlink("{$serendipity['serendipityPath']}/".PATH_ARCHIVES."/{$id}_*.html");
    @unlink("{$serendipity['serendipityPath']}/".PATH_FEEDS."/index.rss");
    @unlink("{$serendipity['serendipityPath']}/".PATH_FEEDS."/index.rss2");
    @unlink("{$serendipity['serendipityPath']}/index.html");
}

/**
* Inserts a new entry into the database or updates an existing
**/
function serendipity_updertEntry($entry) {
    global $serendipity;

    include_once S9Y_INCLUDE_PATH . 'include/functions_entries_admin.inc.php';

    $errors = array();
    serendipity_plugin_api::hook_event('backend_entry_updertEntry', $errors, $entry);
    if (count($errors) > 0) {
        // Return error message(s)
        return implode("\n", $errors);
    }

    serendipity_plugin_api::hook_event('backend_entry_presave', $entry);

    $categories = $entry['categories'];
    unset($entry['categories']);

    $newEntry = 0;
    $exflag = 0;

    if (!is_numeric($entry['timestamp'])) {
        $entry['timestamp'] = time();
    }

    if (!isset($entry['last_modified']) || !is_numeric($entry['last_modified'])) {
        $entry['last_modified'] = time();
    }

    /* WYSIWYG-editor inserts empty ' ' for extended body; this is reversed here */
    if (isset($entry['extended']) && (trim($entry['extended']) == '' || trim($entry['extended']) == '<br />' || trim($entry['extended']) == '<p></p>')) {
        $entry['extended'] = '';
    }

    if (strlen($entry['extended'])) {
        $exflag = 1;
    }

    $entry['exflag']   = $exflag;

    if (!is_numeric($entry['id'])) {
        /* we need to insert */

        unset($entry['id']);
        $entry['comments'] = 0;

        // New entries need an author
        $entry['author']   = $serendipity['user'];
        if (!isset($entry['authorid']) || empty($entry['authorid'])) {
            $entry['authorid'] = $serendipity['authorid'];
        }

        if (!$_SESSION['serendipityRightPublish']) {
            $entry['isdraft'] = 'true';
        }

        if(!isset($entry['allow_comments'])){
            $entry['allow_comments']='false';
        }
        if(!isset($entry['moderate_comments'])){
            $entry['moderate_comments']='false';
        }

        $res = serendipity_db_insert('entries', $entry);

        if ($res) {
            $entry['id'] = $serendipity['lastSavedEntry'] = serendipity_db_insert_id('entries', 'id');
            if (is_array($categories)) {
                foreach ($categories as $cat) {
                    if (is_numeric($cat)) {
                        serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}entrycat (entryid, categoryid) VALUES ({$entry['id']}, {$cat})");
                    }
                }
            }

            serendipity_insertPermalink($entry);
        } else {
            //Some error message here
            return ENTRIES_NOT_SUCCESSFULLY_INSERTED;
        }
        $newEntry    = 1;
    } else {
        /* we need to update */

        // Get settings from entry if already in DB, which should not be alterable with POST methods
        $_entry            = serendipity_fetchEntry('id', $entry['id'], 1, 1);
        $entry['authorid'] = $_entry['authorid'];

        if (isset($serendipity['GET']['adminModule']) && $serendipity['GET']['adminModule'] == 'entries' && $entry['authorid'] != $serendipity['authorid'] && !serendipity_checkPermission('adminEntriesMaintainOthers')) {
            // Only chiefs and admins can change other's entry. Else update fails.
            return;
        }

        if (!$_SESSION['serendipityRightPublish']) {
            unset($entry['isdraft']);
        }

        if (is_array($categories)) {
            serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}entrycat WHERE entryid={$entry['id']}");
            foreach ($categories as $cat) {
                serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}entrycat (entryid, categoryid) VALUES ({$entry['id']}, {$cat})");
            }
        }

        $res = serendipity_db_update('entries', array('id' => $entry['id']), $entry);
        $newEntry = 0;
        serendipity_updatePermalink($entry);
    }

    if (is_string($res)) {
        return $res;
    }

    if (!serendipity_db_bool($entry['isdraft'])) {
        serendipity_plugin_api::hook_event('frontend_display', $entry, array('no_scramble' => true));
        serendipity_handle_references($entry['id'], $serendipity['blogTitle'], $entry['title'], $entry['body'] . $entry['extended'], $newEntry);
    }

    serendipity_purgeEntry($entry['id'], $entry['timestamp']);

    // Send publish tags if either a new article has been inserted from scratch, or if the entry was previously
    // stored as draft and is now published
    if (!serendipity_db_bool($entry['isdraft']) && ($newEntry || serendipity_db_bool($_entry['isdraft']))) {
        serendipity_plugin_api::hook_event('backend_publish', $entry, $newEntry);
    } else {
        serendipity_plugin_api::hook_event('backend_save', $entry, $newEntry);
    }

    return (int)$entry['id'];
}

/**
* Deletes an entry and everything that belongs to it (comments, etc...) from
* the database
**/
function serendipity_deleteEntry($id) {
    global $serendipity;

    if (!is_numeric($id)) {
        return false;
    }

    // Purge the daily/monthly entries so they can be rebuilt
    $result = serendipity_db_query("SELECT timestamp, authorid FROM {$serendipity['dbPrefix']}entries WHERE id = '". (int)$id ."'", true);

    if ($result[1] != $serendipity['authorid'] && !serendipity_checkPermission('adminEntriesMaintainOthers')) {
        // Only admins and chief users can delete entries which do not belong to the author
        return;
    }

    serendipity_purgeEntry($id, $result[0]);

    serendipity_db_query("DELETE FROM {$serendipity["dbPrefix"]}entries WHERE id=$id");
    serendipity_db_query("DELETE FROM {$serendipity["dbPrefix"]}entrycat WHERE entryid=$id");
    serendipity_db_query("DELETE FROM {$serendipity["dbPrefix"]}entryproperties WHERE entryid=$id");
    serendipity_db_query("DELETE FROM {$serendipity["dbPrefix"]}comments WHERE entry_id=$id");
    serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}references WHERE entry_id='$id'");
    serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}permalinks WHERE entry_id='$id'");
}

/**
* Print a list of categories
*
* Prints a list of categories for use in forms, the sidebar, or whereever...
* @param array  An array of categories, typically gathered by serendipity_fetchCategories()
* @param array  Select
* @param int    Type
* @param int    ID
* @param int    Level
* @param string Tells the function, whether or not to display the XML button for each category.
*               If empty, no links to the xml feeds will be displayed; If you want to, you can
*               pass an image here (this setting is only used, when type==3).
* @see serendipity_fetchCategories()
*/
function serendipity_generateCategoryList($cats, $select = array(0), $type = 0, $id = 0, $level = 0, $xmlImg = '') {
    global $serendipity;

    if ( !is_array($cats) || !count($cats) )
        return;

    $ret = '';
    foreach ($cats as $cat) {
        if ($cat['parentid'] == $id) {
            switch ($type) {
                case 0:
                    $ret .= str_repeat('&nbsp;', $level * 2).'&bull;&nbsp;<span id="catItem_' . $cat['categoryid'] . '"' . (($cat['categoryid'] && in_array($cat['categoryid'], $select)) ? ' selected="selected"' : '') . '><a href="?serendipity[adminModule]=category&amp;serendipity[cat][catid]=' . $cat['categoryid'] . '">' . (!empty($cat['category_icon']) ? '<img style="vertical-align: middle;" src="' . $cat['category_icon'] . '" border="0" alt="' . $cat['category_name'] . '"/> ' : '') . htmlspecialchars($cat['category_name']) . (!empty($cat['category_description']) ? ' - ' . htmlspecialchars($cat['category_description']) : '') . '</a></span><br/>' . "\n";
                    break;
                case 1:
                case 2:
                   $ret .= '<option value="' . $cat['categoryid'] . '"' . (($cat['categoryid'] && in_array($cat['categoryid'], $select)) ? ' selected="selected"' : '') . '>';
                   $ret .= str_repeat('&nbsp;', $level * 2) . htmlspecialchars($cat['category_name']) . ($type == 1 && !empty($cat['category_description']) ? (' - ' . htmlspecialchars($cat['category_description'])) : '');
                   $ret .= '</option>';
                   break;
                case 3:
                    $category_id = serendipity_makeFilename($cat['category_name']);
                    if (!empty($xmlImg)) {
                        $ret .= sprintf(
                          '<div style="padding-bottom: 2px;">' .
                          '<a href="%s" title="%s"><img alt="xml" src="%s" style="vertical-align: bottom; display: inline; border: 0px" /></a>&#160;%s' .
                          '<a href="%s" title="%s">%s</a>' .
                          '</div>',
                          $serendipity['serendipityHTTPPath'] . 'rss.php?category=' . $cat['categoryid'] . '_' . $category_id,
                          htmlspecialchars($cat['category_description']),
                          $xmlImg,
                          str_repeat('&#160;', $level * 3),
                          serendipity_categoryURL($cat, 'serendipityHTTPPath'),
                          htmlspecialchars($cat['category_description']),
                          htmlspecialchars($cat['category_name']));
                    } else {
                        $ret .= sprintf(
                          '%s<a href="%s" title="%s">%s</a><br />',
                          str_repeat('&#160;', $level * 3),
                          serendipity_categoryURL($cat, 'serendipityHTTPPath'),
                          htmlspecialchars($cat['category_description']),
                          htmlspecialchars($cat['category_name']));
                    }
                    break;
                case 4:
                    $ret .= $cat['categoryid'] . '|||' . str_repeat(' ', $level * 2) . $cat['category_name'] . '@@@';
                    break;
            }
            $ret .= serendipity_generateCategoryList($cats, $select, $type, $cat['categoryid'], $level + 1, $xmlImg);
        }
    }
    return $ret;
}

function serendipity_updateEntryCategories($postid, $categories) {
    global $serendipity;

    if (!$postid || !$categories) {
        return;
    }

    $query = "DELETE FROM $serendipity[dbPrefix]entrycat WHERE entryid = " . (int)$postid;
    serendipity_db_query($query);

    if (!is_array($categories)) {
        $categories = array(0 => $categories);
    }

    foreach($categories AS $idx => $cat) {
        $query = "INSERT INTO $serendipity[dbPrefix]entrycat (categoryid, entryid) VALUES (" . (int)$cat . ", " . (int)$postid . ")";
        serendipity_db_query($query);
    }
}

function serendipity_printArchives() {
    global $serendipity;

    $f = serendipity_db_query("SELECT timestamp FROM {$serendipity['dbPrefix']}entries ORDER BY timestamp ASC LIMIT 1");
    $lastYear   = date('Y', serendipity_serverOffsetHour($f[0][0]));
    $lastMonth  = date('m', serendipity_serverOffsetHour($f[0][0]));
    $thisYear   = date('Y', serendipity_serverOffsetHour());
    $thisMonth  = date('m', serendipity_serverOffsetHour());
    $max        = 0;

    if (isset($serendipity['GET']['category'])) {
        $cat_sql = serendipity_getMultiCategoriesSQL($serendipity['GET']['category']);
        $cat_get = '/C' . (int)$serendipity['GET']['category'];
    } else {
        $cat_sql = '';
        $cat_get = '';
    }

    $output = array();
    for ($y = $thisYear; $y >= $lastYear; $y--) {
        $output[$y]['year'] = $y;
        for ($m = 12; $m >= 1; $m--) {

            /* If the month we are checking are in the future, we drop it */
            if ($m > $thisMonth && $y == $thisYear) {
                continue;
            }

            /* If the month is lower than the lowest month containing entries, we're done */
            if ($m < $lastMonth && $y <= $lastYear) {
                break;
            }

            $s = serendipity_serverOffsetHour(mktime(0, 0, 0, $m, 1, $y), true);
            $e = serendipity_serverOffsetHour(mktime(23, 59, 59, $m, date('t', $s), $y), true);

            $entries = serendipity_db_query("SELECT count(id) 
                                               FROM {$serendipity['dbPrefix']}entries e
                                          LEFT JOIN {$serendipity['dbPrefix']}entrycat ec
                                                 ON e.id = ec.entryid
                                          LEFT JOIN {$serendipity['dbPrefix']}category c
                                                 ON ec.categoryid = c.categoryid
                                              WHERE isdraft = 'false'
                                                AND timestamp >= $s
                                                AND timestamp <= $e " 
                                                    . (!serendipity_db_bool($serendipity['showFutureEntries']) ? " AND timestamp <= " . time() : '')
                                                    . (!empty($cat_sql) ? ' AND ' . $cat_sql : '')
                                                    );
            $entry_count = $entries[0][0];

            /* A silly hack to get the maximum amount of entries per month */
            if ($entry_count > $max) {
                $max = $entry_count;
            }

            $data = array();
            $data['entry_count']    = $entry_count;
            $data['link']           = serendipity_archiveDateUrl($y . '/'. sprintf('%02s', $m) . $cat_get);
            $data['link_summary']   = serendipity_archiveDateUrl($y . '/'. sprintf('%02s', $m) . $cat_get, true);
            $data['date']           = $s;
            $output[$y]['months'][] = $data;
        }
    }
    $serendipity['smarty']->assign(array('archives' => $output,
                                         'max_entries' => $max));

    serendipity_smarty_fetch('ARCHIVES', 'entries_archives.tpl', true);
}

