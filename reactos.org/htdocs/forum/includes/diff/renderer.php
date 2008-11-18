<?php
/**
*
* @package diff
* @version $Id: renderer.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2006 phpBB Group
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
* Code from pear.php.net, Text_Diff-0.2.1 (beta) package
* http://pear.php.net/package/Text_Diff/
*
* Modified by phpBB Group to meet our coding standards
* and being able to integrate into phpBB
*
* A class to render Diffs in different formats.
*
* This class renders the diff in classic diff format. It is intended that
* this class be customized via inheritance, to obtain fancier outputs.
*
* @package diff
*/
class diff_renderer
{
	/**
	* Number of leading context "lines" to preserve.
	*
	* This should be left at zero for this class, but subclasses may want to
	* set this to other values.
	*/
	var $_leading_context_lines = 0;

	/**
	* Number of trailing context "lines" to preserve.
	*
	* This should be left at zero for this class, but subclasses may want to
	* set this to other values.
	*/
	var $_trailing_context_lines = 0;

	/**
	* Constructor.
	*/
	function diff_renderer($params = array())
	{
		foreach ($params as $param => $value)
		{
			$v = '_' . $param;
			if (isset($this->$v))
			{
				$this->$v = $value;
			}
		}
	}

	/**
	* Get any renderer parameters.
	*
	* @return array  All parameters of this renderer object.
	*/
	function get_params()
	{
		$params = array();
		foreach (get_object_vars($this) as $k => $v)
		{
			if ($k[0] == '_')
			{
				$params[substr($k, 1)] = $v;
			}
		}

		return $params;
	}

	/**
	* Renders a diff.
	*
	* @param diff &$diff A diff object.
	*
	* @return string  The formatted output.
	*/
	function render(&$diff)
	{
		$xi = $yi = 1;
		$block = false;
		$context = array();

		// Create a new diff object if it is a 3-way diff
		if (is_a($diff, 'diff3'))
		{
			$diff3 = &$diff;

			$diff_1 = $diff3->get_original();
			$diff_2 = $diff3->merged_output();

			unset($diff3);

			$diff = &new diff($diff_1, $diff_2);
		}

		$nlead = $this->_leading_context_lines;
		$ntrail = $this->_trailing_context_lines;

		$output = $this->_start_diff();
		$diffs = $diff->get_diff();

		foreach ($diffs as $i => $edit)
		{
			if (is_a($edit, 'diff_op_copy'))
			{
				if (is_array($block))
				{
					$keep = ($i == sizeof($diffs) - 1) ? $ntrail : $nlead + $ntrail;
					if (sizeof($edit->orig) <= $keep)
					{
						$block[] = $edit;
					}
					else
					{
						if ($ntrail)
						{
							$context = array_slice($edit->orig, 0, $ntrail);
							$block[] = &new diff_op_copy($context);
						}

						$output .= $this->_block($x0, $ntrail + $xi - $x0, $y0, $ntrail + $yi - $y0, $block);
						$block = false;
					}
				}
				$context = $edit->orig;
			}
			else
			{
				if (!is_array($block))
				{
					$context = array_slice($context, sizeof($context) - $nlead);
					$x0 = $xi - sizeof($context);
					$y0 = $yi - sizeof($context);
					$block = array();

					if ($context)
					{
						$block[] = &new diff_op_copy($context);
					}
				}
				$block[] = $edit;
			}

			$xi += ($edit->orig) ? sizeof($edit->orig) : 0;
			$yi += ($edit->final) ? sizeof($edit->final) : 0;
		}

		if (is_array($block))
		{
			$output .= $this->_block($x0, $xi - $x0, $y0, $yi - $y0, $block);
		}

		return $output . $this->_end_diff();
	}

	function _block($xbeg, $xlen, $ybeg, $ylen, &$edits)
	{
		$output = $this->_start_block($this->_block_header($xbeg, $xlen, $ybeg, $ylen));

		foreach ($edits as $edit)
		{
			switch (get_class($edit))
			{
				case 'diff_op_copy':
					$output .= $this->_context($edit->orig);
				break;

				case 'diff_op_add':
					$output .= $this->_added($edit->final);
				break;

				case 'diff_op_delete':
					$output .= $this->_deleted($edit->orig);
				break;

				case 'diff_op_change':
					$output .= $this->_changed($edit->orig, $edit->final);
				break;
			}
		}

		return $output . $this->_end_block();
	}

