<?php

	// Database functions:
	
	function SQL_query ( $ROST_sql_statement ) {
		return mysql_query( $ROST_sql_statement );
	}

	function SQL_loop_array ( $ROST_sql_statement ) {
		return mysql_fetch_array($ROST_sql_statement);
	}

	function SQL_query_array ( $ROST_sql_statement ) {
		$ROST_sql_statement_temp = mysql_query( $ROST_sql_statement );
		return mysql_fetch_array($ROST_sql_statement_temp);
	}

	function SQL_query_row ( $ROST_sql_statement ) {
		$ROST_sql_statement_temp = mysql_query( $ROST_sql_statement );
		return mysql_fetch_row( $ROST_sql_statement_temp );
	}
	
	function SQL_escape ( $ROST_sql_statement ) {
		return mysql_real_escape_string ( $ROST_sql_statement );
	}


	function check_langcode ( $temp_var ) { // allow only valid language codes; e.g. "en", "en-us", "en-uk", "de-at", etc.
		if ( (strlen($temp_var) == 2 && ereg("([a-z]{2})", $temp_var)) || (strlen($temp_var) == 5 && ereg("([a-z]{2}-[a-z]{2})", $temp_var)) ) {
			return 1;
		}
		else {
			return 0;
		}
		
		return 0;
	}

?>