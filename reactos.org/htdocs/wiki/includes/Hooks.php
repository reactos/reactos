<?php
/**
 * Hooks.php -- a tool for running hook functions
 * Copyright 2004, 2005 Evan Prodromou <evan@wikitravel.org>.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * @author Evan Prodromou <evan@wikitravel.org>
 * @see hooks.txt
 * @file
 */


/**
 * Because programmers assign to $wgHooks, we need to be very
 * careful about its contents. So, there's a lot more error-checking
 * in here than would normally be necessary.
 */
function wfRunHooks($event, $args = array()) {

	global $wgHooks;

	if (!is_array($wgHooks)) {
		throw new MWException("Global hooks array is not an array!\n");
		return false;
	}

	if (!array_key_exists($event, $wgHooks)) {
		return true;
	}

	if (!is_array($wgHooks[$event])) {
		throw new MWException("Hooks array for event '$event' is not an array!\n");
		return false;
	}

	foreach ($wgHooks[$event] as $index => $hook) {

		$object = NULL;
		$method = NULL;
		$func = NULL;
		$data = NULL;
		$have_data = false;

		/* $hook can be: a function, an object, an array of $function and $data,
		 * an array of just a function, an array of object and method, or an
		 * array of object, method, and data.
		 */

		if (is_array($hook)) {
			if (count($hook) < 1) {
				throw new MWException("Empty array in hooks for " . $event . "\n");
			} else if (is_object($hook[0])) {
				$object = $wgHooks[$event][$index][0];
				if (count($hook) < 2) {
					$method = "on" . $event;
				} else {
					$method = $hook[1];
					if (count($hook) > 2) {
						$data = $hook[2];
						$have_data = true;
					}
				}
			} else if (is_string($hook[0])) {
				$func = $hook[0];
				if (count($hook) > 1) {
					$data = $hook[1];
					$have_data = true;
				}
			} else {
				var_dump( $wgHooks );
				throw new MWException("Unknown datatype in hooks for " . $event . "\n");
			}
		} else if (is_string($hook)) { # functions look like strings, too
			$func = $hook;
		} else if (is_object($hook)) {
			$object = $wgHooks[$event][$index];
			$method = "on" . $event;
		} else {
			throw new MWException("Unknown datatype in hooks for " . $event . "\n");
		}

		/* We put the first data element on, if needed. */

		if ($have_data) {
			$hook_args = array_merge(array($data), $args);
		} else {
			$hook_args = $args;
		}

		if ( isset( $object ) ) {
			$func = get_class( $object ) . '::' . $method;
			$callback = array( $object, $method );
		} elseif ( false !== ( $pos = strpos( $func, '::' ) ) ) {
			$callback = array( substr( $func, 0, $pos ), substr( $func, $pos + 2 ) );
		} else {
			$callback = $func;
		}

		// Run autoloader (workaround for call_user_func_array bug)
		is_callable( $callback );

		/* Call the hook. */
		wfProfileIn( $func );
		$retval = call_user_func_array( $callback, $hook_args );
		wfProfileOut( $func );

		/* String return is an error; false return means stop processing. */

		if (is_string($retval)) {
			global $wgOut;
			$wgOut->showFatalError($retval);
			return false;
		} elseif( $retval === null ) {
			if( is_array( $callback ) ) {
				if( is_object( $callback[0] ) ) {
					$prettyClass = get_class( $callback[0] );
				} else {
					$prettyClass = strval( $callback[0] );
				}
				$prettyFunc = $prettyClass . '::' . strval( $callback[1] );
			} else {
				$prettyFunc = strval( $callback );
			}
			throw new MWException( "Detected bug in an extension! " .
				"Hook $prettyFunc failed to return a value; " .
				"should return true to continue hook processing or false to abort." );
		} else if (!$retval) {
			return false;
		}
	}

	return true;
}
