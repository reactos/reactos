<?php
/**
 * This script delete image information from memcached.
 *
 * Usage example:
 * php deleteImageMemcached.php --until "2005-09-05 00:00:00" --sleep 0 --report 10
 *
 * @file
 * @ingroup Maintenance
 */

$optionsWithArgs = array( 'until', 'sleep', 'report' );

require_once 'commandLine.inc';

/**
 * @ingroup Maintenance
 */
class DeleteImageCache {
	var $until, $sleep, $report;

	function DeleteImageCache( $until, $sleep, $report ) {
		$this->until = $until;
		$this->sleep = $sleep;
		$this->report = $report;
	}

	function main() {
		global $wgMemc;
		$fname = 'DeleteImageCache::main';

		ini_set( 'display_errors', false );

		$dbr = wfGetDB( DB_SLAVE );

		$res = $dbr->select( 'image',
			array( 'img_name' ),
			array( "img_timestamp < {$this->until}" ),
			$fname
		);

		$i = 0;
		$total = $this->getImageCount();

		while ( $row = $dbr->fetchObject( $res ) ) {
			if ($i % $this->report == 0)
				printf("%s: %13s done (%s)\n", wfWikiID(), "$i/$total", wfPercent( $i / $total * 100 ));
			$md5 = md5( $row->img_name );
			$wgMemc->delete( wfMemcKey( 'Image', $md5 ) );

			if ($this->sleep != 0)
				usleep( $this->sleep );

			++$i;
		}
	}

	function getImageCount() {
		$fname = 'DeleteImageCache::getImageCount';

		$dbr = wfGetDB( DB_SLAVE );
		return $dbr->selectField( 'image', 'COUNT(*)', array(), $fname );
	}
}

$until = preg_replace( "/[^\d]/", '', $options['until'] );
$sleep = (int)$options['sleep'] * 1000; // milliseconds
$report = (int)$options['report'];

$dic = new DeleteImageCache( $until, $sleep, $report );
$dic->main();

