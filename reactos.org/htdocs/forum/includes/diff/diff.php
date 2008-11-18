<?php
/**
*
* @package diff
* @version $Id: diff.php 8479 2008-03-29 00:22:48Z naderman $
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
* General API for generating and formatting diffs - the differences between
* two sequences of strings.
*
* @package diff
* @author  Geoffrey T. Dairiki <dairiki@dairiki.org>
*/
class diff
{
	/**
	* Array of changes.
	* @var array
	*/
	var $_edits;

	/**
	* Computes diffs between sequences of strings.
	*
	* @param array $from_lines  An array of strings. Typically these are lines from a file.
	* @param array $to_lines    An array of strings.
	*/
	function diff(&$from_content, &$to_content, $preserve_cr = true)
	{
		$diff_engine = &new diff_engine();
		$this->_edits = $diff_engine->diff($from_content, $to_content, $preserve_cr);
	}

	/**
	* Returns the array of differences.
	*/
	function get_diff()
	{
		return $this->_edits;
	}

	/**
	* Computes a reversed diff.
	*
	* Example:
	* <code>
	* $diff = &new diff($lines1, $lines2);
	* $rev = $diff->reverse();
	* </code>
	*
	* @return diff  A Diff object representing the inverse of the original diff.
	*               Note that we purposely don't return a reference here, since
	*               this essentially is a clone() method.
	*/
	function reverse()
	{
		if (version_compare(zend_version(), '2', '>'))
		{
			$rev = clone($this);
		}
		else
		{
			$rev = $this;
		}

		$rev->_edits = array();

		foreach ($this->_edits as $edit)
		{
			$rev->_edits[] = $edit->reverse();
		}

		return $rev;
	}

	/**
	* Checks for an empty diff.
	*
	* @return boolean  True if two sequences were identical.
	*/
	function is_empty()
	{
		foreach ($this->_edits as $edit)
		{
			if (!is_a($edit, 'diff_op_copy'))
			{
				return false;
			}
		}
		return true;
	}

	/**
	* Computes the length of the Longest Common Subsequence (LCS).
	*
	* This is mostly for diagnostic purposes.
	*
	* @return integer  The length of the LCS.
	*/
	function lcs()
	{
		$lcs = 0;

		foreach ($this->_edits as $edit)
		{
			if (is_a($edit, 'diff_op_copy'))
			{
				$lcs += sizeof($edit->orig);
			}
		}
		return $lcs;
	}

	/**
	* Gets the original set of lines.
	*
	* This reconstructs the $from_lines parameter passed to the constructor.
	*
	* @return array  The original sequence of strings.
	*/
	function get_original()
	{
		$lines = array();

		foreach ($this->_edits as $edit)
		{
			if ($edit->orig)
			{
				array_splice($lines, sizeof($lines), 0, $edit->orig);
			}
		}
		return $lines;
	}

	/**
	* Gets the final set of lines.
	*
	* This reconstructs the $to_lines parameter passed to the constructor.
	*
	* @return array  The sequence of strings.
	*/
	function get_final()
	{
		$lines = array();

		foreach ($this->_edits as $edit)
		{
			if ($edit->final)
			{
				array_splice($lines, sizeof($lines), 0, $edit->final);
			}
		}
		return $lines;
	}

	/**
	* Removes trailing newlines from a line of text. This is meant to be used with array_walk().
	*
	* @param string &$line  The line to trim.
	* @param integer $key  The index of the line in the array. Not used.
	*/
	function trim_newlines(&$line, $key)
	{
		$line = str_replace(array("\n", "\r"), '', $line);
	}

	/**
	* Checks a diff for validity.
	*
	* This is here only for debugging purposes.
	*/
	function _check($from_lines, $to_lines)
	{
		if (serialize($from_lines) != serialize($this->get_original()))
		{
			trigger_error("[diff] Reconstructed original doesn't match", E_USER_ERROR);
		}

		if (serialize($to_lines) != serialize($this->get_final()))
		{
			trigger_error("[diff] Reconstructed final doesn't match", E_USER_ERROR);
		}

		$rev = $this->reverse();

		if (serialize($to_lines) != serialize($rev->get_original()))
		{
			trigger_error("[diff] Reversed original doesn't match", E_USER_ERROR);
		}

		if (serialize($from_lines) != serialize($rev->get_final()))
		{
			trigger_error("[diff] Reversed final doesn't match", E_USER_ERROR);
		}

		$prevtype = null;

		foreach ($this->_edits as $edit)
		{
			if ($prevtype == get_class($edit))
			{
				trigger_error("[diff] Edit sequence is non-optimal", E_USER_ERROR);
			}
			$prevtype = get_class($edit);
		}

		return true;
	}
}

