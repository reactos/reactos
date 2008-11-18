<?php
/**
*
* @package phpBB3
* @version $Id: functions_template.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group, sections (c) 2001 ispi of Lincoln Inc
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
* Extension of template class - Functions needed for compiling templates only.
*
* psoTFX, phpBB Development Team - Completion of file caching, decompilation
* routines and implementation of conditionals/keywords and associated changes
*
* The interface was inspired by PHPLib templates,  and the template file (formats are
* quite similar)
*
* The keyword/conditional implementation is currently based on sections of code from
* the Smarty templating engine (c) 2001 ispi of Lincoln, Inc. which is released
* (on its own and in whole) under the LGPL. Section 3 of the LGPL states that any code
* derived from an LGPL application may be relicenced under the GPL, this applies
* to this source
*
* DEFINE directive inspired by a request by Cyberalien
*
* @package phpBB3
*/
class template_compile
{
	var $template;

	// Various storage arrays
	var $block_names = array();
	var $block_else_level = array();

	/**
	* constuctor
	*/
	function template_compile(&$template)
	{
		$this->template = &$template;
	}
	
	/**
	* Load template source from file
	* @access private
	*/
	function _tpl_load_file($handle, $store_in_db = false)
	{
		// Try and open template for read
		if (!file_exists($this->template->files[$handle]))
		{
			trigger_error("template->_tpl_load_file(): File {$this->template->files[$handle]} does not exist or is empty", E_USER_ERROR);
		}

		$this->template->compiled_code[$handle] = $this->compile(trim(@file_get_contents($this->template->files[$handle])));

		// Actually compile the code now.
		$this->compile_write($handle, $this->template->compiled_code[$handle]);

		// Store in database if required...
		if ($store_in_db)
		{
			global $db, $user;

			$sql_ary = array(
				'template_id'			=> $user->theme['template_id'],
				'template_filename'		=> $this->template->filename[$handle],
				'template_included'		=> '',
				'template_mtime'		=> time(),
				'template_data'			=> trim(@file_get_contents($this->template->files[$handle])),
			);

			$sql = 'INSERT INTO ' . STYLES_TEMPLATE_DATA_TABLE . ' ' . $db->sql_build_array('INSERT', $sql_ary);
			$db->sql_query($sql);
		}
	}

	/**
	* Remove any PHP tags that do not belong, these regular expressions are derived from
	* the ones that exist in zend_language_scanner.l
	* @access private
	*/
	function remove_php_tags(&$code)
	{
		// This matches the information gathered from the internal PHP lexer
		$match = array(
			'#<([\?%])=?.*?\1>#s',
			'#<script\s+language\s*=\s*(["\']?)php\1\s*>.*?</script\s*>#s',
			'#<\?php(?:\r\n?|[ \n\t]).*?\?>#s'
		);

		$code = preg_replace($match, '', $code);
	}

