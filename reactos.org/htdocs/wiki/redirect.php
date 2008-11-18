<?php
require_once( './includes/WebStart.php' );
global $wgArticlePath;

$page = $wgRequest->getVal( 'wpDropdown' );

$url = str_replace( "$1", urlencode( $page ), $wgArticlePath );

header( "Location: {$url}" );
