<?php
/**
 * @file
 * @ingroup Maintenance
 * @author Simetrical
 */

define( 'REPORTING_INTERVAL', 1000 );

function populateCategory( $begin, $maxlag, $throttle, $force ) {
	$dbw = wfGetDB( DB_MASTER );

	if( !$force ) {
		$row = $dbw->selectRow(
			'updatelog',
			'1',
			array( 'ul_key' => 'populate category' ),
			__FUNCTION__
		);
		if( $row ) {
			echo "Category table already populated.  Use php ".
			"maintenance/populateCategory.php\n--force from the command line ".
			"to override.\n";
			return true;
		}
	}

	$maxlag = intval( $maxlag );
	$throttle = intval( $throttle );
	$force = (bool)$force;
	if( $begin !== '' ) {
		$where = 'cl_to > '.$dbw->addQuotes( $begin );
	} else {
		$where = null;
	}
	$i = 0;

	while( true ) {
		# Find which category to update
		$row = $dbw->selectRow(
			'categorylinks',
			'cl_to',
			$where,
			__FUNCTION__,
			array(
				'ORDER BY' => 'cl_to'
			)
		);
		if( !$row ) {
			# Done, hopefully.
			break;
		}
		$name = $row->cl_to;
		$where = 'cl_to > '.$dbw->addQuotes( $name );

		# Use the row to update the category count
		$cat = Category::newFromName( $name );
		if( !is_object( $cat ) ) {
			echo "The category named $name is not valid?!\n";
		} else {
			$cat->refreshCounts();
		}

		++$i;
		if( !($i % REPORTING_INTERVAL) ) {
			echo "$name\n";
			wfWaitForSlaves( $maxlag );
		}
		usleep( $throttle*1000 );
	}

	if( $dbw->insert(
			'updatelog',
			array( 'ul_key' => 'populate category' ),
			__FUNCTION__,
			'IGNORE'
		)
	) {
		echo "Category population complete.\n";
		return true;
	} else {
		echo "Could not insert category population row.\n";
		return false;
	}
}