	function _start_diff()
	{
		return '';
	}

	function _end_diff()
	{
		return '';
	}

	function _block_header($xbeg, $xlen, $ybeg, $ylen)
	{
		if ($xlen > 1)
		{
			$xbeg .= ',' . ($xbeg + $xlen - 1);
		}

		if ($ylen > 1)
		{
			$ybeg .= ',' . ($ybeg + $ylen - 1);
		}

		return $xbeg . ($xlen ? ($ylen ? 'c' : 'd') : 'a') . $ybeg;
	}

	function _start_block($header)
	{
		return $header . "\n";
	}

	function _end_block()
	{
		return '';
	}

	function _lines($lines, $prefix = ' ')
	{
		return $prefix . implode("\n$prefix", $lines) . "\n";
	}

	function _context($lines)
	{
		return $this->_lines($lines, '  ');
	}

	function _added($lines)
	{
		return $this->_lines($lines, '> ');
	}

	function _deleted($lines)
	{
		return $this->_lines($lines, '< ');
	}

	function _changed($orig, $final)
	{
		return $this->_deleted($orig) . "---\n" . $this->_added($final);
	}

	/**
	* Our function to get the diff
	*/
	function get_diff_content($diff)
	{
		return $this->render($diff);
	}
}

/**
* Renders a unified diff
* @package diff
*/
class diff_renderer_unified extends diff_renderer
{
	var $_leading_context_lines = 4;
	var $_trailing_context_lines = 4;

	/**
	* Our function to get the diff
	*/
	function get_diff_content($diff)
	{
		return nl2br($this->render($diff));
	}

	function _block_header($xbeg, $xlen, $ybeg, $ylen)
	{
		if ($xlen != 1)
		{
			$xbeg .= ',' . $xlen;
		}

		if ($ylen != 1)
		{
			$ybeg .= ',' . $ylen;
		}
		return '<div class="diff"><big class="info">@@ -' . $xbeg . ' +' . $ybeg . ' @@</big></div>';
	}

	function _context($lines)
	{
		return '<pre class="diff context">' . htmlspecialchars($this->_lines($lines, ' ')) . '<br /></pre>';
	}

	function _added($lines)
	{
		return '<pre class="diff added">' . htmlspecialchars($this->_lines($lines, '+')) . '<br /></pre>';
	}

	function _deleted($lines)
	{
		return '<pre class="diff removed">' . htmlspecialchars($this->_lines($lines, '-')) . '<br /></pre>';
	}

	function _changed($orig, $final)
	{
		return $this->_deleted($orig) . $this->_added($final);
	}

	function _start_diff()
	{
		$start = '<div class="file">';

		return $start;
	}

	function _end_diff()
	{
		return '</div>';
	}

	function _end_block()
	{
		return '';
	}
}

/**
* "Inline" diff renderer.
*
* This class renders diffs in the Wiki-style "inline" format.
*
* @author  Ciprian Popovici
* @package diff
*/
class diff_renderer_inline extends diff_renderer
{
	var $_leading_context_lines = 10000;
	var $_trailing_context_lines = 10000;

	// Prefix and suffix for inserted text
	var $_ins_prefix = '<span class="ins">';
	var $_ins_suffix = '</span>';

	// Prefix and suffix for deleted text
	var $_del_prefix = '<span class="del">';
	var $_del_suffix = '</span>';

	var $_block_head = '';

	// What are we currently splitting on? Used to recurse to show word-level
	var $_split_level = 'lines';

	/**
	* Our function to get the diff
	*/
	function get_diff_content($diff)
	{
		return '<pre>' . nl2br($this->render($diff)) . '<br /></pre>';
	}

	function _start_diff()
	{
		return '';
	}

	function _end_diff()
	{
		return '';
	}

