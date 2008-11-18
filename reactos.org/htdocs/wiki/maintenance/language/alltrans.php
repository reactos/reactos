<?php
/**
 * @file
 * @ingroup MaintenanceLanguage
 *
 * Get all the translations messages, as defined in the English language file.
 */

require_once( dirname(__FILE__).'/../commandLine.inc' );

$wgEnglishMessages = array_keys( Language::getMessagesFor( 'en' ) );
foreach( $wgEnglishMessages as $key ) {
	echo "$key\n";
}


