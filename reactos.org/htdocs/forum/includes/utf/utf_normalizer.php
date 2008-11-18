<?php
/**
*
* @package utf
* @version $Id: utf_normalizer.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

/**
* Some Unicode characters encoded in UTF-8
*
* Preserved for compatibility
*/
define('UTF8_REPLACEMENT', "\xEF\xBF\xBD");
define('UTF8_MAX', "\xF4\x8F\xBF\xBF");
define('UTF8_FFFE', "\xEF\xBF\xBE");
define('UTF8_FFFF', "\xEF\xBF\xBF");
define('UTF8_SURROGATE_FIRST', "\xED\xA0\x80");
define('UTF8_SURROGATE_LAST', "\xED\xBF\xBF");
define('UTF8_HANGUL_FIRST', "\xEA\xB0\x80");
define('UTF8_HANGUL_LAST', "\xED\x9E\xA3");

define('UTF8_CJK_FIRST', "\xE4\xB8\x80");
define('UTF8_CJK_LAST', "\xE9\xBE\xBB");
define('UTF8_CJK_B_FIRST', "\xF0\xA0\x80\x80");
define('UTF8_CJK_B_LAST', "\xF0\xAA\x9B\x96");

// Unset global variables
unset($GLOBALS['utf_jamo_index'], $GLOBALS['utf_jamo_type'], $GLOBALS['utf_nfc_qc'], $GLOBALS['utf_combining_class'], $GLOBALS['utf_canonical_comp'], $GLOBALS['utf_canonical_decomp'], $GLOBALS['utf_nfkc_qc'], $GLOBALS['utf_compatibility_decomp']);

// NFC_QC and NFKC_QC values
define('UNICODE_QC_MAYBE', 0);
define('UNICODE_QC_NO', 1);

// Contains all the ASCII characters appearing in UTF-8, sorted by frequency
define('UTF8_ASCII_RANGE', "\x20\x65\x69\x61\x73\x6E\x74\x72\x6F\x6C\x75\x64\x5D\x5B\x63\x6D\x70\x27\x0A\x67\x7C\x68\x76\x2E\x66\x62\x2C\x3A\x3D\x2D\x71\x31\x30\x43\x32\x2A\x79\x78\x29\x28\x4C\x39\x41\x53\x2F\x50\x22\x45\x6A\x4D\x49\x6B\x33\x3E\x35\x54\x3C\x44\x34\x7D\x42\x7B\x38\x46\x77\x52\x36\x37\x55\x47\x4E\x3B\x4A\x7A\x56\x23\x48\x4F\x57\x5F\x26\x21\x4B\x3F\x58\x51\x25\x59\x5C\x09\x5A\x2B\x7E\x5E\x24\x40\x60\x7F\x00\x01\x02\x03\x04\x05\x06\x07\x08\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F");

// Contains all the tail bytes that can appear in the composition of a UTF-8 char
define('UTF8_TRAILING_BYTES', "\xA9\xA0\xA8\x80\xAA\x99\xA7\xBB\xAB\x89\x94\x82\xB4\xA2\xAE\x83\xB0\xB9\xB8\x93\xAF\xBC\xB3\x81\xA4\xB2\x9C\xA1\xB5\xBE\xBD\xBA\x98\xAD\xB1\x84\x95\xA6\xB6\x88\x8D\x90\xB7\xBF\x92\x85\xA5\x97\x8C\x86\xA3\x8E\x9F\x8F\x87\x91\x9D\xAC\x9E\x8B\x96\x9B\x8A\x9A");

// Constants used by the Hangul [de]composition algorithms
define('UNICODE_HANGUL_SBASE', 0xAC00);
define('UNICODE_HANGUL_LBASE', 0x1100);
define('UNICODE_HANGUL_VBASE', 0x1161);
define('UNICODE_HANGUL_TBASE', 0x11A7);
define('UNICODE_HANGUL_SCOUNT', 11172);
define('UNICODE_HANGUL_LCOUNT', 19);
define('UNICODE_HANGUL_VCOUNT', 21);
define('UNICODE_HANGUL_TCOUNT', 28);
define('UNICODE_HANGUL_NCOUNT', 588);
define('UNICODE_JAMO_L', 0);
define('UNICODE_JAMO_V', 1);
define('UNICODE_JAMO_T', 2);

