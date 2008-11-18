<?php

if ( !defined( 'MEDIAWIKI' ) ) {
	die( 'This file is a MediaWiki extension, it is not a valid entry point' );
}

// Character classes
define( 'EXPR_WHITE_CLASS', " \t\r\n" );
define( 'EXPR_NUMBER_CLASS', '0123456789.' );

// Token types
define( 'EXPR_WHITE', 1 );
define( 'EXPR_NUMBER', 2 );
define( 'EXPR_NEGATIVE', 3 );
define( 'EXPR_POSITIVE', 4 );
define( 'EXPR_PLUS', 5 );
define( 'EXPR_MINUS', 6 );
define( 'EXPR_TIMES', 7 );
define( 'EXPR_DIVIDE', 8 );
define( 'EXPR_MOD', 9 );
define( 'EXPR_OPEN', 10 );
define( 'EXPR_CLOSE', 11 );
define( 'EXPR_AND', 12 );
define( 'EXPR_OR', 13 );
define( 'EXPR_NOT', 14 );
define( 'EXPR_EQUALITY', 15 );
define( 'EXPR_LESS', 16 );
define( 'EXPR_GREATER', 17 );
define( 'EXPR_LESSEQ', 18 );
define( 'EXPR_GREATEREQ', 19 );
define( 'EXPR_NOTEQ', 20 );
define( 'EXPR_ROUND', 21 );
define( 'EXPR_EXPONENT', 22 );
define( 'EXPR_SINE', 23 );
define( 'EXPR_COSINE', 24 );
define( 'EXPR_TANGENS', 25 );
define( 'EXPR_ARCSINE', 26 );
define( 'EXPR_ARCCOS', 27 );
define( 'EXPR_ARCTAN', 28 );
define( 'EXPR_EXP', 29 );
define( 'EXPR_LN', 30 );
define( 'EXPR_ABS', 31 );
define( 'EXPR_FLOOR', 32 );
define( 'EXPR_TRUNC', 33 );
define( 'EXPR_CEIL', 34 );
define( 'EXPR_POW', 35 );
define( 'EXPR_PI', 36 );

class ExprError extends Exception {
	public function __construct($msg, $parameter = ''){
		wfLoadExtensionMessages( 'ParserFunctions' );
		$this->message = '<strong class="error">' . wfMsgForContent( "pfunc_expr_$msg", htmlspecialchars( $parameter ) ) . '</strong>';
	}
}

class ExprParser {
	var $maxStackSize = 100;

	var $precedence = array(
		EXPR_NEGATIVE => 10,
		EXPR_POSITIVE => 10,
		EXPR_EXPONENT => 10,
		EXPR_SINE => 9,
		EXPR_COSINE => 9,
		EXPR_TANGENS => 9,
		EXPR_ARCSINE => 9,
		EXPR_ARCCOS => 9,
		EXPR_ARCTAN => 9,
		EXPR_EXP => 9,
		EXPR_LN => 9,
		EXPR_ABS => 9,
		EXPR_FLOOR => 9,
		EXPR_TRUNC => 9,
		EXPR_CEIL => 9,
		EXPR_NOT => 9,
		EXPR_POW => 8,
		EXPR_TIMES => 7,
		EXPR_DIVIDE => 7,
		EXPR_MOD => 7,
		EXPR_PLUS => 6,
		EXPR_MINUS => 6,
		EXPR_ROUND => 5,
		EXPR_EQUALITY => 4,
		EXPR_LESS => 4,
		EXPR_GREATER => 4,
		EXPR_LESSEQ => 4,
		EXPR_GREATEREQ => 4,
		EXPR_NOTEQ => 4,
		EXPR_AND => 3,
		EXPR_OR => 2,
		EXPR_PI => 0,
		EXPR_OPEN => -1,
		EXPR_CLOSE => -1,
	);

