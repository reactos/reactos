<?php
if ( php_sapi_name() != 'cli' ) exit;

$IP = dirname(__FILE__) .'/..';
require( "$IP/includes/AutoLoader.php" );
$files = array_unique( AutoLoader::$localClasses );

foreach ( $files as $file ) {
	$parseInfo = parsekit_compile_file( "$IP/$file" );
	$classes = array_keys( $parseInfo['class_table'] );
	foreach ( $classes as $class ) {
		if ( !isset( AutoLoader::$localClasses[$class] ) ) {
			//printf( "%-50s Unlisted, in %s\n", $class, $file );
			echo "		'$class' => '$file',\n";
		} elseif ( AutoLoader::$localClasses[$class] !== $file ) {
			echo "$class: Wrong file: found in $file, listed in " . AutoLoader::$localClasses[$class] . "\n";
		}
	}

}


