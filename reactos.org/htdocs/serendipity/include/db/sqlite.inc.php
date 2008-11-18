<?php # $Id: sqlite.inc.php 565 2005-10-17 13:04:11Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (!function_exists('sqlite_open')) {
    @dl('sqlite.so');
    @dl('sqlite.dll');
}

function serendipity_db_begin_transaction(){
    serendipity_db_query('begin transaction');
}

function serendipity_db_end_transaction($commit){
    if ($commit){
        serendipity_db_query('commit transaction');
    }else{
        serendipity_db_query('rollback transaction');
    }
}

function serendipity_db_connect()
{
    global $serendipity;

    if (isset($serendipity['dbConn'])) {
        return $serendipity['dbConn'];
    }

    if (isset($serendipity['dbPersistent']) && $serendipity['dbPersistent']) {
        $function = 'sqlite_popen';
    } else {
        $function = 'sqlite_open';
    }


    $serendipity['dbConn'] = $function($serendipity['serendipityPath'] . $serendipity['dbName'] . '.db');

    return $serendipity['dbConn'];
}

function serendipity_db_escape_string($string)
{
    static $search  = array("\x00", '%',   "'",   '\"');
    static $replace = array('%00',  '%25', "''", '\\\"');

    return str_replace($search, $replace, $string);
}

function serendipity_db_affected_rows()
{
    global $serendipity;

    return sqlite_changes($serendipity['dbConn']);
}

function serendipity_db_updated_rows()
{
    global $serendipity;
    // It is unknown whether sqllite returns rows MATCHED or rows UPDATED
    return sqlite_changes($serendipity['dbConn']);
}

function serendipity_db_matched_rows()
{
    global $serendipity;
    // It is unknown whether sqllite returns rows MATCHED or rows UPDATED
    return sqlite_changes($serendipity['dbConn']);
}

function serendipity_db_insert_id()
{
    global $serendipity;

    return sqlite_last_insert_rowid($serendipity['dbConn']);
}

function serendipity_db_sqlite_fetch_array($res, $type = SQLITE_BOTH)
{
    static $search  = array('%00',  '%25');
    static $replace = array("\x00", '%');

    $row = sqlite_fetch_array($res, $type);
    if (!is_array($row)) {
        return $row;
    }

    /* strip any slashes, correct fieldname */
    foreach ($row as $i => $v) {
        // TODO: If a query of the format 'SELECT a.id, b.text FROM table' is used,
        //       the sqlite extension will give us key indizes 'a.id' and 'b.text'
        //       instead of just 'id' and 'text' like in mysql/postgresql extension.
        //       To fix that, we use a preg-regex; but that is quite performance costy.
        //       Either we always need to use 'SELECT a.id AS id, b.text AS text' in query,
        //       or the sqlite extension may get fixed. :-)
        $row[preg_replace('@^.+\.(.*)@', '\1', $i)] = str_replace($search, $replace, $v);
    }

    return $row;
}

function serendipity_db_in_sql($col, &$search_ids, $type = ' OR ') {
    $sql = array();
    if (!is_array($search_ids)) {
        return false;
    }
    
    foreach($search_ids AS $id) {
        $sql[] = $col . ' = ' . $id;
    }
    
    $cond = '(' . implode($type, $sql) . ')';
    return $cond;
}

function &serendipity_db_query($sql, $single = false, $result_type = "both", $reportErr = true, $assocKey = false, $assocVal = false, $expectError = false)
{
    global $serendipity;
    static $type_map = array(
                         'assoc' => SQLITE_ASSOC,
                         'num'   => SQLITE_NUM,
                         'both'  => SQLITE_BOTH,
                         'true'  => true,
                         'false' => false
    );

    static $debug = false;

    if ($debug) {
        // Open file and write directly. In case of crashes, the pointer needs to be killed.
        $fp = @fopen('sqlite.log', 'a');
        fwrite($fp, '[' . date('d.m.Y H:i') . '] SQLITE QUERY: ' . $sql . "\n\n");
        fclose($fp);
    }
    
    if ($reportErr && !$expectError) {
        $res = sqlite_query($sql, $serendipity['dbConn']);
    } else {
        $res = @sqlite_query($sql, $serendipity['dbConn']);
    }

    if (!$res) {
        if (!$expectError && !$serendipity['production']) {
            var_dump($res);
            var_dump($sql);
            $msg = "problem with query";
            return $msg;
        }
        if ($debug) {
            $fp = @fopen('sqlite.log', 'a');
            fwrite($fp, '[' . date('d.m.Y H:i') . '] [ERROR] ' . "\n\n");
            fclose($fp);
        }

        return $type_map['false'];
    }

    if ($res === true) {
        return $type_map['true'];
    }

    if (sqlite_num_rows($res) == 0) {
        if ($single) {
            return $type_map['false'];
        }
        return $type_map['true'];
    } else {
        $rows = array();

        while (($row = serendipity_db_sqlite_fetch_array($res, $type_map[$result_type]))) {
            if (!empty($assocKey) && !empty($assocVal)) {
                // You can fetch a key-associated array via the two function parameters assocKey and assocVal
                $rows[$row[$assocKey]] = $row[$assocVal];
            } else {
                $rows[] = $row;
            }
        }

        if ($debug) {
            $fp = @fopen('sqlite.log', 'a');
            fwrite($fp, '[' . date('d.m.Y H:i') . '] SQLITE RESULT: ' . print_r($rows, true). "\n\n");
            fclose($fp);
        }

        if ($single && count($rows) == 1) {
            return $rows[0];
        }

        return $rows;
    }
}

function serendipity_db_probe($hash, &$errs)
{
    global $serendipity;

    $dbName = (isset($hash['sqlitedbName']) ? $hash['sqlitedbName'] : $hash['dbName']);

    if (!function_exists('sqlite_open')) {
        $errs[] = 'SQLite extension not installed. Run "pear install sqlite" on your webserver or contact your systems administrator regarding this problem.';
        return false;
    }

    $serendipity['dbConn'] = sqlite_open($serendipity['serendipityPath'] . $dbName . '.db');

    if ($serendipity['dbConn']) {
        return true;
    }

    $errs[] = "Unable to open \"{$serendipity['serendipityPath']}$dbName.db\" - check permissions (directory needs to be writeable for webserver)!";
    return false;
}

function serendipity_db_schema_import($query)
{
    static $search  = array('{AUTOINCREMENT}', '{PRIMARY}', '{UNSIGNED}', '{FULLTEXT}', '{BOOLEAN}', '{UTF_8}');
    static $replace = array('INTEGER', 'PRIMARY KEY', '', '', 'BOOLEAN NOT NULL', '');

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

function serendipity_db_limit($start, $offset) {
    return $start . ', ' . $offset;
}

function serendipity_db_limit_sql($limitstring) {
    return ' LIMIT ' . $limitstring;
}

function serendipity_db_concat($string) {
    return 'concat(' . $string . ')';
}

/* vim: set sts=4 ts=4 expandtab : */
?>
