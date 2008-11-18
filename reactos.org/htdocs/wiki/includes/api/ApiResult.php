<?php

/*
 * Created on Sep 4, 2006
 *
 * API for MediaWiki 1.8+
 *
 * Copyright (C) 2006 Yuri Astrakhan <Firstname><Lastname>@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * http://www.gnu.org/copyleft/gpl.html
 */

if (!defined('MEDIAWIKI')) {
	// Eclipse helper - will be ignored in production
	require_once ('ApiBase.php');
}

/**
 * This class represents the result of the API operations.
 * It simply wraps a nested array() structure, adding some functions to simplify array's modifications.
 * As various modules execute, they add different pieces of information to this result,
 * structuring it as it will be given to the client.
 *
 * Each subarray may either be a dictionary - key-value pairs with unique keys,
 * or lists, where the items are added using $data[] = $value notation.
 *
 * There are two special key values that change how XML output is generated:
 *   '_element' This key sets the tag name for the rest of the elements in the current array.
 *              It is only inserted if the formatter returned true for getNeedsRawData()
 *   '*'        This key has special meaning only to the XML formatter, and is outputed as is
 * 				for all others. In XML it becomes the content of the current element.
 *
 * @ingroup API
 */
class ApiResult extends ApiBase {

	private $mData, $mIsRawMode;

	/**
	* Constructor
	*/
	public function __construct($main) {
		parent :: __construct($main, 'result');
		$this->mIsRawMode = false;
		$this->reset();
	}

	/**
	 * Clear the current result data.
	 */
	public function reset() {
		$this->mData = array ();
	}

	/**
	 * Call this function when special elements such as '_element'
	 * are needed by the formatter, for example in XML printing.
	 */
	public function setRawMode() {
		$this->mIsRawMode = true;
	}

	/**
	 * Returns true if the result is being created for the formatter that requested raw data.
	 */
	public function getIsRawMode() {
		return $this->mIsRawMode;
	}

	/**
	 * Get result's internal data array
	 */
	public function & getData() {
		return $this->mData;
	}

	/**
	 * Add an output value to the array by name.
	 * Verifies that value with the same name has not been added before.
	 */
	public static function setElement(& $arr, $name, $value) {
		if ($arr === null || $name === null || $value === null || !is_array($arr) || is_array($name))
			ApiBase :: dieDebug(__METHOD__, 'Bad parameter');

		if (!isset ($arr[$name])) {
			$arr[$name] = $value;
		}
		elseif (is_array($arr[$name]) && is_array($value)) {
			$merged = array_intersect_key($arr[$name], $value);
			if (empty ($merged))
				$arr[$name] += $value;
			else
				ApiBase :: dieDebug(__METHOD__, "Attempting to merge element $name");
		} else
			ApiBase :: dieDebug(__METHOD__, "Attempting to add element $name=$value, existing value is {$arr[$name]}");
	}

	/**
	 * Adds the content element to the array.
	 * Use this function instead of hardcoding the '*' element.
	 * @param string $subElemName when present, content element is created as a sub item of the arr.
	 *  Use this parameter to create elements in format <elem>text</elem> without attributes
	 */
	public static function setContent(& $arr, $value, $subElemName = null) {
		if (is_array($value))
			ApiBase :: dieDebug(__METHOD__, 'Bad parameter');
		if (is_null($subElemName)) {
			ApiResult :: setElement($arr, '*', $value);
		} else {
			if (!isset ($arr[$subElemName]))
				$arr[$subElemName] = array ();
			ApiResult :: setElement($arr[$subElemName], '*', $value);
		}
	}

	/**
	 * In case the array contains indexed values (in addition to named),
	 * all indexed values will have the given tag name.
	 */
	public function setIndexedTagName(& $arr, $tag) {
		// In raw mode, add the '_element', otherwise just ignore
		if (!$this->getIsRawMode())
			return;
		if ($arr === null || $tag === null || !is_array($arr) || is_array($tag))
			ApiBase :: dieDebug(__METHOD__, 'Bad parameter');
		// Do not use setElement() as it is ok to call this more than once
		$arr['_element'] = $tag;
	}

	/**
	 * Calls setIndexedTagName() on $arr and each sub-array
	 */
	public function setIndexedTagName_recursive(&$arr, $tag)
	{
			if(!is_array($arr))
					return;
			foreach($arr as &$a)
			{
					if(!is_array($a))
							continue;
					$this->setIndexedTagName($a, $tag);
					$this->setIndexedTagName_recursive($a, $tag);
			}
	}

	/**
	 * Add value to the output data at the given path.
	 * Path is an indexed array, each element specifing the branch at which to add the new value
	 * Setting $path to array('a','b','c') is equivalent to data['a']['b']['c'] = $value
	 * If $name is empty, the $value is added as a next list element data[] = $value
	 */
	public function addValue($path, $name, $value) {

		$data = & $this->getData();

		if (!is_null($path)) {
			if (is_array($path)) {
				foreach ($path as $p) {
					if (!isset ($data[$p]))
						$data[$p] = array ();
					$data = & $data[$p];
				}
			} else {
				if (!isset ($data[$path]))
					$data[$path] = array ();
				$data = & $data[$path];
			}
		}

		if (empty($name))
			$data[] = $value;	// Add list element
		else
			ApiResult :: setElement($data, $name, $value);	// Add named element
	}

	public function execute() {
		ApiBase :: dieDebug(__METHOD__, 'execute() is not supported on Result object');
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: ApiResult.php 35098 2008-05-20 17:13:28Z ialex $';
	}
}

/* For compatibility with PHP versions < 5.1.0, define our own array_intersect_key function. */
if (!function_exists('array_intersect_key')) {
	function array_intersect_key($isec, $keys) {
		$argc = func_num_args();

		if ($argc > 2) {
			for ($i = 1; !empty($isec) && $i < $argc; $i++) {
				$arr = func_get_arg($i);

				foreach (array_keys($isec) as $key) {
					if (!isset($arr[$key]))
						unset($isec[$key]);
				}
			}

			return $isec;
		} else {
			$res = array();
			foreach (array_keys($isec) as $key) {
				if (isset($keys[$key]))
					$res[$key] = $isec[$key];
			}

			return $res;
		}
	}
}