/**
* Unicode normalization routines
*
* @package utf
*/
class utf_normalizer
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

		$str = utf_normalizer::recompose($str, $pos, $len, $GLOBALS['utf_nfc_qc'], $GLOBALS['utf_canonical_decomp']);
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

		$str = utf_normalizer::recompose($str, $pos, $len, $GLOBALS['utf_nfc_qc'], $GLOBALS['utf_canonical_decomp']);
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

		$str = utf_normalizer::recompose($str, $pos, $len, $GLOBALS['utf_nfkc_qc'], $GLOBALS['utf_compatibility_decomp']);
	}

	/**
	* Validate and normalize a UTF string to NFD
	*
	* @param	string	&$str	Unchecked UTF string
	* @return	string			The string, validated and in normal form
	*/
	function nfd(&$str)
	{
		$pos = strspn($str, UTF8_ASCII_RANGE);
		$len = strlen($str);

		if ($pos == $len)
		{
			// ASCII strings return immediately
			return;
		}

		if (!isset($GLOBALS['utf_canonical_decomp']))
		{
			global $phpbb_root_path, $phpEx;
			include($phpbb_root_path . 'includes/utf/data/utf_canonical_decomp.' . $phpEx);
		}

		$str = utf_normalizer::decompose($str, $pos, $len, $GLOBALS['utf_canonical_decomp']);
	}

	/**
	* Validate and normalize a UTF string to NFKD
	*
	* @param	string	&$str	Unchecked UTF string
	* @return	string			The string, validated and in normal form
	*/
	function nfkd(&$str)
	{
		$pos = strspn($str, UTF8_ASCII_RANGE);
		$len = strlen($str);

		if ($pos == $len)
		{
			// ASCII strings return immediately
			return;
		}

		if (!isset($GLOBALS['utf_compatibility_decomp']))
		{
			global $phpbb_root_path, $phpEx;
			include($phpbb_root_path . 'includes/utf/data/utf_compatibility_decomp.' . $phpEx);
		}

		$str = utf_normalizer::decompose($str, $pos, $len, $GLOBALS['utf_compatibility_decomp']);
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
		global $utf_combining_class, $utf_canonical_comp, $utf_jamo_type, $utf_jamo_index;

		// Load some commonly-used tables
		if (!isset($utf_jamo_index, $utf_jamo_type, $utf_combining_class))
		{
			global $phpbb_root_path, $phpEx;
			include($phpbb_root_path . 'includes/utf/data/utf_normalizer_common.' . $phpEx);
		}

		// Load the canonical composition table
		if (!isset($utf_canonical_comp))
		{
			global $phpbb_root_path, $phpEx;
			include($phpbb_root_path . 'includes/utf/data/utf_canonical_comp.' . $phpEx);
		}

		// Buffer the last ASCII char before the UTF-8 stuff if applicable
		$tmp = '';
		$i = $tmp_pos = $last_cc = 0;

		$buffer = ($pos) ? array(++$i => $str[$pos - 1]) : array();

		// UTF char length array
		// This array is used to determine the length of a UTF character.
		// Be $c the result of ($str[$pos] & "\xF0") --where $str is the string we're operating on and $pos
		// the position of the cursor--, if $utf_len_mask[$c] does not exist, the byte is an ASCII char.
		// Otherwise, if $utf_len_mask[$c] is greater than 0, we have a the leading byte of a multibyte character
		// whose length is $utf_len_mask[$c] and if it is equal to 0, the byte is a trailing byte.
		$utf_len_mask = array(
			// Leading bytes masks
			"\xC0" => 2, "\xD0" => 2, "\xE0" => 3, "\xF0" => 4,
			// Trailing bytes masks
			"\x80" => 0, "\x90" => 0, "\xA0" => 0, "\xB0" => 0
		);

		$extra_check = array(
			"\xED" => 1, "\xEF" => 1, "\xC0" => 1, "\xC1" => 1, "\xE0" => 1, "\xF0" => 1,
			"\xF4" => 1, "\xF5" => 1, "\xF6" => 1, "\xF7" => 1, "\xF8" => 1, "\xF9" => 1,
			"\xFA" => 1, "\xFB" => 1, "\xFC" => 1, "\xFD" => 1, "\xFE" => 1, "\xFF" => 1
		);

		$utf_validation_mask = array(
			2	=> "\xE0\xC0",
			3	=> "\xF0\xC0\xC0",
			4	=> "\xF8\xC0\xC0\xC0"
		);

		$utf_validation_check = array(
			2	=> "\xC0\x80",
			3	=> "\xE0\x80\x80",
			4	=> "\xF0\x80\x80\x80"
		);

		// Main loop
		do
		{
			// STEP 0: Capture the current char and buffer it
			$c = $str[$pos];
			$c_mask = $c & "\xF0";

			if (isset($utf_len_mask[$c_mask]))
			{
				// Byte at $pos is either a leading byte or a missplaced trailing byte
				if ($utf_len = $utf_len_mask[$c_mask])
				{
					// Capture the char
					$buffer[++$i & 7] = $utf_char = substr($str, $pos, $utf_len);

					// Let's find out if a thorough check is needed
					if (isset($qc[$utf_char]))
					{
						// If the UTF char is in the qc array then it may not be in normal form. We do nothing here, the actual processing is below this "if" block
					}
					else if (isset($utf_combining_class[$utf_char]))
					{
						if ($utf_combining_class[$utf_char] < $last_cc)
						{
							// A combining character that is NOT canonically ordered
						}
						else
						{
							// A combining character that IS canonically ordered, skip to the next char
							$last_cc = $utf_combining_class[$utf_char];

							$pos += $utf_len;
							continue;
						}
					}
					else
					{
						// At this point, $utf_char holds a UTF char that we know is not a NF[K]C_QC and is not a combining character.
						// It can be a singleton, a canonical composite, a replacement char or an even an ill-formed bunch of bytes. Let's find out
						$last_cc = 0;

						// Check that we have the correct number of trailing bytes
						if (($utf_char & $utf_validation_mask[$utf_len]) != $utf_validation_check[$utf_len])
						{
							// Current char isn't well-formed or legal: either one or several trailing bytes are missing, or the Unicode char
							// has been encoded in a five- or six- byte sequence
							if ($utf_char[0] >= "\xF8")
							{
								if ($utf_char[0] < "\xFC")
								{
									$trailing_bytes = 4;
								}
								else if ($utf_char[0] > "\xFD")
								{
									$trailing_bytes = 0;
								}
								else
								{
									$trailing_bytes = 5;
								}
							}
							else
							{
								$trailing_bytes = $utf_len - 1;
							}

							$tmp .= substr($str, $tmp_pos, $pos - $tmp_pos) . UTF8_REPLACEMENT;
							$pos += strspn($str, UTF8_TRAILING_BYTES, ++$pos, $trailing_bytes);
							$tmp_pos = $pos;

							continue;
						}

						if (isset($extra_check[$c]))
						{
							switch ($c)
							{
								// Note: 0xED is quite common in Korean
								case "\xED":
									if ($utf_char >= "\xED\xA0\x80")
									{
										// Surrogates (U+D800..U+DFFF) are not allowed in UTF-8 (UTF sequence 0xEDA080..0xEDBFBF)
										$tmp .= substr($str, $tmp_pos, $pos - $tmp_pos) . UTF8_REPLACEMENT;
										$pos += $utf_len;
										$tmp_pos = $pos;
										continue 2;
									}
								break;

								// Note: 0xEF is quite common in Japanese
								case "\xEF":
									if ($utf_char == "\xEF\xBF\xBE" || $utf_char == "\xEF\xBF\xBF")
									{
										// U+FFFE and U+FFFF are explicitly disallowed (UTF sequence 0xEFBFBE..0xEFBFBF)
										$tmp .= substr($str, $tmp_pos, $pos - $tmp_pos) . UTF8_REPLACEMENT;
										$pos += $utf_len;
										$tmp_pos = $pos;
										continue 2;
									}
								break;

								case "\xC0":
								case "\xC1":
									if ($utf_char <= "\xC1\xBF")
									{
										// Overlong sequence: Unicode char U+0000..U+007F encoded as a double-byte UTF char
										$tmp .= substr($str, $tmp_pos, $pos - $tmp_pos) . UTF8_REPLACEMENT;
										$pos += $utf_len;
										$tmp_pos = $pos;
										continue 2;
									}
								break;

								case "\xE0":
									if ($utf_char <= "\xE0\x9F\xBF")
									{
										// Unicode char U+0000..U+07FF encoded in 3 bytes
										$tmp .= substr($str, $tmp_pos, $pos - $tmp_pos) . UTF8_REPLACEMENT;
										$pos += $utf_len;
										$tmp_pos = $pos;
										continue 2;
									}
								break;

								case "\xF0":
									if ($utf_char <= "\xF0\x8F\xBF\xBF")
									{
										// Unicode char U+0000..U+FFFF encoded in 4 bytes
										$tmp .= substr($str, $tmp_pos, $pos - $tmp_pos) . UTF8_REPLACEMENT;
										$pos += $utf_len;
										$tmp_pos = $pos;
										continue 2;
									}
								break;

								default:
									// Five- and six- byte sequences do not need being checked for here anymore
									if ($utf_char > UTF8_MAX)
									{
										// Out of the Unicode range
										if ($utf_char[0] < "\xF8")
										{
											$trailing_bytes = 3;
										}
										else if ($utf_char[0] < "\xFC")
										{
											$trailing_bytes = 4;
										}
										else if ($utf_char[0] > "\xFD")
										{
											$trailing_bytes = 0;
										}
										else
										{
											$trailing_bytes = 5;
										}

										$tmp .= substr($str, $tmp_pos, $pos - $tmp_pos) . UTF8_REPLACEMENT;
										$pos += strspn($str, UTF8_TRAILING_BYTES, ++$pos, $trailing_bytes);
										$tmp_pos = $pos;
										continue 2;
									}
								break;
							}
						}

						// The char is a valid starter, move the cursor and go on
						$pos += $utf_len;
						continue;
					}
				}
				else
				{
					// A trailing byte came out of nowhere, we will advance the cursor and treat the this byte and all following trailing bytes as if
					// each of them was a Unicode replacement char
					$spn = strspn($str, UTF8_TRAILING_BYTES, $pos);
					$tmp .= substr($str, $tmp_pos, $pos - $tmp_pos) . str_repeat(UTF8_REPLACEMENT, $spn);

					$pos += $spn;
					$tmp_pos = $pos;
					continue;
				}


				// STEP 1: Decompose current char

				// We have found a character that is either:
				//  - in the NFC_QC/NFKC_QC list
				//  - a non-starter char that is not canonically ordered
				//
				// We are going to capture the shortest UTF sequence that satisfies these two conditions:
				//
				//  1 - If the sequence does not start at the begginning of the string, it must begin with a starter,
				// and that starter must not have the NF[K]C_QC property equal to "MAYBE"
				//
				//  2 - If the sequence does not end at the end of the string, it must end with a non-starter and be
				// immediately followed by a starter that is not on the QC list
				//
				$utf_seq = array();
				$last_cc = 0;
				$lpos = $pos;
				$pos += $utf_len;

				if (isset($decomp_map[$utf_char]))
				{
					$_pos = 0;
					$_len = strlen($decomp_map[$utf_char]);

					do
					{
						$_utf_len =& $utf_len_mask[$decomp_map[$utf_char][$_pos] & "\xF0"];

						if (isset($_utf_len))
						{
							$utf_seq[] = substr($decomp_map[$utf_char], $_pos, $_utf_len);
							$_pos += $_utf_len;
						}
						else
						{
							$utf_seq[] = $decomp_map[$utf_char][$_pos];
							++$_pos;
						}
					}
					while ($_pos < $_len);
				}
				else
				{
					// The char is not decomposable
					$utf_seq = array($utf_char);
				}


				// STEP 2: Capture the starter

				// Check out the combining class of the first character of the UTF sequence
				$k = 0;
				if (isset($utf_combining_class[$utf_seq[0]]) || $qc[$utf_char] == UNICODE_QC_MAYBE)
				{
					// Not a starter, inspect previous characters
					// The last 8 characters are kept in a buffer so that we don't have to capture them everytime.
					// This is enough for all real-life strings but even if it wasn't, we can capture characters in backward mode,
					// although it is slower than this method.
					//
					// In the following loop, $j starts at the previous buffered character ($i - 1, because current character is
					// at offset $i) and process them in backward mode until we find a starter.
					//
					// $k is the index on each UTF character inside of our UTF sequence. At this time, $utf_seq contains one or more
					// characters numbered 0 to n. $k starts at 0 and for each char we prepend we pre-decrement it and for numbering
					$starter_found = 0;
					$j_min = max(1, $i - 7);

					for ($j = $i - 1; $j >= $j_min && $lpos > $tmp_pos; --$j)
					{
						$utf_char = $buffer[$j & 7];
						$lpos -= strlen($utf_char);

						if (isset($decomp_map[$utf_char]))
						{
							// The char is a composite, decompose for storage
							$decomp_seq = array();
							$_pos = 0;
							$_len = strlen($decomp_map[$utf_char]);

							do
							{
								$c = $decomp_map[$utf_char][$_pos];
								$_utf_len =& $utf_len_mask[$c & "\xF0"];

								if (isset($_utf_len))
								{
									$decomp_seq[] = substr($decomp_map[$utf_char], $_pos, $_utf_len);
									$_pos += $_utf_len;
								}
								else
								{
									$decomp_seq[] = $c;
									++$_pos;
								}
							}
							while ($_pos < $_len);

							// Prepend the UTF sequence with our decomposed sequence
							if (isset($decomp_seq[1]))
							{
								// The char expanded into several chars
								$decomp_cnt = sizeof($decomp_seq);

								foreach ($decomp_seq as $decomp_i => $decomp_char)
								{
									$utf_seq[$k + $decomp_i - $decomp_cnt] = $decomp_char;
								}
								$k -= $decomp_cnt;
							}
							else
							{
								// Decomposed to a single char, easier to prepend
								$utf_seq[--$k] = $decomp_seq[0];
							}
						}
						else
						{
							$utf_seq[--$k] = $utf_char;
						}

						if (!isset($utf_combining_class[$utf_seq[$k]]))
						{
							// We have found our starter
							$starter_found = 1;
							break;
						}
					}

					if (!$starter_found && $lpos > $tmp_pos)
					{
						// The starter was not found in the buffer, let's rewind some more
						do
						{
							// $utf_len_mask contains the masks of both leading bytes and trailing bytes. If $utf_en > 0 then it's a leading byte, otherwise it's a trailing byte.
							$c = $str[--$lpos];
							$c_mask = $c & "\xF0";

							if (isset($utf_len_mask[$c_mask]))
							{
								// UTF byte
								if ($utf_len = $utf_len_mask[$c_mask])
								{
									// UTF *leading* byte
									$utf_char = substr($str, $lpos, $utf_len);

									if (isset($decomp_map[$utf_char]))
									{
										// Decompose the character
										$decomp_seq = array();
										$_pos = 0;
										$_len = strlen($decomp_map[$utf_char]);

										do
										{
											$c = $decomp_map[$utf_char][$_pos];
											$_utf_len =& $utf_len_mask[$c & "\xF0"];

											if (isset($_utf_len))
											{
												$decomp_seq[] = substr($decomp_map[$utf_char], $_pos, $_utf_len);
												$_pos += $_utf_len;
											}
											else
											{
												$decomp_seq[] = $c;
												++$_pos;
											}
										}
										while ($_pos < $_len);

										// Prepend the UTF sequence with our decomposed sequence
										if (isset($decomp_seq[1]))
										{
											// The char expanded into several chars
											$decomp_cnt = sizeof($decomp_seq);
											foreach ($decomp_seq as $decomp_i => $utf_char)
											{
												$utf_seq[$k + $decomp_i - $decomp_cnt] = $utf_char;
											}
											$k -= $decomp_cnt;
										}
										else
										{
											// Decomposed to a single char, easier to prepend
											$utf_seq[--$k] = $decomp_seq[0];
										}
									}
									else
									{
										$utf_seq[--$k] = $utf_char;
									}
								}
							}
							else
							{
								// ASCII char
								$utf_seq[--$k] = $c;
							}
						}
						while ($lpos > $tmp_pos);
					}
				}


				// STEP 3: Capture following combining modifiers

				while ($pos < $len)
				{
					$c_mask = $str[$pos] & "\xF0";

					if (isset($utf_len_mask[$c_mask]))
					{
						if ($utf_len = $utf_len_mask[$c_mask])
						{
							$utf_char = substr($str, $pos, $utf_len);
						}
						else
						{
							// A trailing byte came out of nowhere
							// Trailing bytes are replaced with Unicode replacement chars, we will just ignore it for now, break out of the loop
							// as if it was a starter (replacement chars ARE starters) and let the next loop replace it
							break;
						}

						if (isset($utf_combining_class[$utf_char]) || isset($qc[$utf_char]))
						{
							// Combining character, add it to the sequence and move the cursor
							if (isset($decomp_map[$utf_char]))
							{
								// Decompose the character
								$_pos = 0;
								$_len = strlen($decomp_map[$utf_char]);

								do
								{
									$c = $decomp_map[$utf_char][$_pos];
									$_utf_len =& $utf_len_mask[$c & "\xF0"];

									if (isset($_utf_len))
									{
										$utf_seq[] = substr($decomp_map[$utf_char], $_pos, $_utf_len);
										$_pos += $_utf_len;
									}
									else
									{
										$utf_seq[] = $c;
										++$_pos;
									}
								}
								while ($_pos < $_len);
							}
							else
							{
								$utf_seq[] = $utf_char;
							}

							$pos += $utf_len;
						}
						else
						{
							// Combining class 0 and no QC, break out of the loop
							// Note: we do not know if that character is valid. If it's not, the next iteration will replace it
							break;
						}
					}
					else
					{
						// ASCII chars are starters
						break;
					}
				}


				// STEP 4: Sort and combine

				// Here we sort...
				$k_max = $k + sizeof($utf_seq);

				if (!$k && $k_max == 1)
				{
					// There is only one char in the UTF sequence, add it then jump to the next iteration of main loop
						// Note: the two commented lines below can be enabled under PHP5 for a very small performance gain in most cases
//						if (substr_compare($str, $utf_seq[0], $lpos, $pos - $lpos))
//						{
						$tmp .= substr($str, $tmp_pos, $lpos - $tmp_pos) . $utf_seq[0];
						$tmp_pos = $pos;
//						}

					continue;
				}

				// ...there we combine
				if (isset($utf_combining_class[$utf_seq[$k]]))
				{
					$starter = $nf_seq = '';
				}
				else
				{
					$starter = $utf_seq[$k++];
					$nf_seq = '';
				}
				$utf_sort = array();

				// We add an empty char at the end of the UTF char sequence. It will act as a starter and trigger the sort/combine routine
				// at the end of the string without altering it
				$utf_seq[] = '';

				do
				{
					$utf_char = $utf_seq[$k++];

					if (isset($utf_combining_class[$utf_char]))
					{
						$utf_sort[$utf_combining_class[$utf_char]][] = $utf_char;
					}
					else
					{
						if (empty($utf_sort))
						{
							// No combining characters... check for a composite of the two starters
							if (isset($utf_canonical_comp[$starter . $utf_char]))
							{
								// Good ol' composite character
								$starter = $utf_canonical_comp[$starter . $utf_char];
							}
							else if (isset($utf_jamo_type[$utf_char]))
							{
								// Current char is a composable jamo
								if (isset($utf_jamo_type[$starter]) && $utf_jamo_type[$starter] == UNICODE_JAMO_L && $utf_jamo_type[$utf_char] == UNICODE_JAMO_V)
								{
									// We have a L jamo followed by a V jamo, we are going to prefetch the next char to see if it's a T jamo
									if (isset($utf_jamo_type[$utf_seq[$k]]) && $utf_jamo_type[$utf_seq[$k]] == UNICODE_JAMO_T)
									{
										// L+V+T jamos, combine to a LVT Hangul syllable ($k is incremented)
										$cp = $utf_jamo_index[$starter] + $utf_jamo_index[$utf_char] + $utf_jamo_index[$utf_seq[$k]];
										++$k;
									}
									else
									{
										// L+V jamos, combine to a LV Hangul syllable
										$cp = $utf_jamo_index[$starter] + $utf_jamo_index[$utf_char];
									}

									$starter = chr(0xE0 | ($cp >> 12)) . chr(0x80 | (($cp >> 6) & 0x3F)) . chr(0x80 | ($cp & 0x3F));
								}
								else
								{
									// Non-composable jamo, just add it to the sequence
									$nf_seq .= $starter;
									$starter = $utf_char;
								}
							}
							else
							{
								// No composite, just add the first starter to the sequence then continue with the other one
								$nf_seq .= $starter;
								$starter = $utf_char;
							}
						}
						else
						{
							ksort($utf_sort);

							// For each class of combining characters
							foreach ($utf_sort as $cc => $utf_chars)
							{
								$j = 0;

								do
								{
									// Look for a composite
									if (isset($utf_canonical_comp[$starter . $utf_chars[$j]]))
									{
										// Found a composite, replace the starter
										$starter = $utf_canonical_comp[$starter . $utf_chars[$j]];
										unset($utf_sort[$cc][$j]);
									}
									else
									{
										// No composite, all following characters in that class are blocked
										break;
									}
								}
								while (isset($utf_sort[$cc][++$j]));
							}

							// Add the starter to the normalized sequence, followed by non-starters in canonical order
							$nf_seq .= $starter;

							foreach ($utf_sort as $utf_chars)
							{
								if (!empty($utf_chars))
								{
									$nf_seq .= implode('', $utf_chars);
								}
							}

							// Reset the array and go on
							$utf_sort = array();
							$starter = $utf_char;
						}
					}
				}
				while ($k <= $k_max);

				$tmp .= substr($str, $tmp_pos, $lpos - $tmp_pos) . $nf_seq;
				$tmp_pos = $pos;
			}
			else
			{
				// Only a ASCII char can make the program get here
				//
				// First we skip the current byte with ++$pos, then we quickly skip following ASCII chars with strspn().
				//
				// The first two "if"'s here can be removed, with the consequences of being faster on latin text (lots of ASCII) and slower on
				// multi-byte text (where the only ASCII chars are spaces and punctuation)
				if (++$pos != $len)
				{
					if ($str[$pos] < "\x80")
					{
						$pos += strspn($str, UTF8_ASCII_RANGE, ++$pos);
						$buffer[++$i & 7] = $str[$pos - 1];
					}
					else
					{
						$buffer[++$i & 7] = $c;
					}
				}
			}
		}
		while ($pos < $len);

		// Now is time to return the string
		if ($tmp_pos)
		{
			// If the $tmp_pos cursor is not at the beggining of the string then at least one character was not in normal form. Replace $str with the fixed version
			if ($tmp_pos == $len)
			{
				// The $tmp_pos cursor is at the end of $str, therefore $tmp holds the whole $str
				return $tmp;
			}
			else
			{
				// The rightmost chunk of $str has not been appended to $tmp yet
				return $tmp . substr($str, $tmp_pos);
			}
		}

		// The string was already in normal form
		return $str;
	}

	/**
	* Decompose a UTF string
	*
	* @param	string	$str			UTF string
	* @param	integer	$pos			Position of the first UTF char (in bytes)
	* @param	integer	$len			Length of the string (in bytes)
	* @param	array	&$decomp_map	Decomposition mapping, passed by reference but never modified
	* @return	string					The string, decomposed and sorted canonically
	*
	* @access	private
	*/
	function decompose($str, $pos, $len, &$decomp_map)
	{
		global $utf_combining_class;

		// Load some commonly-used tables
		if (!isset($utf_combining_class))
		{
			global $phpbb_root_path, $phpEx;
			include($phpbb_root_path . 'includes/utf/data/utf_normalizer_common.' . $phpEx);
		}

		// UTF char length array
		$utf_len_mask = array(
			// Leading bytes masks
			"\xC0" => 2, "\xD0" => 2, "\xE0" => 3, "\xF0" => 4,
			// Trailing bytes masks
			"\x80" => 0, "\x90" => 0, "\xA0" => 0, "\xB0" => 0
		);

		// Some extra checks are triggered on the first byte of a UTF sequence
		$extra_check = array(
			"\xED" => 1, "\xEF" => 1, "\xC0" => 1, "\xC1" => 1, "\xE0" => 1, "\xF0" => 1,
			"\xF4" => 1, "\xF5" => 1, "\xF6" => 1, "\xF7" => 1, "\xF8" => 1, "\xF9" => 1,
			"\xFA" => 1, "\xFB" => 1, "\xFC" => 1, "\xFD" => 1, "\xFE" => 1, "\xFF" => 1
		);

		// These masks are used to check if a UTF sequence is well formed. Here are the only 3 lengths we acknowledge:
		//   - 2-byte: 110? ???? 10?? ????
		//   - 3-byte: 1110 ???? 10?? ???? 10?? ????
		//   - 4-byte: 1111 0??? 10?? ???? 10?? ???? 10?? ????
		// Note that 5- and 6- byte sequences are automatically discarded
		$utf_validation_mask = array(
			2	=> "\xE0\xC0",
			3	=> "\xF0\xC0\xC0",
			4	=> "\xF8\xC0\xC0\xC0"
		);

		$utf_validation_check = array(
			2	=> "\xC0\x80",
			3	=> "\xE0\x80\x80",
			4	=> "\xF0\x80\x80\x80"
		);

		$tmp = '';
		$starter_pos = $pos;
		$tmp_pos = $last_cc = $sort = $dump = 0;
		$utf_sort = array();


		// Main loop
		do
		{
			// STEP 0: Capture the current char

			$cur_mask = $str[$pos] & "\xF0";
			if (isset($utf_len_mask[$cur_mask]))
			{
				if ($utf_len = $utf_len_mask[$cur_mask])
				{
					// Multibyte char
					$utf_char = substr($str, $pos, $utf_len);
					$pos += $utf_len;
				}
				else
				{
					// A trailing byte came out of nowhere, we will treat it and all following trailing bytes as if each of them was a Unicode
					// replacement char and we will advance the cursor
					$spn = strspn($str, UTF8_TRAILING_BYTES, $pos);

					if ($dump)
					{
						$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

						// Dump combiners
						if (!empty($utf_sort))
						{
							if ($sort)
							{
								ksort($utf_sort);
							}

							foreach ($utf_sort as $utf_chars)
							{
								$tmp .= implode('', $utf_chars);
							}
						}

						$tmp .= str_repeat(UTF8_REPLACEMENT, $spn);
						$dump = $sort = 0;
					}
					else
					{
						$tmp .= substr($str, $tmp_pos, $pos - $tmp_pos) . str_repeat(UTF8_REPLACEMENT, $spn);
					}

					$pos += $spn;
					$tmp_pos = $starter_pos = $pos;

					$utf_sort = array();
					$last_cc = 0;

					continue;
				}


				// STEP 1: Decide what to do with current char

				// Now, in that order:
				//  - check if that character is decomposable
				//  - check if that character is a non-starter
				//  - check if that character requires extra checks to be performed
				if (isset($decomp_map[$utf_char]))
				{
					// Decompose the char
					$_pos = 0;
					$_len = strlen($decomp_map[$utf_char]);

					do
					{
						$c = $decomp_map[$utf_char][$_pos];
						$_utf_len =& $utf_len_mask[$c & "\xF0"];

						if (isset($_utf_len))
						{
							$_utf_char = substr($decomp_map[$utf_char], $_pos, $_utf_len);
							$_pos += $_utf_len;

							if (isset($utf_combining_class[$_utf_char]))
							{
								// The character decomposed to a non-starter, buffer it for sorting
								$utf_sort[$utf_combining_class[$_utf_char]][] = $_utf_char;

								if ($utf_combining_class[$_utf_char] < $last_cc)
								{
									// Not canonically ordered, will require sorting
									$sort = $dump = 1;
								}
								else
								{
									$dump = 1;
									$last_cc = $utf_combining_class[$_utf_char];
								}
							}
							else
							{
								// This character decomposition contains a starter, dump the buffer and continue
								if ($dump)
								{
									$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

									// Dump combiners
									if (!empty($utf_sort))
									{
										if ($sort)
										{
											ksort($utf_sort);
										}

										foreach ($utf_sort as $utf_chars)
										{
											$tmp .= implode('', $utf_chars);
										}
									}

									$tmp .= $_utf_char;
									$dump = $sort = 0;
								}
								else
								{
									$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos) . $_utf_char;
								}

								$tmp_pos = $starter_pos = $pos;
								$utf_sort = array();
								$last_cc = 0;
							}
						}
						else
						{
							// This character decomposition contains an ASCII char, which is a starter. Dump the buffer and continue
							++$_pos;

							if ($dump)
							{
								$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

								// Dump combiners
								if (!empty($utf_sort))
								{
									if ($sort)
									{
										ksort($utf_sort);
									}

									foreach ($utf_sort as $utf_chars)
									{
										$tmp .= implode('', $utf_chars);
									}
								}

								$tmp .= $c;
								$dump = $sort = 0;
							}
							else
							{
								$tmp .= substr($str, $tmp_pos, $pos - $utf_len - $tmp_pos) . $c;
							}

							$tmp_pos = $starter_pos = $pos;
							$utf_sort = array();
							$last_cc = 0;
						}
					}
					while ($_pos < $_len);
				}
				else if (isset($utf_combining_class[$utf_char]))
				{
					// Combining character
					if ($utf_combining_class[$utf_char] < $last_cc)
					{
						// Not in canonical order
						$sort = $dump = 1;
					}
					else
					{
						$last_cc = $utf_combining_class[$utf_char];
					}

					$utf_sort[$utf_combining_class[$utf_char]][] = $utf_char;
				}
				else
				{
					// Non-decomposable starter, check out if it's a Hangul syllable
					if ($utf_char < UTF8_HANGUL_FIRST || $utf_char > UTF8_HANGUL_LAST)
					{
						// Nope, regular UTF char, check that we have the correct number of trailing bytes
						if (($utf_char & $utf_validation_mask[$utf_len]) != $utf_validation_check[$utf_len])
						{
							// Current char isn't well-formed or legal: either one or several trailing bytes are missing, or the Unicode char
							// has been encoded in a five- or six- byte sequence.
							// Move the cursor back to its original position then advance it to the position it should really be at
							$pos -= $utf_len;
							$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

							if (!empty($utf_sort))
							{
								ksort($utf_sort);

								foreach ($utf_sort as $utf_chars)
								{
									$tmp .= implode('', $utf_chars);
								}
								$utf_sort = array();
							}

							// Add a replacement char then another replacement char for every trailing byte.
							//
							// @todo I'm not entirely sure that's how we're supposed to mark invalidated byte sequences, check this
							$spn = strspn($str, UTF8_TRAILING_BYTES, ++$pos);
							$tmp .= str_repeat(UTF8_REPLACEMENT, $spn + 1);

							$dump = $sort = 0;

							$pos += $spn;
							$tmp_pos = $pos;
							continue;
						}

						if (isset($extra_check[$utf_char[0]]))
						{
							switch ($utf_char[0])
							{
								// Note: 0xED is quite common in Korean
								case "\xED":
									if ($utf_char >= "\xED\xA0\x80")
									{
										// Surrogates (U+D800..U+DFFF) are not allowed in UTF-8 (UTF sequence 0xEDA080..0xEDBFBF)
										$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

										if (!empty($utf_sort))
										{
											ksort($utf_sort);

											foreach ($utf_sort as $utf_chars)
											{
												$tmp .= implode('', $utf_chars);
											}
											$utf_sort = array();
										}

										$tmp .= UTF8_REPLACEMENT;
										$dump = $sort = 0;

										$tmp_pos = $starter_pos = $pos;
										continue 2;
									}
								break;

								// Note: 0xEF is quite common in Japanese
								case "\xEF":
									if ($utf_char == "\xEF\xBF\xBE" || $utf_char == "\xEF\xBF\xBF")
									{
										// U+FFFE and U+FFFF are explicitly disallowed (UTF sequence 0xEFBFBE..0xEFBFBF)
										$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

										if (!empty($utf_sort))
										{
											ksort($utf_sort);

											foreach ($utf_sort as $utf_chars)
											{
												$tmp .= implode('', $utf_chars);
											}
											$utf_sort = array();
										}

										$tmp .= UTF8_REPLACEMENT;
										$dump = $sort = 0;

										$tmp_pos = $starter_pos = $pos;
										continue 2;
									}
								break;

								case "\xC0":
								case "\xC1":
									if ($utf_char <= "\xC1\xBF")
									{
										// Overlong sequence: Unicode char U+0000..U+007F encoded as a double-byte UTF char
										$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

										if (!empty($utf_sort))
										{
											ksort($utf_sort);

											foreach ($utf_sort as $utf_chars)
											{
												$tmp .= implode('', $utf_chars);
											}
											$utf_sort = array();
										}

										$tmp .= UTF8_REPLACEMENT;
										$dump = $sort = 0;

										$tmp_pos = $starter_pos = $pos;
										continue 2;
									}
								break;

								case "\xE0":
									if ($utf_char <= "\xE0\x9F\xBF")
									{
										// Unicode char U+0000..U+07FF encoded in 3 bytes
										$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

										if (!empty($utf_sort))
										{
											ksort($utf_sort);

											foreach ($utf_sort as $utf_chars)
											{
												$tmp .= implode('', $utf_chars);
											}
											$utf_sort = array();
										}

										$tmp .= UTF8_REPLACEMENT;
										$dump = $sort = 0;

										$tmp_pos = $starter_pos = $pos;
										continue 2;
									}
								break;

								case "\xF0":
									if ($utf_char <= "\xF0\x8F\xBF\xBF")
									{
										// Unicode char U+0000..U+FFFF encoded in 4 bytes
										$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

										if (!empty($utf_sort))
										{
											ksort($utf_sort);

											foreach ($utf_sort as $utf_chars)
											{
												$tmp .= implode('', $utf_chars);
											}
											$utf_sort = array();
										}

										$tmp .= UTF8_REPLACEMENT;
										$dump = $sort = 0;

										$tmp_pos = $starter_pos = $pos;
										continue 2;
									}
								break;

								default:
									if ($utf_char > UTF8_MAX)
									{
										// Out of the Unicode range
										$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

										if (!empty($utf_sort))
										{
											ksort($utf_sort);

											foreach ($utf_sort as $utf_chars)
											{
												$tmp .= implode('', $utf_chars);
											}
											$utf_sort = array();
										}

										$tmp .= UTF8_REPLACEMENT;
										$dump = $sort = 0;

										$tmp_pos = $starter_pos = $pos;
										continue 2;
									}
								break;
							}
						}
					}
					else
					{
						// Hangul syllable
						$idx = (((ord($utf_char[0]) & 0x0F) << 12) | ((ord($utf_char[1]) & 0x3F) << 6) | (ord($utf_char[2]) & 0x3F)) - UNICODE_HANGUL_SBASE;

						// LIndex can only range from 0 to 18, therefore it cannot influence the first two bytes of the L Jamo, which allows us to hardcode them (based on LBase).
						//
						// The same goes for VIndex, but for TIndex there's a catch: the value of the third byte could exceed 0xBF and we would have to increment the second byte
						if ($t_index = $idx % UNICODE_HANGUL_TCOUNT)
						{
							if ($t_index < 25)
							{
								$utf_char = "\xE1\x84\x00\xE1\x85\x00\xE1\x86\x00";
								$utf_char[8] = chr(0xA7 + $t_index);
							}
							else
							{
								$utf_char = "\xE1\x84\x00\xE1\x85\x00\xE1\x87\x00";
								$utf_char[8] = chr(0x67 + $t_index);
							}
						}
						else
						{
							$utf_char = "\xE1\x84\x00\xE1\x85\x00";
						}

						$utf_char[2] = chr(0x80 + (int) ($idx / UNICODE_HANGUL_NCOUNT));
						$utf_char[5] = chr(0xA1 + (int) (($idx % UNICODE_HANGUL_NCOUNT) / UNICODE_HANGUL_TCOUNT));

						// Just like other decompositions, the resulting Jamos must be dumped to the tmp string
						$dump = 1;
					}

					// Do we need to dump stuff to the tmp string?
					if ($dump)
					{
						$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

						// Dump combiners
						if (!empty($utf_sort))
						{
							if ($sort)
							{
								ksort($utf_sort);
							}

							foreach ($utf_sort as $utf_chars)
							{
								$tmp .= implode('', $utf_chars);
							}
						}

						$tmp .= $utf_char;
						$dump = $sort = 0;
						$tmp_pos = $pos;
					}

					$last_cc = 0;
					$utf_sort = array();
					$starter_pos = $pos;
				}
			}
			else
			{
				// ASCII char, which happens to be a starter (as any other ASCII char)
				if ($dump)
				{
					$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

					// Dump combiners
					if (!empty($utf_sort))
					{
						if ($sort)
						{
							ksort($utf_sort);
						}

						foreach ($utf_sort as $utf_chars)
						{
							$tmp .= implode('', $utf_chars);
						}
					}

					$tmp .= $str[$pos];
					$dump = $sort = 0;
					$tmp_pos = ++$pos;

					$pos += strspn($str, UTF8_ASCII_RANGE, $pos);
				}
				else
				{
					$pos += strspn($str, UTF8_ASCII_RANGE, ++$pos);
				}

				$last_cc = 0;
				$utf_sort = array();
				$starter_pos = $pos;
			}
		}
		while ($pos < $len);

		// Now is time to return the string
		if ($dump)
		{
			$tmp .= substr($str, $tmp_pos, $starter_pos - $tmp_pos);

			// Dump combiners
			if (!empty($utf_sort))
			{
				if ($sort)
				{
					ksort($utf_sort);
				}

				foreach ($utf_sort as $utf_chars)
				{
					$tmp .= implode('', $utf_chars);
				}
			}

			return $tmp;
		}
		else if ($tmp_pos)
		{
			// If the $tmp_pos cursor was moved then at least one character was not in normal form. Replace $str with the fixed version
			if ($tmp_pos == $len)
			{
				// The $tmp_pos cursor is at the end of $str, therefore $tmp holds the whole $str
				return $tmp;
			}
			else
			{
				// The rightmost chunk of $str has not been appended to $tmp yet
				return $tmp . substr($str, $tmp_pos);
			}
		}

		// The string was already in normal form
		return $str;
	}
}

?>