/**
* @package diff
* @author  Geoffrey T. Dairiki <dairiki@dairiki.org>
*/
class mapped_diff extends diff
{
	/**
	* Computes a diff between sequences of strings.
	*
	* This can be used to compute things like case-insensitve diffs, or diffs
	* which ignore changes in white-space.
	*
	* @param array $from_lines         An array of strings.
	* @param array $to_lines           An array of strings.
	* @param array $mapped_from_lines  This array should have the same size number of elements as $from_lines.
	*                                  The elements in $mapped_from_lines and $mapped_to_lines are what is actually
	*                                  compared when computing the diff.
	* @param array $mapped_to_lines    This array should have the same number of elements as $to_lines.
	*/
	function mapped_diff(&$from_lines, &$to_lines, &$mapped_from_lines, &$mapped_to_lines)
	{
		if (sizeof($from_lines) != sizeof($mapped_from_lines) || sizeof($to_lines) != sizeof($mapped_to_lines))
		{
			return false;
		}

		parent::diff($mapped_from_lines, $mapped_to_lines);

		$xi = $yi = 0;
		for ($i = 0; $i < sizeof($this->_edits); $i++)
		{
			$orig = &$this->_edits[$i]->orig;
			if (is_array($orig))
			{
				$orig = array_slice($from_lines, $xi, sizeof($orig));
				$xi += sizeof($orig);
			}

			$final = &$this->_edits[$i]->final;
			if (is_array($final))
			{
				$final = array_slice($to_lines, $yi, sizeof($final));
				$yi += sizeof($final);
			}
		}
	}
}

/**
* @package diff
* @author  Geoffrey T. Dairiki <dairiki@dairiki.org>
*
* @access private
*/
class diff_op
{
	var $orig;
	var $final;

	function reverse()
	{
		trigger_error('[diff] Abstract method', E_USER_ERROR);
	}

	function norig()
	{
		return ($this->orig) ? sizeof($this->orig) : 0;
	}

	function nfinal()
	{
		return ($this->final) ? sizeof($this->final) : 0;
	}
}

/**
* @package diff
* @author  Geoffrey T. Dairiki <dairiki@dairiki.org>
*
* @access private
*/
class diff_op_copy extends diff_op
{
	function diff_op_copy($orig, $final = false)
	{
		if (!is_array($final))
		{
			$final = $orig;
		}
		$this->orig = $orig;
		$this->final = $final;
	}

	function &reverse()
	{
		$reverse = &new diff_op_copy($this->final, $this->orig);
		return $reverse;
	}
}

/**
* @package diff
* @author  Geoffrey T. Dairiki <dairiki@dairiki.org>
*
* @access private
*/
class diff_op_delete extends diff_op
{
	function diff_op_delete($lines)
	{
		$this->orig = $lines;
		$this->final = false;
	}

	function &reverse()
	{
		$reverse = &new diff_op_add($this->orig);
		return $reverse;
	}
}

/**
* @package diff
* @author  Geoffrey T. Dairiki <dairiki@dairiki.org>
*
* @access private
*/
class diff_op_add extends diff_op
{
	function diff_op_add($lines)
	{
		$this->final = $lines;
		$this->orig = false;
	}

	function &reverse()
	{
		$reverse = &new diff_op_delete($this->final);
		return $reverse;
	}
}

/**
* @package diff
* @author  Geoffrey T. Dairiki <dairiki@dairiki.org>
*
* @access private
*/
class diff_op_change extends diff_op
{
	function diff_op_change($orig, $final)
	{
		$this->orig = $orig;
		$this->final = $final;
	}

	function &reverse()
	{
		$reverse = &new diff_op_change($this->final, $this->orig);
		return $reverse;
	}
}