	function _block_header($xbeg, $xlen, $ybeg, $ylen)
	{
		return $this->_block_head;
	}

	function _start_block($header)
	{
		return $header;
	}

	function _lines($lines, $prefix = ' ', $encode = true)
	{
		if ($encode)
		{
			array_walk($lines, array(&$this, '_encode'));
		}

		if ($this->_split_level == 'words')
		{
			return implode('', $lines);
		}
		else
		{
			return implode("\n", $lines) . "\n";
		}
	}

	function _added($lines)
	{
		array_walk($lines, array(&$this, '_encode'));
		$lines[0] = $this->_ins_prefix . $lines[0];
		$lines[sizeof($lines) - 1] .= $this->_ins_suffix;
		return $this->_lines($lines, ' ', false);
	}

	function _deleted($lines, $words = false)
	{
		array_walk($lines, array(&$this, '_encode'));
		$lines[0] = $this->_del_prefix . $lines[0];
		$lines[sizeof($lines) - 1] .= $this->_del_suffix;
		return $this->_lines($lines, ' ', false);
	}

	function _changed($orig, $final)
	{
		// If we've already split on words, don't try to do so again - just display.
		if ($this->_split_level == 'words')
		{
			$prefix = '';
			while ($orig[0] !== false && $final[0] !== false && substr($orig[0], 0, 1) == ' ' && substr($final[0], 0, 1) == ' ')
			{
				$prefix .= substr($orig[0], 0, 1);
				$orig[0] = substr($orig[0], 1);
				$final[0] = substr($final[0], 1);
			}

			return $prefix . $this->_deleted($orig) . $this->_added($final);
		}

		$text1 = implode("\n", $orig);
		$text2 = implode("\n", $final);

		// Non-printing newline marker.
		$nl = "\0";

		// We want to split on word boundaries, but we need to preserve whitespace as well.
		// Therefore we split on words, but include all blocks of whitespace in the wordlist.
		$splitted_text_1 = $this->_split_on_words($text1, $nl);
		$splitted_text_2 = $this->_split_on_words($text2, $nl);

		$diff = &new diff($splitted_text_1, $splitted_text_2);
		unset($splitted_text_1, $splitted_text_2);

		// Get the diff in inline format.
		$renderer = &new diff_renderer_inline(array_merge($this->get_params(), array('split_level' => 'words')));

		// Run the diff and get the output.
		return str_replace($nl, "\n", $renderer->render($diff)) . "\n";
	}

	function _split_on_words($string, $newline_escape = "\n")
	{
		// Ignore \0; otherwise the while loop will never finish.
		$string = str_replace("\0", '', $string);

		$words = array();
		$length = strlen($string);
		$pos = 0;

		$tab_there = true;
		while ($pos < $length)
		{
			// Check for tabs... do not include them
			if ($tab_there && substr($string, $pos, 1) === "\t")
			{
				$words[] = "\t";
				$pos++;

				continue;
			}
			else
			{
				$tab_there = false;
			}

			// Eat a word with any preceding whitespace.
			$spaces = strspn(substr($string, $pos), " \n");
			$nextpos = strcspn(substr($string, $pos + $spaces), " \n");
			$words[] = str_replace("\n", $newline_escape, substr($string, $pos, $spaces + $nextpos));
			$pos += $spaces + $nextpos;
		}

		return $words;
	}

	function _encode(&$string)
	{
		$string = htmlspecialchars($string);
	}
}

/**
* "raw" diff renderer.
* This class could be used to output a raw unified patch file
*
* @package diff
*/
class diff_renderer_raw extends diff_renderer
{
	var $_leading_context_lines = 4;
	var $_trailing_context_lines = 4;

	/**
	* Our function to get the diff
	*/
	function get_diff_content($diff)
	{
		return '<textarea style="height: 290px;" class="full">' . htmlspecialchars($this->render($diff)) . '</textarea>';
	}

	function _block_header($xbeg, $xlen, $ybeg, $ylen)
	{
		if ($xlen != 1)
		{
			$xbeg .= ',' . $xlen;
		}

		if ($ylen != 1)
		{
			$ybeg .= ',' . $ylen;
		}
		return '@@ -' . $xbeg . ' +' . $ybeg . ' @@';
	}

