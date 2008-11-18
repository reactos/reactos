<?php

/**
 * Generate an OpenSearch description file
 */

require_once( dirname(__FILE__) . '/includes/WebStart.php' );

if( $wgRequest->getVal( 'ctype' ) == 'application/xml' ) {
	// Makes testing tweaks about a billion times easier
	$ctype = 'application/xml';
} else {
	$ctype = 'application/opensearchdescription+xml';
}

$response = $wgRequest->response();
$response->header( "Content-type: $ctype" );

// Set an Expires header so that squid can cache it for a short time
// Short enough so that the sysadmin barely notices when $wgSitename is changed
$expiryTime = 600; # 10 minutes
$response->header( 'Expires: ' . gmdate( 'D, d M Y H:i:s', time() + $expiryTime ) . ' GMT' );
$response->header( 'Cache-control: max-age=600' );

print '<?xml version="1.0"?>';
print Xml::openElement( 'OpenSearchDescription',
	array(
		'xmlns' => 'http://a9.com/-/spec/opensearch/1.1/',
		'xmlns:moz' => 'http://www.mozilla.org/2006/browser/search/' ) );

// The spec says the ShortName must be no longer than 16 characters,
// but 16 is *realllly* short. In practice, browsers don't appear to care
// when we give them a longer string, so we're no longer attempting to trim.
//
// Note: ShortName and the <link title=""> need to match; they are used as
// a key for identifying if the search engine has been added already, *and*
// as the display name presented to the end-user.
//
// Behavior seems about the same between Firefox and IE 7/8 here.
// 'Description' doesn't appear to be used by either.
$fullName = wfMsgForContent( 'opensearch-desc' );
print Xml::element( 'ShortName', null, $fullName );
print Xml::element( 'Description', null, $fullName );

// By default we'll use the site favicon.
// Double-check if IE supports this properly?
print Xml::element( 'Image',
	array(
		'height' => 16,
		'width' => 16,
		'type' => 'image/x-icon' ),
	wfExpandUrl( $wgFavicon ) );

$urls = array();

// General search template. Given an input term, this should bring up
// search results or a specific found page.
// At least Firefox and IE 7 support this.
$searchPage = SpecialPage::getTitleFor( 'Search' );
$urls[] = array(
	'type' => 'text/html',
	'method' => 'get',
	'template' => $searchPage->getFullURL( 'search={searchTerms}' ) );

if( $wgEnableAPI ) {
	// JSON interface for search suggestions.
	// Supported in Firefox 2 and later.
	$urls[] = array(
		'type' => 'application/x-suggestions+json',
		'method' => 'get',
		'template' => SearchEngine::getOpenSearchTemplate() );
}

// Allow hooks to override the suggestion URL settings in a more
// general way than overriding the whole search engine...
wfRunHooks( 'OpenSearchUrls', array( &$urls ) );

foreach( $urls as $attribs ) {
	print Xml::element( 'Url', $attribs );
}

// And for good measure, add a link to the straight search form.
// This is a custom format extension for Firefox, which otherwise
// sends you to the domain root if you hit "enter" with an empty
// search box.
print Xml::element( 'moz:SearchForm', null,
	$searchPage->getFullUrl() );

print '</OpenSearchDescription>';