/**
* A class for computing three way diffs.
*
* @package diff
* @author  Geoffrey T. Dairiki <dairiki@dairiki.org>
*/
class diff3 extends diff
{
	/**
	* Conflict counter.
	* @var integer
	*/
	var $_conflicting_blocks = 0;

	/**
	* Computes diff between 3 sequences of strings.
	*
	* @param array $orig    The original lines to use.
	* @param array $final1  The first version to compare to.
	* @param array $final2  The second version to compare to.
	*/
	function diff3(&$orig, &$final1, &$final2)
	{
		$diff_engine = &new diff_engine();

		$diff_1 = $diff_engine->diff($orig, $final1);
		$diff_2 = $diff_engine->diff($orig, $final2);

		unset($engine);

		$this->_edits = $this->_diff3($diff_1, $diff_2);
	}

	/**
	* Return merged output
	*
	* @param string $label1 the cvs file version/label from the original set of lines
	* @param string $label2 the cvs file version/label from the new set of lines
	* @param string $label_sep the explanation between label1 and label2 - more of a helper for the user
	* @param bool $get_conflicts if set to true only the number of conflicts is returned
	* @param bool $merge_new if set to true the merged output will have the new file contents on a conflicting merge
	*
	* @return mixed the merged output
	*/
	function merged_output($label1 = 'CURRENT_FILE', $label2 = 'NEW_FILE', $label_sep = 'DIFF_SEP_EXPLAIN', $get_conflicts = false, $merge_new = false)
	{
		global $user;

		if ($get_conflicts)
		{
			foreach ($this->_edits as $edit)
			{
				if ($edit->is_conflict())
				{
					$this->_conflicting_blocks++;
				}
			}

			return $this->_conflicting_blocks;
		}

		$label1 = (!empty($user->lang[$label1])) ? $user->lang[$label1] : $label1;
		$label2 = (!empty($user->lang[$label2])) ? $user->lang[$label2] : $label2;
		$label_sep = (!empty($user->lang[$label_sep])) ? $user->lang[$label_sep] : $label_sep;

		$lines = array();

		foreach ($this->_edits as $edit)
		{
			if ($edit->is_conflict())
			{
				if (!$merge_new)
				{
					$lines = array_merge($lines, array('<<<<<<<' . ($label1 ? ' ' . $label1 : '')), $edit->final1, array('=======' . ($label_sep ? ' ' . $label_sep : '')), $edit->final2, array('>>>>>>>' . ($label2 ? ' ' . $label2 : '')));
				}
				else
				{
					$lines = array_merge($lines, $edit->final1);
				}
				$this->_conflicting_blocks++;
			}
			else
			{
				$lines = array_merge($lines, $edit->merged());
			}
		}

		return $lines;
	}

	/**
	* Merge the output and use the new file code for conflicts
	*/
	function merged_new_output()
	{
		$lines = array();

		foreach ($this->_edits as $edit)
		{
			if ($edit->is_conflict())
			{
				$lines = array_merge($lines, $edit->final2);
			}
			else
			{
				$lines = array_merge($lines, $edit->merged());
			}
		}

		return $lines;
	}

	/**
	* Merge the output and use the original file code for conflicts
	*/
	function merged_orig_output()
	{
		$lines = array();

		foreach ($this->_edits as $edit)
		{
			if ($edit->is_conflict())
			{
				$lines = array_merge($lines, $edit->final1);
			}
			else
			{
				$lines = array_merge($lines, $edit->merged());
			}
		}

		return $lines;
	}

	/**
	* Get conflicting block(s)
	*/
	function get_conflicts()
	{
		$conflicts = array();

		foreach ($this->_edits as $edit)
		{
			if ($edit->is_conflict())
			{
				$conflicts[] = array($edit->final1, $edit->final2);
			}
		}

		return $conflicts;
	}

