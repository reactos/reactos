<?php
/**
 * @file
 * @ingroup MaintenanceLanguage
 */

require( '../commandLine.inc' );

# A list of unicode numerals is available at:
# http://www.fileformat.info/info/unicode/category/Nd/list.htm
$langs = array( 'Ar', 'As', 'Bh', 'Bo', 'Dz', 'Fa', 'Gu', 'Hi', 'Km', 'Kn', 'Ks', 'Lo', 'Ml', 'Mr', 'Ne', 'New', 'Or', 'Pa', 'Pi', 'Sa' );

foreach( $langs as $code ) {
	$filename = Language::getMessagesFileName( $code );
	echo "Loading language [$code] ... ";
	unset( $digitTransformTable );
	require_once( $filename );
	if( !isset( $digitTransformTable ) ) {
		print "\$digitTransformTable not found\n";
		continue;
	}

	print "OK\n\$digitTransformTable = array(\n";
	foreach( $digitTransformTable as $latin => $translation ) {
		$htmlent = utf8ToHexSequence( $translation );
		print "'$latin' => '$translation', # &#x$htmlent;\n";
	}
	print ");\n";
}