	/**
	* The all seeing all doing compile method. Parts are inspired by or directly from Smarty
	* @access private
	*/
	function compile($code, $no_echo = false, $echo_var = '')
	{
		global $config;

		if ($echo_var)
		{
			global $$echo_var;
		}

		// Remove any "loose" php ... we want to give admins the ability
		// to switch on/off PHP for a given template. Allowing unchecked
		// php is a no-no. There is a potential issue here in that non-php
		// content may be removed ... however designers should use entities
		// if they wish to display < and >
		$this->remove_php_tags($code);

		// Pull out all block/statement level elements and separate plain text
		preg_match_all('#<!-- PHP -->(.*?)<!-- ENDPHP -->#s', $code, $matches);
		$php_blocks = $matches[1];
		$code = preg_replace('#<!-- PHP -->.*?<!-- ENDPHP -->#s', '<!-- PHP -->', $code);

		preg_match_all('#<!-- INCLUDE ([a-zA-Z0-9\_\-\+\./]+) -->#', $code, $matches);
		$include_blocks = $matches[1];
		$code = preg_replace('#<!-- INCLUDE [a-zA-Z0-9\_\-\+\./]+ -->#', '<!-- INCLUDE -->', $code);

		preg_match_all('#<!-- INCLUDEPHP ([a-zA-Z0-9\_\-\+\./]+) -->#', $code, $matches);
		$includephp_blocks = $matches[1];
		$code = preg_replace('#<!-- INCLUDEPHP [a-zA-Z0-9\_\-\+\./]+ -->#', '<!-- INCLUDEPHP -->', $code);

		preg_match_all('#<!-- ([^<].*?) (.*?)? ?-->#', $code, $blocks, PREG_SET_ORDER);

		$text_blocks = preg_split('#<!-- [^<].*? (?:.*?)? ?-->#', $code);

		for ($i = 0, $j = sizeof($text_blocks); $i < $j; $i++)
		{
			$this->compile_var_tags($text_blocks[$i]);
		}
		$compile_blocks = array();

		for ($curr_tb = 0, $tb_size = sizeof($blocks); $curr_tb < $tb_size; $curr_tb++)
		{
			$block_val = &$blocks[$curr_tb];

			switch ($block_val[1])
			{
				case 'BEGIN':
					$this->block_else_level[] = false;
					$compile_blocks[] = '<?php ' . $this->compile_tag_block($block_val[2]) . ' ?>';
				break;

				case 'BEGINELSE':
					$this->block_else_level[sizeof($this->block_else_level) - 1] = true;
					$compile_blocks[] = '<?php }} else { ?>';
				break;

				case 'END':
					array_pop($this->block_names);
					$compile_blocks[] = '<?php ' . ((array_pop($this->block_else_level)) ? '}' : '}}') . ' ?>';
				break;

				case 'IF':
					$compile_blocks[] = '<?php ' . $this->compile_tag_if($block_val[2], false) . ' ?>';
				break;

				case 'ELSE':
					$compile_blocks[] = '<?php } else { ?>';
				break;

				case 'ELSEIF':
					$compile_blocks[] = '<?php ' . $this->compile_tag_if($block_val[2], true) . ' ?>';
				break;

				case 'ENDIF':
					$compile_blocks[] = '<?php } ?>';
				break;

				case 'DEFINE':
					$compile_blocks[] = '<?php ' . $this->compile_tag_define($block_val[2], true) . ' ?>';
				break;

				case 'UNDEFINE':
					$compile_blocks[] = '<?php ' . $this->compile_tag_define($block_val[2], false) . ' ?>';
				break;

				case 'INCLUDE':
					$temp = array_shift($include_blocks);
					$compile_blocks[] = '<?php ' . $this->compile_tag_include($temp) . ' ?>';
					$this->template->_tpl_include($temp, false);
				break;

				case 'INCLUDEPHP':
					$compile_blocks[] = ($config['tpl_allow_php']) ? '<?php ' . $this->compile_tag_include_php(array_shift($includephp_blocks)) . ' ?>' : '';
				break;

				case 'PHP':
					$compile_blocks[] = ($config['tpl_allow_php']) ? '<?php ' . array_shift($php_blocks) . ' ?>' : '';
				break;

				default:
					$this->compile_var_tags($block_val[0]);
					$trim_check = trim($block_val[0]);
					$compile_blocks[] = (!$no_echo) ? ((!empty($trim_check)) ? $block_val[0] : '') : ((!empty($trim_check)) ? $block_val[0] : '');
				break;
			}
		}

		$template_php = '';
		for ($i = 0, $size = sizeof($text_blocks); $i < $size; $i++)
		{
			$trim_check_text = trim($text_blocks[$i]);
			$template_php .= (!$no_echo) ? (($trim_check_text != '') ? $text_blocks[$i] : '') . ((isset($compile_blocks[$i])) ? $compile_blocks[$i] : '') : (($trim_check_text != '') ? $text_blocks[$i] : '') . ((isset($compile_blocks[$i])) ? $compile_blocks[$i] : '');
		}

		// There will be a number of occasions where we switch into and out of
		// PHP mode instantaneously. Rather than "burden" the parser with this
		// we'll strip out such occurences, minimising such switching
		$template_php = str_replace(' ?><?php ', ' ', $template_php);

		return (!$no_echo) ? $template_php : "\$$echo_var .= '" . $template_php . "'";
	}

