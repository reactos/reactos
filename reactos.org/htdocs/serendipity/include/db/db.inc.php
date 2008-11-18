<?php # $Id: db.inc.php 448 2005-09-05 21:14:49Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (@include_once(S9Y_INCLUDE_PATH . "include/db/{$serendipity['dbType']}.inc.php")) {
    @define('S9Y_DB_INCLUDED', TRUE);
}

function serendipity_db_update($table, $keys, $values)
{
    global $serendipity;

    $set = '';

    foreach ($values as $k => $v) {
        if (strlen($set))
            $set .= ', ';
        $set .= $k . '=\'' . serendipity_db_escape_string($v) . '\'';
    }

    $where = '';
    foreach ($keys as $k => $v) {
        if (strlen($where))
            $where .= ' AND ';
        $where .= $k . '=\'' . serendipity_db_escape_string($v) . '\'';
    }

    if (strlen($where)) {
        $where = " WHERE $where";
    }

    return serendipity_db_query("UPDATE {$serendipity['dbPrefix']}$table SET $set $where");
}

function serendipity_db_insert($table, $values)
{
    global $serendipity;

    $names = implode(',', array_keys($values));

    $vals = '';
    foreach ($values as $k => $v) {
        if (strlen($vals))
            $vals .= ', ';
        $vals .= '\'' . serendipity_db_escape_string($v) . '\'';
    }

    return serendipity_db_query("INSERT INTO {$serendipity['dbPrefix']}$table ($names) values ($vals)");
}

function serendipity_db_bool ($val)
{
    if(($val === true) || ($val == 'true') || ($val == 't') || ($val == '1'))
        return true;
    #elseif (($val === false || $val == 'false' || $val == 'f'))
    else
        return false;
}

function serendipity_db_get_interval($val, $ival = 900) {
    global $serendipity;

    switch($serendipity['dbType']) {
        case 'sqlite':
            $interval = $ival;
            $ts       = time();
            break;

        case 'postgres':
            $interval = "interval '$ival'";
            $ts       = 'NOW()';
            break;

        case 'mysql':
        case 'mysqli':
        default:
            $interval = $ival;
            $ts       = 'NOW()';
            break;
    }
    
    switch($val) {
        case 'interval':
            return $interval;
        
        default:
        case 'ts':
            return $ts;
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
