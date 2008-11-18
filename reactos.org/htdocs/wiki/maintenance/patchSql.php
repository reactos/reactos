<?php
/**
 * Manually run an SQL patch outside of the general updaters.
 * This ensures that the DB options (charset, prefix, engine) are correctly set.
 *
 * @file
 * @ingroup Maintenance
 */

require_once 'commandLine.inc';
require_once "$IP/maintenance/updaters.inc";

if( $args ) {
	foreach( $args as $arg ) {
		$files = array(
			$arg,
			archive( $arg ),
			archive( "patch-$arg.sql" ),
		);
		foreach( $files as $file ) {
			if( file_exists( $file ) ) {
				echo "$file ...\n";
				dbsource( $file );
				continue 2;
			}
		}
		echo "Could not find $arg\n";
	}
	echo "done.\n";
} else {
	echo "Run an SQL file into the DB, replacing prefix and charset vars.\n";
	echo "Usage:\n";
	echo "  php maintenance/patchSql.php file1.sql file2.sql ...\n";
	echo "\n";
	echo "Paths in maintenance/archive are automatically expanded if a local file isn't found.\n";
}