	function _context($lines)
	{
		return $this->_lines($lines, ' ');
	}

	function _added($lines)
	{
		return $this->_lines($lines, '+');
	}

	function _deleted($lines)
	{
		return $this->_lines($lines, '-');
	}

	function _changed($orig, $final)
	{
		return $this->_deleted($orig) . $this->_added($final);
	}
}

/**
* "chora (Horde)" diff renderer - similar style.
* This renderer class is a modified human_readable function from the Horde Framework.
*
* @package diff
*/
class diff_renderer_side_by_side extends diff_renderer
{
	var $_leading_context_lines = 3;
	var $_trailing_context_lines = 3;

	var $lines = array();

	// Hold the left and right columns of lines for change blocks.
	var $cols;
	var $state;

	var $data = false;

	/**
	* Our function to get the diff
	*/
	function get_diff_content($diff)
	{
		global $user;

		$output = '';
		$output .= '<table cellspacing="0" class="hrdiff">
<caption>
	<span class="unmodified">&nbsp;</span> ' . $user->lang['LINE_UNMODIFIED'] . '
	<span class="added">&nbsp;</span> ' . $user->lang['LINE_ADDED'] . '
	<span class="modified">&nbsp;</span> ' . $user->lang['LINE_MODIFIED'] . '
	<span class="removed">&nbsp;</span> ' . $user->lang['LINE_REMOVED'] . '
</caption>
<tbody>
';

		$this->render($diff);

		// Is the diff empty?
		if (!sizeof($this->lines))
		{
			$output .= '<tr><th colspan="2">' . $user->lang['NO_VISIBLE_CHANGES'] . '</th></tr>';
		}
		else
		{
			// Iterate through every header block of changes
			foreach ($this->lines as $header)
			{
				$output .= '<tr><th>' . $user->lang['LINE'] . ' ' . $header['oldline'] . '</th><th>' . $user->lang['LINE'] . ' ' . $header['newline'] . '</th></tr>';

				// Each header block consists of a number of changes (add, remove, change).
				$current_context = '';

				foreach ($header['contents'] as $change)
				{
					if (!empty($current_context) && $change['type'] != 'empty')
					{
						$line = $current_context;
						$current_context = '';

						$output .= '<tr class="unmodified"><td><pre>' . ((strlen($line)) ? $line : '&nbsp;') . '<br /></pre></td>
							<td><pre>' . ((strlen($line)) ? $line : '&nbsp;') . '<br /></pre></td></tr>';
					}

					switch ($change['type'])
					{
						case 'add':
							$line = '';

							foreach ($change['lines'] as $_line)
							{
								$line .= htmlspecialchars($_line) . '<br />';
							}

							$output .= '<tr><td class="added_empty">&nbsp;</td><td class="added"><pre>' . ((strlen($line)) ? $line : '&nbsp;') . '<br /></pre></td></tr>';
						break;

						case 'remove':
							$line = '';

							foreach ($change['lines'] as $_line)
							{
								$line .= htmlspecialchars($_line) . '<br />';
							}

							$output .= '<tr><td class="removed"><pre>' . ((strlen($line)) ? $line : '&nbsp;') . '<br /></pre></td><td class="removed_empty">&nbsp;</td></tr>';
						break;

						case 'empty':
							$current_context .= htmlspecialchars($change['line']) . '<br />';
						break;

						case 'change':
							// Pop the old/new stacks one by one, until both are empty.
							$oldsize = sizeof($change['old']);
							$newsize = sizeof($change['new']);
							$left = $right = '';

							for ($row = 0, $row_max = max($oldsize, $newsize); $row < $row_max; ++$row)
							{
								$left .= isset($change['old'][$row]) ? htmlspecialchars($change['old'][$row]) : '';
								$left .= '<br />';
								$right .= isset($change['new'][$row]) ? htmlspecialchars($change['new'][$row]) : '';
								$right .= '<br />';
							}

							$output .= '<tr>';

							if (!empty($left))
							{
								$output .= '<td class="modified"><pre>' . $left . '<br /></pre></td>';
							}
							else if ($row < $oldsize)
							{
								$output .= '<td class="modified">&nbsp;</td>';
							}
							else
							{
								$output .= '<td class="unmodified">&nbsp;</td>';
							}

							if (!empty($right))
							{
								$output .= '<td class="modified"><pre>' . $right . '<br /></pre></td>';
							}
							else if ($row < $newsize)
							{
								$output .= '<td class="modified">&nbsp;</td>';
							}
							else
							{
								$output .= '<td class="unmodified">&nbsp;</td>';
							}

							$output .= '</tr>';
						break;
					}
				}

				if (!empty($current_context))
				{
					$line = $current_context;
					$current_context = '';

					$output .= '<tr class="unmodified"><td><pre>' . ((strlen($line)) ? $line : '&nbsp;') . '<br /></pre></td>';
					$output .= '<td><pre>' . ((strlen($line)) ? $line : '&nbsp;') . '<br /></pre></td></tr>';
				}
			}
		}

		$output .= '</tbody></table>';

		return $output;
	}

