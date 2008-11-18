<?php

/**
 * This file contains functions to convert callback templates to other languages.
 * The template should first be pre-processed with CBTProcessor to remove static
 * sections.
 */


require_once( dirname( __FILE__ ) . '/CBTProcessor.php' );

/**
 * Push a value onto the stack
 * Argument 1: value
 */
define( 'CBT_PUSH', 1 );

/**
 * Pop, concatenate argument, push
 * Argument 1: value
 */
define( 'CBT_CAT', 2 );

/**
 * Concatenate where the argument is on the stack, instead of immediate
 */
define( 'CBT_CATS', 3 );

/**
 * Call a function, push the return value onto the stack and put it in the cache
 * Argument 1: argument count
 *
 * The arguments to the function are on the stack
 */
define( 'CBT_CALL', 4 );

/**
 * Pop, htmlspecialchars, push
 */
define( 'CBT_HX', 5 );

class CBTOp {
	var $opcode;
	var $arg1;
	var $arg2;

	function CBTOp( $opcode, $arg1, $arg2 ) {
		$this->opcode = $opcode;
		$this->arg1 = $arg1;
		$this->arg2 = $arg2;
	}

	function name() {
		$opcodeNames = array(
			CBT_PUSH => 'PUSH',
			CBT_CAT => 'CAT',
			CBT_CATS => 'CATS',
			CBT_CALL => 'CALL',
			CBT_HX => 'HX',
		);
		return $opcodeNames[$this->opcode];
	}
};

class CBTCompiler {
	var $mOps = array();
	var $mCode;

	function CBTCompiler( $text ) {
		$this->mText = $text;
	}

	/**
	 * Compile the text.
	 * Returns true on success, error message on failure
	 */
	function compile() {
		$this->mLastError = false;
		$this->mOps = array();

		$this->doText( 0, strlen( $this->mText ) );

		if ( $this->mLastError !== false ) {
			$pos = $this->mErrorPos;

			// Find the line number at which the error occurred
			$startLine = 0;
			$endLine = 0;
			$line = 0;
			do {
				if ( $endLine ) {
					$startLine = $endLine + 1;
				}
				$endLine = strpos( $this->mText, "\n", $startLine );
				++$line;
			} while ( $endLine !== false && $endLine < $pos );

			$text = "Template error at line $line: $this->mLastError\n<pre>\n";

			$context = rtrim( str_replace( "\t", " ", substr( $this->mText, $startLine, $endLine - $startLine ) ) );
			$text .= htmlspecialchars( $context ) . "\n" . str_repeat( ' ', $pos - $startLine ) . "^\n</pre>\n";
		} else {
			$text = true;
		}

		return $text;
	}

	/** Shortcut for doOpenText( $start, $end, false */
	function doText( $start, $end ) {
		return $this->doOpenText( $start, $end, false );
	}

	function phpQuote( $text ) {
		return "'" . strtr( $text, array( "\\" => "\\\\", "'" => "\\'" ) ) . "'";
	}

	function op( $opcode, $arg1 = null, $arg2 = null) {
		return new CBTOp( $opcode, $arg1, $arg2 );
	}

	/**
	 * Recursive workhorse for text mode.
	 *
	 * Processes text mode starting from offset $p, until either $end is
	 * reached or a closing brace is found. If $needClosing is false, a
	 * closing brace will flag an error, if $needClosing is true, the lack
	 * of a closing brace will flag an error.
	 *
	 * The parameter $p is advanced to the position after the closing brace,
	 * or after the end. A CBTValue is returned.
	 *
	 * @private
	 */
	function doOpenText( &$p, $end, $needClosing = true ) {
		$in =& $this->mText;
		$start = $p;
		$atStart = true;

		$foundClosing = false;
		while ( $p < $end ) {
			$matchLength = strcspn( $in, CBT_BRACE, $p, $end - $p );
			$pToken = $p + $matchLength;

			if ( $pToken >= $end ) {
				// No more braces, output remainder
				if ( $atStart ) {
					$this->mOps[] = $this->op( CBT_PUSH, substr( $in, $p ) );
					$atStart = false;
				} else {
					$this->mOps[] = $this->op( CBT_CAT, substr( $in, $p ) );
				}
				$p = $end;
				break;
			}

			// Output the text before the brace
			if ( $atStart ) {
				$this->mOps[] = $this->op( CBT_PUSH, substr( $in, $p, $matchLength ) );
				$atStart = false;
			} else {
				$this->mOps[] = $this->op( CBT_CAT, substr( $in, $p, $matchLength ) );
			}

			// Advance the pointer
			$p = $pToken + 1;

			// Check for closing brace
			if ( $in[$pToken] == '}' ) {
				$foundClosing = true;
				break;
			}

			// Handle the "{fn}" special case
			if ( $pToken > 0 && $in[$pToken-1] == '"' ) {
				$this->doOpenFunction( $p, $end );
				if ( $p < $end && $in[$p] == '"' ) {
					$this->mOps[] = $this->op( CBT_HX );
				}
			} else {
				$this->doOpenFunction( $p, $end );
			}
			if ( $atStart ) {
				$atStart = false;
			} else {
				$this->mOps[] = $this->op( CBT_CATS );
			}
		}
		if ( $foundClosing && !$needClosing ) {
			$this->error( 'Errant closing brace', $p );
		} elseif ( !$foundClosing && $needClosing ) {
			$this->error( 'Unclosed text section', $start );
		} else {
			if ( $atStart ) {
				$this->mOps[] = $this->op( CBT_PUSH, '' );
			}
		}
	}

