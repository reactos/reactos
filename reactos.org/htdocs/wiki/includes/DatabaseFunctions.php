<?php
/**
 * Legacy database functions, for compatibility with pre-1.3 code
 * NOTE: this file is no longer loaded by default.
 * @file
 * @ingroup Database
 */

/**
 * Usually aborts on failure
 * If errors are explicitly ignored, returns success
 * @param $sql String: SQL query
 * @param $db Mixed: database handler
 * @param $fname String: name of the php function calling
 */
function wfQuery( $sql, $db, $fname = '' ) {
	if ( !is_numeric( $db ) ) {
		# Someone has tried to call this the old way
		throw new FatalError( wfMsgNoDB( 'wrong_wfQuery_params', $db, $sql ) );
	}
	$c = wfGetDB( $db );
	if ( $c !== false ) {
		return $c->query( $sql, $fname );
	} else {
		return false;
	}
}

/**
 *
 * @param $sql String: SQL query
 * @param $dbi
 * @param $fname String: name of the php function calling
 * @return Array: first row from the database
 */
function wfSingleQuery( $sql, $dbi, $fname = '' ) {
	$db = wfGetDB( $dbi );
	$res = $db->query($sql, $fname );
	$row = $db->fetchRow( $res );
	$ret = $row[0];
	$db->freeResult( $res );
	return $ret;
}

/**
 * Turns on (false) or off (true) the automatic generation and sending
 * of a "we're sorry, but there has been a database error" page on
 * database errors. Default is on (false). When turned off, the
 * code should use wfLastErrno() and wfLastError() to handle the
 * situation as appropriate.
 *
 * @param $newstate
 * @param $dbi
 * @return Returns the previous state.
 */
function wfIgnoreSQLErrors( $newstate, $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->ignoreErrors( $newstate );
	} else {
		return NULL;
	}
}

/**#@+
 * @param $res Database result handler
 * @param $dbi
*/

/**
 * Free a database result
 * @return Bool: whether result is sucessful or not.
 */
function wfFreeResult( $res, $dbi = DB_LAST )
{
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		$db->freeResult( $res );
		return true;
	} else {
		return false;
	}
}

/**
 * Get an object from a database result
 * @return object|false object we requested
 */
function wfFetchObject( $res, $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->fetchObject( $res, $dbi = DB_LAST );
	} else {
		return false;
	}
}

/**
 * Get a row from a database result
 * @return object|false row we requested
 */
function wfFetchRow( $res, $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->fetchRow ( $res, $dbi = DB_LAST );
	} else {
		return false;
	}
}

/**
 * Get a number of rows from a database result
 * @return integer|false number of rows
 */
function wfNumRows( $res, $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->numRows( $res, $dbi = DB_LAST );
	} else {
		return false;
	}
}

/**
 * Get the number of fields from a database result
 * @return integer|false number of fields
 */
function wfNumFields( $res, $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->numFields( $res );
	} else {
		return false;
	}
}

/**
 * Return name of a field in a result
 * @param $res Mixed: Ressource link see Database::fieldName()
 * @param $n Integer: id of the field
 * @param $dbi Default DB_LAST
 * @return string|false name of field
 */
function wfFieldName( $res, $n, $dbi = DB_LAST )
{
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->fieldName( $res, $n, $dbi = DB_LAST );
	} else {
		return false;
	}
}
/**#@-*/

/**
 * @todo document function
 */
function wfInsertId( $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->insertId();
	} else {
		return false;
	}
}

/**
 * @todo document function
 */
function wfDataSeek( $res, $row, $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->dataSeek( $res, $row );
	} else {
		return false;
	}
}

/**
 * @todo document function
 */
function wfLastErrno( $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->lastErrno();
	} else {
		return false;
	}
}

/**
 * @todo document function
 */
function wfLastError( $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->lastError();
	} else {
		return false;
	}
}

/**
 * @todo document function
 */
function wfAffectedRows( $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->affectedRows();
	} else {
		return false;
	}
}

/**
 * @todo document function
 */
function wfLastDBquery( $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->lastQuery();
	} else {
		return false;
	}
}

/**
 * @see Database::Set()
 * @todo document function
 * @param $table
 * @param $var
 * @param $value
 * @param $cond
 * @param $dbi Default DB_MASTER
 */
function wfSetSQL( $table, $var, $value, $cond, $dbi = DB_MASTER )
{
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->set( $table, $var, $value, $cond );
	} else {
		return false;
	}
}


/**
 * @see Database::selectField()
 * @todo document function
 * @param $table
 * @param $var
 * @param $cond Default ''
 * @param $dbi Default DB_LAST
 */
function wfGetSQL( $table, $var, $cond='', $dbi = DB_LAST )
{
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->selectField( $table, $var, $cond );
	} else {
		return false;
	}
}

/**
 * @see Database::fieldExists()
 * @todo document function
 * @param $table
 * @param $field
 * @param $dbi Default DB_LAST
 * @return Result of Database::fieldExists() or false.
 */
function wfFieldExists( $table, $field, $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->fieldExists( $table, $field );
	} else {
		return false;
	}
}

/**
 * @see Database::indexExists()
 * @todo document function
 * @param $table String
 * @param $index
 * @param $dbi Default DB_LAST
 * @return Result of Database::indexExists() or false.
 */
function wfIndexExists( $table, $index, $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->indexExists( $table, $index );
	} else {
		return false;
	}
}

/**
 * @see Database::insert()
 * @todo document function
 * @param $table String
 * @param $array Array
 * @param $fname String, default 'wfInsertArray'.
 * @param $dbi Default DB_MASTER
 * @return result of Database::insert() or false.
 */
function wfInsertArray( $table, $array, $fname = 'wfInsertArray', $dbi = DB_MASTER ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->insert( $table, $array, $fname );
	} else {
		return false;
	}
}

/**
 * @see Database::getArray()
 * @todo document function
 * @param $table String
 * @param $vars
 * @param $conds
 * @param $fname String, default 'wfGetArray'.
 * @param $dbi Default DB_LAST
 * @return result of Database::getArray() or false.
 */
function wfGetArray( $table, $vars, $conds, $fname = 'wfGetArray', $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->getArray( $table, $vars, $conds, $fname );
	} else {
		return false;
	}
}

/**
 * @see Database::update()
 * @param $table String
 * @param $values
 * @param $conds
 * @param $fname String, default 'wfUpdateArray'
 * @param $dbi Default DB_MASTER
 * @return Result of Database::update()) or false;
 * @todo document function
 */
function wfUpdateArray( $table, $values, $conds, $fname = 'wfUpdateArray', $dbi = DB_MASTER ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		$db->update( $table, $values, $conds, $fname );
		return true;
	} else {
		return false;
	}
}

/**
 * @todo document function
 */
function wfTableName( $name, $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->tableName( $name );
	} else {
		return false;
	}
}

/**
 * @todo document function
 */
function wfStrencode( $s, $dbi = DB_LAST ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->strencode( $s );
	} else {
		return false;
	}
}

/**
 * @todo document function
 */
function wfNextSequenceValue( $seqName, $dbi = DB_MASTER ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->nextSequenceValue( $seqName );
	} else {
		return false;
	}
}

/**
 * @todo document function
 */
function wfUseIndexClause( $index, $dbi = DB_SLAVE ) {
	$db = wfGetDB( $dbi );
	if ( $db !== false ) {
		return $db->useIndexClause( $index );
	} else {
		return false;
	}
}
