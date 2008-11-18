<?php
/** Mazandarani
 *
 * Minimalistic setup, needed to switch to right-to-left writing.
 *
 * @ingroup Language
 * @file
 */

$linkPrefixExtension = true;
$fallback8bitEncoding = 'windows-1256';

$rtl = true;
$defaultUserOptionOverrides = array(
	# Swap sidebar to right side by default
	'quickbar' => 2,
	# Underlines seriously harm legibility. Force off:
	'underline' => 0,
);

$fallback = 'fa';