	/**
	* Compile variables
	* @access private
	*/
	function compile_var_tags(&$text_blocks)
	{
		// change template varrefs into PHP varrefs
		$varrefs = array();

		// This one will handle varrefs WITH namespaces
		preg_match_all('#\{((?:[a-z0-9\-_]+\.)+)(\$)?([A-Z0-9\-_]+)\}#', $text_blocks, $varrefs, PREG_SET_ORDER);

		foreach ($varrefs as $var_val)
		{
			$namespace = $var_val[1];
			$varname = $var_val[3];
			$new = $this->generate_block_varref($namespace, $varname, true, $var_val[2]);

			$text_blocks = str_replace($var_val[0], $new, $text_blocks);
		}

		// This will handle the remaining root-level varrefs
		// transform vars prefixed by L_ into their language variable pendant if nothing is set within the tpldata array
		if (strpos($text_blocks, '{L_') !== false)
		{
			$text_blocks = preg_replace('#\{L_([a-z0-9\-_]*)\}#is', "<?php echo ((isset(\$this->_rootref['L_\\1'])) ? \$this->_rootref['L_\\1'] : ((isset(\$user->lang['\\1'])) ? \$user->lang['\\1'] : '{ \\1 }')); ?>", $text_blocks);
		}

		// Handle addslashed language variables prefixed with LA_
		// If a template variable already exist, it will be used in favor of it...
		if (strpos($text_blocks, '{LA_') !== false)
		{
			$text_blocks = preg_replace('#\{LA_([a-z0-9\-_]*)\}#is', "<?php echo ((isset(\$this->_rootref['LA_\\1'])) ? \$this->_rootref['LA_\\1'] : ((isset(\$this->_rootref['L_\\1'])) ? addslashes(\$this->_rootref['L_\\1']) : ((isset(\$user->lang['\\1'])) ? addslashes(\$user->lang['\\1']) : '{ \\1 }'))); ?>", $text_blocks);
		}

		// Handle remaining varrefs
		$text_blocks = preg_replace('#\{([a-z0-9\-_]*)\}#is', "<?php echo (isset(\$this->_rootref['\\1'])) ? \$this->_rootref['\\1'] : ''; ?>", $text_blocks);
		$text_blocks = preg_replace('#\{\$([a-z0-9\-_]*)\}#is', "<?php echo (isset(\$this->_tpldata['DEFINE']['.']['\\1'])) ? \$this->_tpldata['DEFINE']['.']['\\1'] : ''; ?>", $text_blocks);

		return;
	}

