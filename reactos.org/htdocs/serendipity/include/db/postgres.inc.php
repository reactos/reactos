<?php # $Id: postgres.inc.php 565 2005-10-17 13:04:11Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

function serendipity_db_begin_transaction(){
    serendipity_db_query('begin work');
}

function serendipity_db_end_transaction($commit){
    if ($commit){
        serendipity_db_query('commit');
    }else{
        serendipity_db_query('rollback');
    }
}

function serendipity_db_in_sql($col, &$search_ids) {
    return $col . " IN (" . implode(', ', $search_ids) . ")";
}

function serendipity_db_connect() {
    global $serendipity;

    if (isset($serendipity['dbPersistent']) && $serendipity['dbPersistent']) {
        $function = 'pg_pconnect';
    } else {
        $function = 'pg_connect';
    }

    $serendipity['dbConn'] = $function(
                               sprintf(
                                 '%sdbname=%s user=%s password=%s',
                                 strlen($serendipity['dbHost']) ? ('host=' . $serendipity['dbHost'] . ' ') : '',
                                 $serendipity['dbName'],
                                 $serendipity['dbUser'],
                                 $serendipity['dbPass']
                               )
                             );

    return $serendipity['dbConn'];
}

function serendipity_db_escape_string($string) {
    return pg_escape_string($string);
}

function serendipity_db_limit($start, $offset) {
    return $offset . ', ' . $start;
}

function serendipity_db_limit_sql($limitstring) {
    $limit_split = split(',', $limitstring);
    if (count($limit_split) > 1) {
        $limit = ' LIMIT ' . $limit_split[0] . ' OFFSET ' . $limit_split[1];
    } else {
        $limit = ' LIMIT ' . $limit_split[0];
    }
    return $limit;
}

function serendipity_db_affected_rows() {
    global $serendipity;
    return pg_affected_rows($serendipity['dbLastResult']);
}

function serendipity_db_updated_rows() {
    global $serendipity;
    // it is unknown whether pg_affected_rows returns number of rows
    //  UPDATED or MATCHED on an UPDATE statement.
    return pg_affected_rows($serendipity['dbLastResult']);
}

function serendipity_db_matched_rows() {
    global $serendipity;
    // it is unknown whether pg_affected_rows returns number of rows
    //  UPDATED or MATCHED on an UPDATE statement.
    return pg_affected_rows($serendipity['dbLastResult']);
}

function serendipity_db_insert_id($table = '', $id = '') {
    global $serendipity;
    if (empty($table) || empty($id)) {
        // BC - will/should never be called with empty parameters!
        return pg_last_oid($serendipity['dbLastResult']);
    } else {
        $query = "SELECT currval('{$serendipity['dbPrefix']}{$table}_{$id}_seq'::text) AS {$id}";
        $res   = pg_query($serendipity['dbConn'], $query);
        if (pg_num_rows($res)) {
            $insert_id = pg_fetch_array($res, 0, PGSQL_ASSOC);
            return $insert_id[$id];
        } else {
            return pg_last_oid($serendipity['dbLastResult']); // BC - should not happen!
        }
    }
}

function &serendipity_db_query($sql, $single = false, $result_type = "both", $reportErr = false, $assocKey = false, $assocVal = false, $expectError = false) {
    global $serendipity;
    static $type_map = array(
                         'assoc' => PGSQL_ASSOC,
                         'num'   => PGSQL_NUM,
                         'both'  => PGSQL_BOTH,
                         'true'  => true,
                         'false' => false
    );

    if (!isset($serendipity['dbPgsqlOIDS'])) {
        $serendipity['dbPgsqlOIDS'] = true;
        @serendipity_db_query('SET default_with_oids = true', true, 'both', false, false, false, true);
    }

    if (!$expectError && ($reportErr || !$serendipity['production'])) {
        $serendipity['dbLastResult'] = pg_query($serendipity['dbConn'], $sql);
    } else {
        $serendipity['dbLastResult'] = @pg_query($serendipity['dbConn'], $sql);
    }

    if (!$serendipity['dbLastResult']) {
        if (!$expectError && !$serendipity['production']) {
            print "Error in $sql<br/>\n";
            print pg_last_error($serendipity['dbConn']) . "<BR/>\n";
            if (function_exists('debug_backtrace')) {
                highlight_string(var_export(debug_backtrace(), 1));
            }
            print "<br><code>$sql</code>\n";
        }
        return $type_map['false'];
    }

    if ($serendipity['dbLastResult'] === true) {
        return $type_map['true'];
    }

    $result_type = $type_map[$result_type];

    $n = pg_num_rows($serendipity['dbLastResult']);

    switch ($n) {
        case 0:
            if ($single) {
                return $type_map['false'];
            }
            return $type_map['true'];
        case 1:
            if ($single) {
                return pg_fetch_array($serendipity['dbLastResult'], 0, $result_type);
            }
        default:
            $rows = array();
            for ($i = 0; $i < $n; $i++) {
                if (!empty($assocKey) && !empty($assocVal)) {
                    // You can fetch a key-associated array via the two function parameters assocKey and assocVal
                    $row = pg_fetch_array($serendipity['dbLastResult'], $i, $result_type);
                    $rows[$row[$assocKey]] = $row[$assocVal];
                } else {
                    $rows[] = pg_fetch_array($serendipity['dbLastResult'], $i, $result_type);
                }
            }
            return $rows;
    }
}

function serendipity_db_schema_import($query) {
    static $search  = array('{AUTOINCREMENT}', '{PRIMARY}', '{UNSIGNED}',
        '{FULLTEXT}', '{BOOLEAN}', 'int(1)', 'int(10)', 'int(11)', 'int(4)', '{UTF_8}');
    static $replace = array('SERIAL', 'primary key', '', '', 'BOOLEAN NOT NULL', 'int2',
        'int4', 'int4', 'int4', '');

    if (stristr($query, '{FULLTEXT_MYSQL}')) {
        return true;
    }

    $query = trim(str_replace($search, $replace, $query));
    if ($query{0} == '@') {
        // Errors are expected to happen (like duplicate index creation)
        return serendipity_db_query(substr($query, 1), false, 'both', false, false, false, true);
    } else {
        return serendipity_db_query($query);
    }
}

function serendipity_db_probe($hash, &$errs) {
    global $serendipity;

    if (!function_exists('pg_connect')) {
        $errs[] = 'No PostgreSQL extension found. Please check your webserver installation or contact your systems administrator regarding this problem.';
        return false;
    }

    $serendipity['dbConn'] = pg_connect(
                               sprintf(
                                 '%sdbname=%s user=%s password=%s',

                                 strlen($hash['dbHost']) ? ('host=' . $hash['dbHost'] . ' ') : '',
                                 $hash['dbName'],
                                 $hash['dbUser'],
                                 $hash['dbPass']
                               )
                             );

    if (!$serendipity['dbConn']) {
        $errs[] = 'Could not connect to database; check your settings.';
        return false;
    }

    return true;
}

function serendipity_db_concat($string) {
    return '(' . str_replace(', ', '||', $string) . ')';
}

/* vim: set sts=4 ts=4 expandtab : */
?>