	var $names = array(
		EXPR_NEGATIVE => '-',
		EXPR_POSITIVE => '+',
		EXPR_NOT => 'not',
		EXPR_TIMES => '*',
		EXPR_DIVIDE => '/',
		EXPR_MOD => 'mod',
		EXPR_PLUS => '+',
		EXPR_MINUS => '-',
		EXPR_ROUND => 'round',
		EXPR_EQUALITY => '=',
		EXPR_LESS => '<',
		EXPR_GREATER => '>',
		EXPR_LESSEQ => '<=',
		EXPR_GREATEREQ => '>=',
		EXPR_NOTEQ => '<>',
		EXPR_AND => 'and',
		EXPR_OR => 'or',
		EXPR_EXPONENT => 'e',
		EXPR_SINE => 'sin',
		EXPR_COSINE => 'cos',
		EXPR_TANGENS => 'tan',
		EXPR_ARCSINE => 'asin',
		EXPR_ARCCOS => 'acos',
		EXPR_ARCTAN => 'atan',
		EXPR_LN => 'ln',
		EXPR_EXP => 'exp',
		EXPR_ABS => 'abs',
		EXPR_FLOOR => 'floor',
		EXPR_TRUNC => 'trunc',
		EXPR_CEIL => 'ceil',
		EXPR_POW => '^',
		EXPR_PI => 'pi',
	);


	var $words = array(
		'mod' => EXPR_MOD,
		'and' => EXPR_AND,
		'or' => EXPR_OR,
		'not' => EXPR_NOT,
		'round' => EXPR_ROUND,
		'div' => EXPR_DIVIDE,
		'e' => EXPR_EXPONENT,
		'sin' => EXPR_SINE,
		'cos' => EXPR_COSINE,
		'tan' => EXPR_TANGENS,
		'asin' => EXPR_ARCSINE,
		'acos' => EXPR_ARCCOS,
		'atan' => EXPR_ARCTAN,
		'exp' => EXPR_EXP,
		'ln' => EXPR_LN,
		'abs' => EXPR_ABS,
		'trunc' => EXPR_TRUNC,
		'floor' => EXPR_FLOOR,
		'ceil' => EXPR_CEIL,
		'pi' => EXPR_PI,
	);