	function _start_diff()
	{
		$this->lines = array();

		$this->data = false;
		$this->cols = array(array(), array());
		$this->state = 'empty';

		return '';
	}

	function _end_diff()
	{
		// Just flush any remaining entries in the columns stack.
		switch ($this->state)
		{
			case 'add':
				$this->data['contents'][] = array('type' => 'add', 'lines' => $this->cols[0]);
			break;

			case 'remove':
				// We have some removal lines pending in our stack, so flush them.
				$this->data['contents'][] = array('type' => 'remove', 'lines' => $this->cols[0]);
			break;

			case 'change':
				// We have both remove and addition lines, so this is a change block.
				$this->data['contents'][] = array('type' => 'change', 'old' => $this->cols[0], 'new' => $this->cols[1]);
			break;
		}

		if ($this->data !== false)
		{
			$this->lines[] = $this->data;
		}

		return '';
	}

	function _block_header($xbeg, $xlen, $ybeg, $ylen)
	{
		// Push any previous header information to the return stack.
		if ($this->data !== false)
		{
			$this->lines[] = $this->data;
		}

		$this->data = array('type' => 'header', 'oldline' => $xbeg, 'newline' => $ybeg, 'contents' => array());
		$this->state = 'dump';
	}

	function _added($lines)
	{
		array_walk($lines, array(&$this, '_perform_add'));
	}

	function _perform_add($line)
	{
		if ($this->state == 'empty')
		{
			return '';
		}

		// This is just an addition line.
		if ($this->state == 'dump' || $this->state == 'add')
		{
			// Start adding to the addition stack.
			$this->cols[0][] = $line;
			$this->state = 'add';
		}
		else
		{
			// This is inside a change block, so start accumulating lines.
			$this->state = 'change';
			$this->cols[1][] = $line;
		}
	}

	function _deleted($lines)
	{
		array_walk($lines, array(&$this, '_perform_delete'));
	}

	function _perform_delete($line)
	{
		// This is a removal line.
		$this->state = 'remove';
		$this->cols[0][] = $line;
	}

	function _context($lines)
	{
		array_walk($lines, array(&$this, '_perform_context'));
	}

	function _perform_context($line)
	{
		// An empty block with no action.
		switch ($this->state)
		{
			case 'add':
				$this->data['contents'][] = array('type' => 'add', 'lines' => $this->cols[0]);
			break;

			case 'remove':
				// We have some removal lines pending in our stack, so flush them.
				$this->data['contents'][] = array('type' => 'remove', 'lines' => $this->cols[0]);
			break;

			case 'change':
				// We have both remove and addition lines, so this is a change block.
				$this->data['contents'][] = array('type' => 'change', 'old' => $this->cols[0], 'new' => $this->cols[1]);
			break;
		}

		$this->cols = array(array(), array());
		$this->data['contents'][] = array('type' => 'empty', 'line' => $line);
		$this->state = 'dump';
	}

	function _changed($orig, $final)
	{
		return $this->_deleted($orig) . $this->_added($final);
	}

}

?>