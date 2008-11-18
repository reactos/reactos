<?php
/**
 * Helper file for update.php
 *
 * @file
 * @ingroup Maintenance
 */

function print_c($last, $current) {
	echo str_repeat( chr(8), strlen( $last ) ) . $current;
}

