<?php
/**
*
* @package install
* @version $Id: new_normalizer.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2007 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @ignore
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

/**
* A wrapper function for the normalizer which takes care of including the class if required and modifies the passed strings
* to be in NFC (Normalization Form Composition).
*
* @param	mixed	$strings	a string or an array of strings to normalize
* @return	mixed				the normalized content, preserving array keys if array given.
*/
function utf8_new_normalize_nfc($strings)
{
	if (empty($strings))
	{
		return $strings;
	}

	if (!is_array($strings))
	{
		utf_new_normalizer::nfc($strings);
	}
	else if (is_array($strings))
	{
		foreach ($strings as $key => $string)
		{
			if (is_array($string))
			{
				foreach ($string as $_key => $_string)
				{
					utf_new_normalizer::nfc($strings[$key][$_key]);
				}
			}
			else
			{
				utf_new_normalizer::nfc($strings[$key]);
			}
		}
	}

	return $strings;
}

class utf_new_normalizer
{
	/**
	* Validate, cleanup and normalize a string
	*
	* The ultimate convenience function! Clean up invalid UTF-8 sequences,
	* and convert to Normal Form C, canonical composition.
	*
	* @param	string	&$str	The dirty string
	* @return	string			The same string, all shiny and cleaned-up
	*/
	function cleanup(&$str)
	{
		// The string below is the list of all autorized characters, sorted by frequency in latin text
		$pos = strspn($str, "\x20\x65\x69\x61\x73\x6E\x74\x72\x6F\x6C\x75\x64\x5D\x5B\x63\x6D\x70\x27\x0A\x67\x7C\x68\x76\x2E\x66\x62\x2C\x3A\x3D\x2D\x71\x31\x30\x43\x32\x2A\x79\x78\x29\x28\x4C\x39\x41\x53\x2F\x50\x22\x45\x6A\x4D\x49\x6B\x33\x3E\x35\x54\x3C\x44\x34\x7D\x42\x7B\x38\x46\x77\x52\x36\x37\x55\x47\x4E\x3B\x4A\x7A\x56\x23\x48\x4F\x57\x5F\x26\x21\x4B\x3F\x58\x51\x25\x59\x5C\x09\x5A\x2B\x7E\x5E\x24\x40\x60\x7F\x0D");
		$len = strlen($str);

		if ($pos == $len)
		{
			// ASCII strings with no special chars return immediately
			return;
		}

		// Note: we do not check for $GLOBALS['utf_canonical_decomp']. It is assumed they are always loaded together
		if (!isset($GLOBALS['utf_nfc_qc']))
		{
			global $phpbb_root_path, $phpEx;
			include($phpbb_root_path . 'includes/utf/data/utf_nfc_qc.' . $phpEx);
		}

		if (!isset($GLOBALS['utf_canonical_decomp']))
		{
			global $phpbb_root_path, $phpEx;
			include($phpbb_root_path . 'includes/utf/data/utf_canonical_decomp.' . $phpEx);
		}

		// Replace any byte in the range 0x00..0x1F, except for \r, \n and \t
		// We replace those characters with a 0xFF byte, which is illegal in UTF-8 and will in turn be replaced with a UTF replacement char
		$str = strtr(
			$str,
			"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x0B\x0C\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F",
			"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
		);

		$str = utf_new_normalizer::recompose($str, $pos, $len, $GLOBALS['utf_nfc_qc'], $GLOBALS['utf_canonical_decomp']);
	}

	/**
	* Validate and normalize a UTF string to NFC
	*
	* @param	string	&$str	Unchecked UTF string
	* @return	string			The string, validated and in normal form
	*/
	function nfc(&$str)
	{
		$pos = strspn($str, UTF8_ASCII_RANGE);
		$len = strlen($str);

		if ($pos == $len)
		{
			// ASCII strings return immediately
			return;
		}

		if (!isset($GLOBALS['utf_nfc_qc']))
		{
			global $phpbb_root_path, $phpEx;
			include($phpbb_root_path . 'includes/utf/data/utf_nfc_qc.' . $phpEx);
		}

		if (!isset($GLOBALS['utf_canonical_decomp']))
		{
			global $phpbb_root_path, $phpEx;
			include($phpbb_root_path . 'includes/utf/data/utf_canonical_decomp.' . $phpEx);
		}

		$str = utf_new_normalizer::recompose($str, $pos, $len, $GLOBALS['utf_nfc_qc'], $GLOBALS['utf_canonical_decomp']);
	}

	/**
	* Validate and normalize a UTF string to NFKC
	*
	* @param	string	&$str	Unchecked UTF string
	* @return	string			The string, validated and in normal form
	*/
	function nfkc(&$str)
	{
		$pos = strspn($str, UTF8_ASCII_RANGE);
		$len = strlen($str);

		if ($pos == $len)
		{
			// ASCII strings return immediately
			return;
		}

		if (!isset($GLOBALS['utf_nfkc_qc']))
		{
			global $phpbb_root_path, $phpEx;
			include($phpbb_root_path . 'includes/utf/data/utf_nfkc_qc.' . $phpEx);
		}

		if (!isset($GLOBALS['utf_compatibility_decomp']))
		{
			global $phpbb_root_path, $phpEx;
			include($phpbb_root_path . 'includes/utf/data/utf_compatibility_decomp.' . $phpEx);
		}

		$str = utf_new_normalizer::recompose($str, $pos, $len, $GLOBALS['utf_nfkc_qc'], $GLOBALS['utf_compatibility_decomp']);
	}

	/**
	* Recompose a UTF string
	*
	* @param	string	$str			Unchecked UTF string
	* @param	integer	$pos			Position of the first UTF char (in bytes)
	* @param	integer	$len			Length of the string (in bytes)
	* @param	array	&$qc			Quick-check array, passed by reference but never modified
	* @param	array	&$decomp_map	Decomposition mapping, passed by reference but never modified
	* @return	string					The string, validated and recomposed
	*
	* @access	private
	*/
	function recompose($str, $pos, $len, &$qc, &$decomp_map)
	{
		global $utf_canonical_comp;

		// Load the canonical composition table
		if (!isset($utf_canonical_comp))
		{
			global $phpbb_root_path, $phpEx;
			include($phpbb_root_path . 'includes/utf/data/utf_canonical_comp.' . $phpEx);
		}

		return utf_normalizer::recompose($str, $pos, $len, $qc, $decomp_map);
	}
}

?>