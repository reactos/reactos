<?php
/**
 * Send SQL queries from the specified file to the database, performing
 * variable replacement along the way.
 *
 * @file
 * @ingroup Database Maintenance
 */

require_once( dirname(__FILE__) . '/' . 'commandLine.inc' );

if ( isset( $options['help'] ) ) {
	echo "Send SQL queries to a MediaWiki database.\nUsage: php sql.php [<file>]\n";
	exit( 1 );
}

if ( isset( $args[0] ) ) {
	$fileName = $args[0];
	$file = fopen( $fileName, 'r' );
	$promptCallback = false;
} else {
	$file = STDIN;
	$promptObject = new SqlPromptPrinter( "> " );
	$promptCallback = $promptObject->cb();
}

if ( !$file  ) {
	echo "Unable to open input file\n";
	exit( 1 );
}

$dbw =& wfGetDB( DB_MASTER );
$error = $dbw->sourceStream( $file, $promptCallback, 'sqlPrintResult' );
if ( $error !== true ) {
	echo $error;
	exit( 1 );
} else {
	exit( 0 );
}

//-----------------------------------------------------------------------------
class SqlPromptPrinter {
	function __construct( $prompt ) {
		$this->prompt = $prompt;
	}

	function cb() {
		return array( $this, 'printPrompt' );
	}

	function printPrompt() {
		echo $this->prompt;
	}
}

function sqlPrintResult( $res ) {
	if ( !$res ) {
		// Do nothing
	} elseif ( $res->numRows() ) {
		while ( $row = $res->fetchObject() ) {
			print_r( $row );
		}
	} else {
		$affected = $res->db->affectedRows();
		echo "Query OK, $affected row(s) affected\n";
	}
}