	/**
	* @access private
	*/
	function _diff3(&$edits1, &$edits2)
	{
		$edits = array();
		$bb = &new diff3_block_builder();

		$e1 = current($edits1);
		$e2 = current($edits2);

		while ($e1 || $e2)
		{
			if ($e1 && $e2 && is_a($e1, 'diff_op_copy') && is_a($e2, 'diff_op_copy'))
			{
				// We have copy blocks from both diffs. This is the (only) time we want to emit a diff3 copy block.
				// Flush current diff3 diff block, if any.
				if ($edit = $bb->finish())
				{
					$edits[] = $edit;
				}

				$ncopy = min($e1->norig(), $e2->norig());
				$edits[] = &new diff3_op_copy(array_slice($e1->orig, 0, $ncopy));

				if ($e1->norig() > $ncopy)
				{
					array_splice($e1->orig, 0, $ncopy);
					array_splice($e1->final, 0, $ncopy);
				}
				else
				{
					$e1 = next($edits1);
				}

				if ($e2->norig() > $ncopy)
				{
					array_splice($e2->orig, 0, $ncopy);
					array_splice($e2->final, 0, $ncopy);
				}
				else
				{
					$e2 = next($edits2);
				}
			}
			else
			{
				if ($e1 && $e2)
				{
					if ($e1->orig && $e2->orig)
					{
						$norig = min($e1->norig(), $e2->norig());
						$orig = array_splice($e1->orig, 0, $norig);
						array_splice($e2->orig, 0, $norig);
						$bb->input($orig);
					}
					else
					{
						$norig = 0;
					}

					if (is_a($e1, 'diff_op_copy'))
					{
						$bb->out1(array_splice($e1->final, 0, $norig));
					}

					if (is_a($e2, 'diff_op_copy'))
					{
						$bb->out2(array_splice($e2->final, 0, $norig));
					}
				}

				if ($e1 && ! $e1->orig)
				{
					$bb->out1($e1->final);
					$e1 = next($edits1);
				}

				if ($e2 && ! $e2->orig)
				{
					$bb->out2($e2->final);
					$e2 = next($edits2);
				}
			}
		}

		if ($edit = $bb->finish())
		{
			$edits[] = $edit;
		}

		return $edits;
	}
}

/**
* @package diff
* @author  Geoffrey T. Dairiki <dairiki@dairiki.org>
*
* @access private
*/
class diff3_op
{
	function diff3_op($orig = false, $final1 = false, $final2 = false)
	{
		$this->orig = $orig ? $orig : array();
		$this->final1 = $final1 ? $final1 : array();
		$this->final2 = $final2 ? $final2 : array();
	}

	function merged()
	{
		if (!isset($this->_merged))
		{
			if ($this->final1 === $this->final2)
			{
				$this->_merged = &$this->final1;
			}
			else if ($this->final1 === $this->orig)
			{
				$this->_merged = &$this->final2;
			}
			else if ($this->final2 === $this->orig)
			{
				$this->_merged = &$this->final1;
			}
			else
			{
				$this->_merged = false;
			}
		}

		return $this->_merged;
	}

	function is_conflict()
	{
		return ($this->merged() === false) ? true : false;
	}
}

/**
* @package diff
* @author  Geoffrey T. Dairiki <dairiki@dairiki.org>
*
* @access private
*/
class diff3_op_copy extends diff3_op
{
	function diff3_op_copy($lines = false)
	{
		$this->orig = $lines ? $lines : array();
		$this->final1 = &$this->orig;
		$this->final2 = &$this->orig;
	}

	function merged()
	{
		return $this->orig;
	}

	function is_conflict()
	{
		return false;
	}
}

/**
* @package diff
* @author  Geoffrey T. Dairiki <dairiki@dairiki.org>
*
* @access private
*/
class diff3_block_builder
{
	function diff3_block_builder()
	{
		$this->_init();
	}

	function input($lines)
	{
		if ($lines)
		{
			$this->_append($this->orig, $lines);
		}
	}

	function out1($lines)
	{
		if ($lines)
		{
			$this->_append($this->final1, $lines);
		}
	}

	function out2($lines)
	{
		if ($lines)
		{
			$this->_append($this->final2, $lines);
		}
	}

	function is_empty()
	{
		return !$this->orig && !$this->final1 && !$this->final2;
	}

	function finish()
	{
		if ($this->is_empty())
		{
			return false;
		}
		else
		{
			$edit = &new diff3_op($this->orig, $this->final1, $this->final2);
			$this->_init();
			return $edit;
		}
	}

	function _init()
	{
		$this->orig = $this->final1 = $this->final2 = array();
	}

	function _append(&$array, $lines)
	{
		array_splice($array, sizeof($array), 0, $lines);
	}
}

?>