	/**
	* Compile blocks
	* @access private
	*/
	function compile_tag_block($tag_args)
	{
		$no_nesting = false;

		// Is the designer wanting to call another loop in a loop?
		if (strpos($tag_args, '!') === 0)
		{
			// Count the number if ! occurrences (not allowed in vars)
			$no_nesting = substr_count($tag_args, '!');
			$tag_args = substr($tag_args, $no_nesting);
		}

		// Allow for control of looping (indexes start from zero):
		// foo(2)    : Will start the loop on the 3rd entry
		// foo(-2)   : Will start the loop two entries from the end
		// foo(3,4)  : Will start the loop on the fourth entry and end it on the fifth
		// foo(3,-4) : Will start the loop on the fourth entry and end it four from last
		if (preg_match('#^([^()]*)\(([\-\d]+)(?:,([\-\d]+))?\)$#', $tag_args, $match))
		{
			$tag_args = $match[1];

			if ($match[2] < 0)
			{
				$loop_start = '($_' . $tag_args . '_count ' . $match[2] . ' < 0 ? 0 : $_' . $tag_args . '_count ' . $match[2] . ')';
			}
			else
			{
				$loop_start = '($_' . $tag_args . '_count < ' . $match[2] . ' ? $_' . $tag_args . '_count : ' . $match[2] . ')';
			}

			if (strlen($match[3]) < 1 || $match[3] == -1)
			{
				$loop_end = '$_' . $tag_args . '_count';
			}
			else if ($match[3] >= 0)
			{
				$loop_end = '(' . ($match[3] + 1) . ' > $_' . $tag_args . '_count ? $_' . $tag_args . '_count : ' . ($match[3] + 1) . ')';
			}
			else //if ($match[3] < -1)
			{
				$loop_end = '$_' . $tag_args . '_count' . ($match[3] + 1);
			}
		}
		else
		{
			$loop_start = 0;
			$loop_end = '$_' . $tag_args . '_count';
		}

		$tag_template_php = '';
		array_push($this->block_names, $tag_args);

		if ($no_nesting !== false)
		{
			// We need to implode $no_nesting times from the end...
			$block = array_slice($this->block_names, -$no_nesting);
		}
		else
		{
			$block = $this->block_names;
		}

		if (sizeof($block) < 2)
		{
			// Block is not nested.
			$tag_template_php = '$_' . $tag_args . "_count = (isset(\$this->_tpldata['$tag_args'])) ? sizeof(\$this->_tpldata['$tag_args']) : 0;";
			$varref = "\$this->_tpldata['$tag_args']";
		}
		else
		{
			// This block is nested.
			// Generate a namespace string for this block.
			$namespace = implode('.', $block);

			// Get a reference to the data array for this block that depends on the
			// current indices of all parent blocks.
			$varref = $this->generate_block_data_ref($namespace, false);

			// Create the for loop code to iterate over this block.
			$tag_template_php = '$_' . $tag_args . '_count = (isset(' . $varref . ')) ? sizeof(' . $varref . ') : 0;';
		}

		$tag_template_php .= 'if ($_' . $tag_args . '_count) {';

		/**
		* The following uses foreach for iteration instead of a for loop, foreach is faster but requires PHP to make a copy of the contents of the array which uses more memory
		* <code>
		*	if (!$offset)
		*	{
		*		$tag_template_php .= 'foreach (' . $varref . ' as $_' . $tag_args . '_i => $_' . $tag_args . '_val){';
		*	}
		* </code>
		*/

		$tag_template_php .= 'for ($_' . $tag_args . '_i = ' . $loop_start . '; $_' . $tag_args . '_i < ' . $loop_end . '; ++$_' . $tag_args . '_i){';
		$tag_template_php .= '$_'. $tag_args . '_val = &' . $varref . '[$_'. $tag_args. '_i];';

		return $tag_template_php;
	}