	/**
	 * Evaluate a mathematical expression
	 *
	 * The algorithm here is based on the infix to RPN algorithm given in
	 * http://montcs.bloomu.edu/~bobmon/Information/RPN/infix2rpn.shtml
	 * It's essentially the same as Dijkstra's shunting yard algorithm.
	 */
	function doExpression( $expr ) {
		$operands = array();
		$operators = array();

		# Unescape inequality operators
		$expr = strtr( $expr, array( '&lt;' => '<', '&gt;' => '>' ) );

		$p = 0;
		$end = strlen( $expr );
		$expecting = 'expression';

		while ( $p < $end ) {
			if ( count( $operands ) > $this->maxStackSize || count( $operators ) > $this->maxStackSize ) {
				throw new ExprError('stack_exhausted');
			}
			$char = $expr[$p];
			$char2 = substr( $expr, $p, 2 );

			// Mega if-elseif-else construct
			// Only binary operators fall through for processing at the bottom, the rest
			// finish their processing and continue

			// First the unlimited length classes

			if ( false !== strpos( EXPR_WHITE_CLASS, $char ) ) {
				// Whitespace
				$p += strspn( $expr, EXPR_WHITE_CLASS, $p );
				continue;
			} elseif ( false !== strpos( EXPR_NUMBER_CLASS, $char ) ) {
				// Number
				if ( $expecting != 'expression' ) {
					throw new ExprError('unexpected_number');
				}

				// Find the rest of it
				$length = strspn( $expr, EXPR_NUMBER_CLASS, $p );
				// Convert it to float, silently removing double decimal points
				$operands[] = floatval( substr( $expr, $p, $length ) );
				$p += $length;
				$expecting = 'operator';
				continue;
			} elseif ( ctype_alpha( $char ) ) {
				// Word
				// Find the rest of it
				$remaining = substr( $expr, $p );
				if ( !preg_match( '/^[A-Za-z]*/', $remaining, $matches ) ) {
					// This should be unreachable
					throw new ExprError('preg_match_failure');
				}
				$word = strtolower( $matches[0] );
				$p += strlen( $word );

				// Interpret the word
				if ( !isset( $this->words[$word] ) ){
					throw new ExprError('unrecognised_word', $word);
				}
				$op = $this->words[$word];
				switch( $op ) {
				// constant
				case EXPR_EXPONENT:
					if ( $expecting != 'expression' ) {
						continue;
					}
					$operands[] = exp(1);
					$expecting = 'operator';
					continue 2;
				case EXPR_PI:
					if ( $expecting != 'expression' ) {
						throw new ExprError( 'unexpected_number' );
					}
					$operands[] = pi();
					$expecting = 'operator';
					continue 2;
				// Unary operator
				case EXPR_NOT:
				case EXPR_SINE:
				case EXPR_COSINE:
				case EXPR_TANGENS:
				case EXPR_ARCSINE:
				case EXPR_ARCCOS:
				case EXPR_ARCTAN:
				case EXPR_EXP:
				case EXPR_LN:
				case EXPR_ABS:
				case EXPR_FLOOR:
				case EXPR_TRUNC:
				case EXPR_CEIL:
					if ( $expecting != 'expression' ) {
						throw new ExprError( 'unexpected_operator', $word );
					}
					$operators[] = $op;
					continue 2;
				}
				// Binary operator, fall through
				$name = $word;
			}

			// Next the two-character operators

			elseif ( $char2 == '<=' ) {
				$name = $char2;
				$op = EXPR_LESSEQ;
				$p += 2;
			} elseif ( $char2 == '>=' ) {
				$name = $char2;
				$op = EXPR_GREATEREQ;
				$p += 2;
			} elseif ( $char2 == '<>' || $char2 == '!=' ) {
				$name = $char2;
				$op = EXPR_NOTEQ;
				$p += 2;
			}

			// Finally the single-character operators
			
			elseif ( $char == '+' ) {
				++$p;
				if ( $expecting == 'expression' ) {
					// Unary plus
					$operators[] = EXPR_POSITIVE;
					continue;
				} else {
					// Binary plus
					$op = EXPR_PLUS;
				}
			} elseif ( $char == '-' ) {
				++$p;
				if ( $expecting == 'expression' ) {
					// Unary minus
					$operators[] = EXPR_NEGATIVE;
					continue;
				} else {
					// Binary minus
					$op = EXPR_MINUS;
				}
			} elseif ( $char == '*' ) {
				$name = $char;
				$op = EXPR_TIMES;
				++$p;
			} elseif ( $char == '/' ) {
				$name = $char;
				$op = EXPR_DIVIDE;
				++$p;
			} elseif ( $char == '^' ) {
				$name = $char;
				$op = EXPR_POW;
				++$p;
			} elseif ( $char == '(' )  {
				if ( $expecting == 'operator' ) {
					throw new ExprError('unexpected_operator', '(');
				}
				$operators[] = EXPR_OPEN;
				++$p;
				continue;
			} elseif ( $char == ')' ) {
				$lastOp = end( $operators );
				while ( $lastOp && $lastOp != EXPR_OPEN ) {
					$this->doOperation( $lastOp, $operands );
					array_pop( $operators );
					$lastOp = end( $operators );
				}
				if ( $lastOp ) {
					array_pop( $operators );
				} else {
					throw new ExprError('unexpected_closing_bracket');
				}
				$expecting = 'operator';
				++$p;
				continue;
			} elseif ( $char == '=' ) {
				$name = $char;
				$op = EXPR_EQUALITY;
				++$p;
			} elseif ( $char == '<' ) {
				$name = $char;
				$op = EXPR_LESS;
				++$p;
			} elseif ( $char == '>' ) {
				$name = $char;
				$op = EXPR_GREATER;
				++$p;
			} else {
				throw new ExprError('unrecognised_punctuation', UtfNormal::cleanUp( $char ));
			}

			// Binary operator processing
			if ( $expecting == 'expression' ) {
				throw new ExprError('unexpected_operator', $name);
			}

			// Shunting yard magic
			$lastOp = end( $operators );
			while ( $lastOp && $this->precedence[$op] <= $this->precedence[$lastOp] ) {
				$this->doOperation( $lastOp, $operands );
				array_pop( $operators );
				$lastOp = end( $operators );
			}
			$operators[] = $op;
			$expecting = 'expression';
		}

		// Finish off the operator array
		while ( $op = array_pop( $operators ) ) {
			if ( $op == EXPR_OPEN ) {
				throw new ExprError('unclosed_bracket');
			}
			$this->doOperation( $op, $operands );
		}

		return implode( "<br />\n", $operands );
	}

