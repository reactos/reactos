<?php

/**
 * Support functions for the importImages script
 *
 * @file
 * @ingroup Maintenance
 * @author Rob Church <robchur@gmail.com>
 */

/**
 * Search a directory for files with one of a set of extensions
 *
 * @param $dir Path to directory to search
 * @param $exts Array of extensions to search for
 * @return mixed Array of filenames on success, or false on failure
 */
function findFiles( $dir, $exts ) {
	if( is_dir( $dir ) ) {
		if( $dhl = opendir( $dir ) ) {
			while( ( $file = readdir( $dhl ) ) !== false ) {
				if( is_file( $dir . '/' . $file ) ) {
					list( /* $name */, $ext ) = splitFilename( $dir . '/' . $file );
					if( array_search( strtolower( $ext ), $exts ) !== false )
						$files[] = $dir . '/' . $file;
				}
			}
			return $files;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

/**
 * Split a filename into filename and extension
 *
 * @param $filename Filename
 * @return array
 */
function splitFilename( $filename ) {
	$parts = explode( '.', $filename );
	$ext = $parts[ count( $parts ) - 1 ];
	unset( $parts[ count( $parts ) - 1 ] );
	$fname = implode( '.', $parts );
	return array( $fname, $ext );
}