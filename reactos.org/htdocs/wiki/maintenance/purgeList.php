<?php
/**
 * Send purge requests for listed pages to squid
 *
 * @file
 * @ingroup Maintenance
 */

require_once( "commandLine.inc" );

$stdin = fopen( "php://stdin", "rt" );
$urls = array();

while( !feof( $stdin ) ) {
	$page = trim( fgets( $stdin ) );
	if ( substr( $page, 0, 7 ) == 'http://' ) {
		$urls[] = $page;
	} elseif( $page !== '' ) {
		$title = Title::newFromText( $page );
		if( $title ) {
			$url = $title->getFullUrl();
			echo "$url\n";
			$urls[] = $url;
			if( isset( $options['purge'] ) ) {
				$title->invalidateCache();
			}
		} else {
			echo "(Invalid title '$page')\n";
		}
	}
}

echo "Purging " . count( $urls ) . " urls...\n";
$u = new SquidUpdate( $urls );
$u->doUpdate();

echo "Done!\n";