	function doOperation( $op, &$stack ) {
		switch ( $op ) {
			case EXPR_NEGATIVE:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				$stack[] = -$arg;
				break;
			case EXPR_POSITIVE:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				break;
			case EXPR_TIMES:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				$stack[] = $left * $right;
					break;
			case EXPR_DIVIDE:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				if ( $right == 0 ) throw new ExprError('division_by_zero', $this->names[$op]);
				$stack[] = $left / $right;
				break;
			case EXPR_MOD:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				if ( $right == 0 ) throw new ExprError('division_by_zero', $this->names[$op]);
				$stack[] = $left % $right;
				break;
			case EXPR_PLUS:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				$stack[] = $left + $right;
				break;
			case EXPR_MINUS:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				$stack[] = $left - $right;
				break;
			case EXPR_AND:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				$stack[] = ( $left && $right ) ? 1 : 0;
				break;
			case EXPR_OR:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				$stack[] = ( $left || $right ) ? 1 : 0;
				break;
			case EXPR_EQUALITY:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				$stack[] = ( $left == $right ) ? 1 : 0;
				break;
			case EXPR_NOT:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				$stack[] = (!$arg) ? 1 : 0;
				break;
			case EXPR_ROUND:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$digits = intval( array_pop( $stack ) );
				$value = array_pop( $stack );
				$stack[] = round( $value, $digits );
				break;
			case EXPR_LESS:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				$stack[] = ( $left < $right ) ? 1 : 0;
				break;
			case EXPR_GREATER:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				$stack[] = ( $left > $right ) ? 1 : 0;
				break;
			case EXPR_LESSEQ:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				$stack[] = ( $left <= $right ) ? 1 : 0;
				break;
			case EXPR_GREATEREQ:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				$stack[] = ( $left >= $right ) ? 1 : 0;
				break;
			case EXPR_NOTEQ:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				$stack[] = ( $left != $right ) ? 1 : 0;
				break;
			case EXPR_EXPONENT:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				$stack[] = $left * pow(10,$right);
				break;
			case EXPR_SINE:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				$stack[] = sin($arg);
				break;
			case EXPR_COSINE:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				$stack[] = cos($arg);
				break;
			case EXPR_TANGENS:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				$stack[] = tan($arg);
				break;
			case EXPR_ARCSINE:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				if ( $arg < -1 || $arg > 1 ) throw new ExprError('invalid_argument', $this->names[$op] );
				$stack[] = asin($arg);
				break;
			case EXPR_ARCCOS:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				if ( $arg < -1 || $arg > 1 ) throw new ExprError('invalid_argument', $this->names[$op] );
				$stack[] = acos($arg);
				break;
			case EXPR_ARCTAN:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				$stack[] = atan($arg);
				break;
			case EXPR_EXP:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				$stack[] = exp($arg);
				break;
			case EXPR_LN:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				if ( $arg <= 0 ) throw new ExprError('invalid_argument_ln', $this->names[$op]);
				$stack[] = log($arg);
				break;
			case EXPR_ABS:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				$stack[] = abs($arg);
				break;
			case EXPR_FLOOR:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				$stack[] = floor($arg);
				break;
			case EXPR_TRUNC:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				$stack[] = (int)$arg;
				break;
			case EXPR_CEIL:
				if ( count( $stack ) < 1 ) throw new ExprError('missing_operand', $this->names[$op]);
				$arg = array_pop( $stack );
				$stack[] = ceil($arg);
				break;
			case EXPR_POW:
				if ( count( $stack ) < 2 ) throw new ExprError('missing_operand', $this->names[$op]);
				$right = array_pop( $stack );
				$left = array_pop( $stack );
				if ( false === ($stack[] = pow($left, $right)) ) throw new ExprError('division_by_zero', $this->names[$op]);
				break;
			default:
				// Should be impossible to reach here.
				throw new ExprError('unknown_error');
		}
	}
}