	/**
	* Compile IF tags - much of this is from Smarty with
	* some adaptions for our block level methods
	* @access private
	*/
	function compile_tag_if($tag_args, $elseif)
	{
		// Tokenize args for 'if' tag.
		preg_match_all('/(?:
			"[^"\\\\]*(?:\\\\.[^"\\\\]*)*"         |
			\'[^\'\\\\]*(?:\\\\.[^\'\\\\]*)*\'     |
			[(),]                                  |
			[^\s(),]+)/x', $tag_args, $match);

		$tokens = $match[0];
		$is_arg_stack = array();

		for ($i = 0, $size = sizeof($tokens); $i < $size; $i++)
		{
			$token = &$tokens[$i];

			switch ($token)
			{
				case '!==':
				case '===':
				case '<<':
				case '>>':
				case '|':
				case '^':
				case '&':
				case '~':
				case ')':
				case ',':
				case '+':
				case '-':
				case '*':
				case '/':
				case '@':
				break;

				case '==':
				case 'eq':
					$token = '==';
				break;

				case '!=':
				case '<>':
				case 'ne':
				case 'neq':
					$token = '!=';
				break;

				case '<':
				case 'lt':
					$token = '<';
				break;

				case '<=':
				case 'le':
				case 'lte':
					$token = '<=';
				break;

				case '>':
				case 'gt':
					$token = '>';
				break;

				case '>=':
				case 'ge':
				case 'gte':
					$token = '>=';
				break;

				case '&&':
				case 'and':
					$token = '&&';
				break;

				case '||':
				case 'or':
					$token = '||';
				break;

				case '!':
				case 'not':
					$token = '!';
				break;

				case '%':
				case 'mod':
					$token = '%';
				break;

				case '(':
					array_push($is_arg_stack, $i);
				break;

				case 'is':
					$is_arg_start = ($tokens[$i-1] == ')') ? array_pop($is_arg_stack) : $i-1;
					$is_arg	= implode('	', array_slice($tokens,	$is_arg_start, $i -	$is_arg_start));

					$new_tokens	= $this->_parse_is_expr($is_arg, array_slice($tokens, $i+1));

					array_splice($tokens, $is_arg_start, sizeof($tokens), $new_tokens);

					$i = $is_arg_start;

				// no break

				default:
					if (preg_match('#^((?:[a-z0-9\-_]+\.)+)?(\$)?(?=[A-Z])([A-Z0-9\-_]+)#s', $token, $varrefs))
					{
						$token = (!empty($varrefs[1])) ? $this->generate_block_data_ref(substr($varrefs[1], 0, -1), true, $varrefs[2]) . '[\'' . $varrefs[3] . '\']' : (($varrefs[2]) ? '$this->_tpldata[\'DEFINE\'][\'.\'][\'' . $varrefs[3] . '\']' : '$this->_rootref[\'' . $varrefs[3] . '\']');
					}
					else if (preg_match('#^\.((?:[a-z0-9\-_]+\.?)+)$#s', $token, $varrefs))
					{
						// Allow checking if loops are set with .loopname
						// It is also possible to check the loop count by doing <!-- IF .loopname > 1 --> for example
						$blocks = explode('.', $varrefs[1]);

						// If the block is nested, we have a reference that we can grab.
						// If the block is not nested, we just go and grab the block from _tpldata
						if (sizeof($blocks) > 1)
						{
							$block = array_pop($blocks);
							$namespace = implode('.', $blocks);
							$varref = $this->generate_block_data_ref($namespace, true);

							// Add the block reference for the last child.
							$varref .= "['" . $block . "']";
						}
						else
						{
							$varref = '$this->_tpldata';

							// Add the block reference for the last child.
							$varref .= "['" . $blocks[0] . "']";
						}
						$token = "sizeof($varref)";
					}

				break;
			}
		}

		return (($elseif) ? '} else if (' : 'if (') . (implode(' ', $tokens) . ') { ');
	}

	/**
	* Compile DEFINE tags
	* @access private
	*/
	function compile_tag_define($tag_args, $op)
	{
		preg_match('#^((?:[a-z0-9\-_]+\.)+)?\$(?=[A-Z])([A-Z0-9_\-]*)(?: = (\'?)([^\']*)(\'?))?$#', $tag_args, $match);

		if (empty($match[2]) || (!isset($match[4]) && $op))
		{
			return '';
		}

		if (!$op)
		{
			return 'unset(' . (($match[1]) ? $this->generate_block_data_ref(substr($match[1], 0, -1), true, true) . '[\'' . $match[2] . '\']' : '$this->_tpldata[\'DEFINE\'][\'.\'][\'' . $match[2] . '\']') . ');';
		}

		// Are we a string?
		if ($match[3] && $match[5])
		{
			$match[4] = str_replace(array('\\\'', '\\\\', '\''), array('\'', '\\', '\\\''), $match[4]);

			// Compile reference, we allow template variables in defines...
			$match[4] = $this->compile($match[4]);

			// Now replace the php code
			$match[4] = "'" . str_replace(array('<?php echo ', '; ?>'), array("' . ", " . '"), $match[4]) . "'";
		}
		else
		{
			preg_match('#true|false|\.#i', $match[4], $type);

			switch (strtolower($type[0]))
			{
				case 'true':
				case 'false':
					$match[4] = strtoupper($match[4]);
				break;

				case '.':
					$match[4] = doubleval($match[4]);
				break;

				default:
					$match[4] = intval($match[4]);
				break;
			}
		}

		return (($match[1]) ? $this->generate_block_data_ref(substr($match[1], 0, -1), true, true) . '[\'' . $match[2] . '\']' : '$this->_tpldata[\'DEFINE\'][\'.\'][\'' . $match[2] . '\']') . ' = ' . $match[4] . ';';
	}

	/**
	* Compile INCLUDE tag
	* @access private
	*/
	function compile_tag_include($tag_args)
	{
		return "\$this->_tpl_include('$tag_args');";
	}

	/**
	* Compile INCLUDE_PHP tag
	* @access private
	*/
	function compile_tag_include_php($tag_args)
	{
		return "include('" . $tag_args . "');";
	}

	/**
	* parse expression
	* This is from Smarty
	* @access private
	*/
	function _parse_is_expr($is_arg, $tokens)
	{
		$expr_end = 0;
		$negate_expr = false;

		if (($first_token = array_shift($tokens)) == 'not')
		{
			$negate_expr = true;
			$expr_type = array_shift($tokens);
		}
		else
		{
			$expr_type = $first_token;
		}

		switch ($expr_type)
		{
			case 'even':
				if (@$tokens[$expr_end] == 'by')
				{
					$expr_end++;
					$expr_arg = $tokens[$expr_end++];
					$expr = "!(($is_arg / $expr_arg) % $expr_arg)";
				}
				else
				{
					$expr = "!($is_arg & 1)";
				}
			break;

			case 'odd':
				if (@$tokens[$expr_end] == 'by')
				{
					$expr_end++;
					$expr_arg = $tokens[$expr_end++];
					$expr = "(($is_arg / $expr_arg) % $expr_arg)";
				}
				else
				{
					$expr = "($is_arg & 1)";
				}
			break;

			case 'div':
				if (@$tokens[$expr_end] == 'by')
				{
					$expr_end++;
					$expr_arg = $tokens[$expr_end++];
					$expr = "!($is_arg % $expr_arg)";
				}
			break;
		}

		if ($negate_expr)
		{
			$expr = "!($expr)";
		}

		array_splice($tokens, 0, $expr_end, $expr);

		return $tokens;
	}

	/**
	* Generates a reference to the given variable inside the given (possibly nested)
	* block namespace. This is a string of the form:
	* ' . $this->_tpldata['parent'][$_parent_i]['$child1'][$_child1_i]['$child2'][$_child2_i]...['varname'] . '
	* It's ready to be inserted into an "echo" line in one of the templates.
	* NOTE: expects a trailing "." on the namespace.
	* @access private
	*/
	function generate_block_varref($namespace, $varname, $echo = true, $defop = false)
	{
		// Strip the trailing period.
		$namespace = substr($namespace, 0, -1);

		// Get a reference to the data block for this namespace.
		$varref = $this->generate_block_data_ref($namespace, true, $defop);
		// Prepend the necessary code to stick this in an echo line.

		// Append the variable reference.
		$varref .= "['$varname']";
		$varref = ($echo) ? "<?php echo $varref; ?>" : ((isset($varref)) ? $varref : '');

		return $varref;
	}

	/**
	* Generates a reference to the array of data values for the given
	* (possibly nested) block namespace. This is a string of the form:
	* $this->_tpldata['parent'][$_parent_i]['$child1'][$_child1_i]['$child2'][$_child2_i]...['$childN']
	*
	* If $include_last_iterator is true, then [$_childN_i] will be appended to the form shown above.
	* NOTE: does not expect a trailing "." on the blockname.
	* @access private
	*/
	function generate_block_data_ref($blockname, $include_last_iterator, $defop = false)
	{
		// Get an array of the blocks involved.
		$blocks = explode('.', $blockname);
		$blockcount = sizeof($blocks) - 1;

		// DEFINE is not an element of any referenced variable, we must use _tpldata to access it
		if ($defop)
		{
			$varref = '$this->_tpldata[\'DEFINE\']';
			// Build up the string with everything but the last child.
			for ($i = 0; $i < $blockcount; $i++)
			{
				$varref .= "['" . $blocks[$i] . "'][\$_" . $blocks[$i] . '_i]';
			}
			// Add the block reference for the last child.
			$varref .= "['" . $blocks[$blockcount] . "']";
			// Add the iterator for the last child if requried.
			if ($include_last_iterator)
			{
				$varref .= '[$_' . $blocks[$blockcount] . '_i]';
			}
			return $varref;
		}
		else if ($include_last_iterator)
		{
			return '$_'. $blocks[$blockcount] . '_val';
		}
		else
		{
			return '$_'. $blocks[$blockcount - 1] . '_val[\''. $blocks[$blockcount]. '\']';
		}
	}

	/**
	* Write compiled file to cache directory
	* @access private
	*/
	function compile_write($handle, $data)
	{
		global $phpEx;

		$filename = $this->template->cachepath . str_replace('/', '.', $this->template->filename[$handle]) . '.' . $phpEx;

		if ($fp = @fopen($filename, 'wb'))
		{
			@flock($fp, LOCK_EX);
			@fwrite ($fp, $data);
			@flock($fp, LOCK_UN);
			@fclose($fp);

			@chmod($filename, 0666);
		}

		return;
	}
}

?>