	/**
	 * Recursive workhorse for function mode.
	 *
	 * Processes function mode starting from offset $p, until either $end is
	 * reached or a closing brace is found. If $needClosing is false, a
	 * closing brace will flag an error, if $needClosing is true, the lack
	 * of a closing brace will flag an error.
	 *
	 * The parameter $p is advanced to the position after the closing brace,
	 * or after the end. A CBTValue is returned.
	 *
	 * @private
	 */
	function doOpenFunction( &$p, $end, $needClosing = true ) {
		$in =& $this->mText;
		$start = $p;
		$argCount = 0;

		$foundClosing = false;
		while ( $p < $end ) {
			$char = $in[$p];
			if ( $char == '{' ) {
				// Switch to text mode
				++$p;
				$this->doOpenText( $p, $end );
				++$argCount;
			} elseif ( $char == '}' ) {
				// Block end
				++$p;
				$foundClosing = true;
				break;
			} elseif ( false !== strpos( CBT_WHITE, $char ) ) {
				// Whitespace
				// Consume the rest of the whitespace
				$p += strspn( $in, CBT_WHITE, $p, $end - $p );
			} else {
				// Token, find the end of it
				$tokenLength = strcspn( $in, CBT_DELIM, $p, $end - $p );
				$this->mOps[] = $this->op( CBT_PUSH, substr( $in, $p, $tokenLength ) );

				// Execute the token as a function if it's not the function name
				if ( $argCount ) {
					$this->mOps[] = $this->op( CBT_CALL, 1 );
				}

				$p += $tokenLength;
				++$argCount;
			}
		}
		if ( !$foundClosing && $needClosing ) {
			$this->error( 'Unclosed function', $start );
			return '';
		}

		$this->mOps[] = $this->op( CBT_CALL, $argCount );
	}

	/**
	 * Set a flag indicating that an error has been found.
	 */
	function error( $text, $pos = false ) {
		$this->mLastError = $text;
		if ( $pos === false ) {
			$this->mErrorPos = $this->mCurrentPos;
		} else {
			$this->mErrorPos = $pos;
		}
	}

	function getLastError() {
		return $this->mLastError;
	}

	function opsToString() {
		$s = '';
		foreach( $this->mOps as $op ) {
			$s .= $op->name();
			if ( !is_null( $op->arg1 ) ) {
				$s .= ' ' . var_export( $op->arg1, true );
			}
			if ( !is_null( $op->arg2 ) ) {
				$s .= ' ' . var_export( $op->arg2, true );
			}
			$s .= "\n";
		}
		return $s;
	}

	function generatePHP( $functionObj ) {
		$fname = 'CBTCompiler::generatePHP';
		wfProfileIn( $fname );
		$stack = array();

		foreach( $this->mOps as $op ) {
			switch( $op->opcode ) {
				case CBT_PUSH:
					$stack[] = $this->phpQuote( $op->arg1 );
					break;
				case CBT_CAT:
					$val = array_pop( $stack );
					array_push( $stack, "$val . " . $this->phpQuote( $op->arg1 ) );
					break;
				case CBT_CATS:
					$right = array_pop( $stack );
					$left = array_pop( $stack );
					array_push( $stack, "$left . $right" );
					break;
				case CBT_CALL:
					$args = array_slice( $stack, count( $stack ) - $op->arg1, $op->arg1 );
					$stack = array_slice( $stack, 0, count( $stack ) - $op->arg1 );

					// Some special optimised expansions
					if ( $op->arg1 == 0 ) {
						$result = '';
					} else {
						$func = array_shift( $args );
						if ( substr( $func, 0, 1 ) == "'" &&  substr( $func, -1 ) == "'" ) {
							$func = substr( $func, 1, strlen( $func ) - 2 );
							if ( $func == "if" ) {
								if ( $op->arg1 < 3 ) {
									// This should have been caught during processing
									return "Not enough arguments to if";
								} elseif ( $op->arg1 == 3 ) {
									$result = "(({$args[0]} != '') ? ({$args[1]}) : '')";
								} else {
									$result = "(({$args[0]} != '') ? ({$args[1]}) : ({$args[2]}))";
								}
							} elseif ( $func == "true" ) {
								$result = "true";
							} elseif( $func == "lbrace" || $func == "{" ) {
								$result = "{";
							} elseif( $func == "rbrace" || $func == "}" ) {
								$result = "}";
							} elseif ( $func == "escape" || $func == "~" ) {
								$result = "htmlspecialchars({$args[0]})";
							} else {
								// Known function name
								$result = "{$functionObj}->{$func}(" . implode( ', ', $args ) . ')';
							}
						} else {
							// Unknown function name
							$result = "call_user_func(array($functionObj, $func), " . implode( ', ', $args ) . ' )';
						}
					}
					array_push( $stack, $result );
					break;
				case CBT_HX:
					$val = array_pop( $stack );
					array_push( $stack, "htmlspecialchars( $val )" );
					break;
				default:
					return "Unknown opcode {$op->opcode}\n";
			}
		}
		wfProfileOut( $fname );
		if ( count( $stack ) !== 1 ) {
			return "Error, stack count incorrect\n";
		}
		return '
			global $cbtExecutingGenerated;
			++$cbtExecutingGenerated;
			$output = ' . $stack[0] . ';
			--$cbtExecutingGenerated;
			return $output;
			';
	}
}
