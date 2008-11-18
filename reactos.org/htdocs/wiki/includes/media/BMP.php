<?php
/**
 * @file
 * @ingroup Media
 */

/**
 * Handler for Microsoft's bitmap format; getimagesize() doesn't
 * support these files
 *
 * @ingroup Media
 */
class BmpHandler extends BitmapHandler {

	/*
	 * Get width and height from the bmp header.
	 */
	function getImageSize( $image, $filename ) {
		$f = fopen( $filename, 'r' );
		if(!$f) return false;
		$header = fread( $f, 54 );
		fclose($f);

		// Extract binary form of width and height from the header
		$w = substr( $header, 18, 4);
		$h = substr( $header, 22, 4);

		// Convert the unsigned long 32 bits (little endian):
		$w = unpack( 'V' , $w );
		$h = unpack( 'V' , $h );
		return array( $w[1], $h[1] );
	}
}
