<?php
/**
*
* @package phpBB3
* @version $Id: index.php 8479 2008-03-29 00:22:48Z naderman $
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
* phpBB Hook Class
* @package phpBB3
*/
class phpbb_hook
{
	/**
	* Registered hooks
	*/
	var $hooks = array();

	/**
	* Results returned by functions called
	*/
	var $hook_result = array();

	/**
	* internal pointer
	*/
	var $current_hook = NULL;

	/**
	* Initialize hook class.
	*
	* @param array $valid_hooks array containing the hookable functions/methods
	*/
	function phpbb_hook($valid_hooks)
	{
		foreach ($valid_hooks as $_null => $method)
		{
			$this->add_hook($method);
		}

		if (function_exists('phpbb_hook_register'))
		{
			phpbb_hook_register($this);
		}
	}

	/**
	* Register function/method to be called within hook
	* This function is normally called by the modification/application to attach/register the functions.
	*
	* @param mixed $definition Declaring function (with __FUNCTION__) or class with array(__CLASS__, __FUNCTION__)
	* @param mixed $hook The replacement function/method to be called. Passing function name or array with object/class definition
	* @param string $mode Specify the priority/chain mode. 'normal' -> hook gets appended to the chain. 'standalone' -> only the specified hook gets called - later hooks are not able to overwrite this (E_NOTICE is triggered then). 'first' -> hook is called as the first one within the chain. 'last' -> hook is called as the last one within the chain.
	*/
	function register($definition, $hook, $mode = 'normal')
	{
		$class = (!is_array($definition)) ? '__global' : $definition[0];
		$function = (!is_array($definition)) ? $definition : $definition[1];

		// Method able to be hooked?
		if (isset($this->hooks[$class][$function]))
		{
			switch ($mode)
			{
				case 'standalone':
					if (!isset($this->hooks[$class][$function]['standalone']))
					{
						$this->hooks[$class][$function] = array('standalone' => $hook);
					}
					else
					{
						trigger_error('Hook not able to be called standalone, previous hook already standalone.', E_NOTICE);
					}
				break;

				case 'first':
				case 'last':
					$this->hooks[$class][$function][$mode][] = $hook;
				break;

				case 'normal':
				default:
					$this->hooks[$class][$function]['normal'][] = $hook;
				break;
			}
		}
	}

	/**
	* Calling all functions/methods attached to a specified hook.
	* Called by the function allowing hooks...
	*
	* @param mixed $definition Declaring function (with __FUNCTION__) or class with array(__CLASS__, __FUNCTION__)
	* @return bool False if no hook got executed, true otherwise
	*/
	function call_hook($definition)
	{
		$class = (!is_array($definition)) ? '__global' : $definition[0];
		$function = (!is_array($definition)) ? $definition : $definition[1];

		if (!empty($this->hooks[$class][$function]))
		{
			// Developer tries to call a hooked function within the hooked function...
			if ($this->current_hook !== NULL && $this->current_hook['class'] === $class && $this->current_hook['function'] === $function)
			{
				return false;
			}

			// Call the hook with the arguments attached and store result
			$arguments = func_get_args();
			$this->current_hook = array('class' => $class, 'function' => $function);
			$arguments[0] = &$this;

			// Call the hook chain...
			if (isset($this->hooks[$class][$function]['standalone']))
			{
				$this->hook_result[$class][$function] = call_user_func_array($this->hooks[$class][$function]['standalone'], $arguments);
			}
			else
			{
				foreach (array('first', 'normal', 'last') as $mode)
				{
					if (!isset($this->hooks[$class][$function][$mode]))
					{
						continue;
					}

					foreach ($this->hooks[$class][$function][$mode] as $hook)
					{
						$this->hook_result[$class][$function] = call_user_func_array($hook, $arguments);
					}
				}
			}

			$this->current_hook = NULL;
			return true;
		}

		$this->current_hook = NULL;
		return false;
	}

	/**
	* Get result from previously called functions/methods for the same hook
	*
	* @param mixed $definition Declaring function (with __FUNCTION__) or class with array(__CLASS__, __FUNCTION__)
	* @return mixed False if nothing returned if there is no result, else array('result' => ... )
	*/
	function previous_hook_result($definition)
	{
		$class = (!is_array($definition)) ? '__global' : $definition[0];
		$function = (!is_array($definition)) ? $definition : $definition[1];

		if (!empty($this->hooks[$class][$function]) && isset($this->hook_result[$class][$function]))
		{
			return array('result' => $this->hook_result[$class][$function]);
		}

		return false;
	}

	/**
	* Check if the called functions/methods returned something.
	*
	* @param mixed $definition Declaring function (with __FUNCTION__) or class with array(__CLASS__, __FUNCTION__)
	* @return bool True if results are there, false if not
	*/
	function hook_return($definition)
	{
		$class = (!is_array($definition)) ? '__global' : $definition[0];
		$function = (!is_array($definition)) ? $definition : $definition[1];

		if (!empty($this->hooks[$class][$function]) && isset($this->hook_result[$class][$function]))
		{
			return true;
		}

		return false;
	}

	/**
	* Give actual result from called functions/methods back.
	*
	* @param mixed $definition Declaring function (with __FUNCTION__) or class with array(__CLASS__, __FUNCTION__)
	* @return mixed The result
	*/
	function hook_return_result($definition)
	{
		$class = (!is_array($definition)) ? '__global' : $definition[0];
		$function = (!is_array($definition)) ? $definition : $definition[1];

		if (!empty($this->hooks[$class][$function]) && isset($this->hook_result[$class][$function]))
		{
			$result = $this->hook_result[$class][$function];
			unset($this->hook_result[$class][$function]);
			return $result;
		}

		return;
	}

	/**
	* Add new function to the allowed hooks.
	*
	* @param mixed $definition Declaring function (with __FUNCTION__) or class with array(__CLASS__, __FUNCTION__)
	*/
	function add_hook($definition)
	{
		if (!is_array($definition))
		{
			$definition = array('__global', $definition);
		}

		$this->hooks[$definition[0]][$definition[1]] = array();
	}

	/**
	* Remove function from the allowed hooks.
	*
	* @param mixed $definition Declaring function (with __FUNCTION__) or class with array(__CLASS__, __FUNCTION__)
	*/
	function remove_hook($definition)
	{
		$class = (!is_array($definition)) ? '__global' : $definition[0];
		$function = (!is_array($definition)) ? $definition : $definition[1];

		if (isset($this->hooks[$class][$function]))
		{
			unset($this->hooks[$class][$function]);

			if (isset($this->hook_result[$class][$function]))
			{
				unset($this->hook_result[$class][$function]);
			}
		}
	}
}

?>