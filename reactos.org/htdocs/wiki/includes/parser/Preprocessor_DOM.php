<?php

/**
 * @ingroup Parser
 */
class Preprocessor_DOM implements Preprocessor {
	var $parser, $memoryLimit;

	function __construct( $parser ) {
		$this->parser = $parser;
		$mem = ini_get( 'memory_limit' );
		$this->memoryLimit = false;
		if ( strval( $mem ) !== '' && $mem != -1 ) {
			if ( preg_match( '/^\d+$/', $mem ) ) {
				$this->memoryLimit = $mem;
			} elseif ( preg_match( '/^(\d+)M$/i', $mem, $m ) ) {
				$this->memoryLimit = $m[1] * 1048576;
			}
		}
	}

	function newFrame() {
		return new PPFrame_DOM( $this );
	}

	function newCustomFrame( $args ) {
		return new PPCustomFrame_DOM( $this, $args );
	}

	function memCheck() {
		if ( $this->memoryLimit === false ) {
			return;
		}
		$usage = memory_get_usage();
		if ( $usage > $this->memoryLimit * 0.9 ) {
			$limit = intval( $this->memoryLimit * 0.9 / 1048576 + 0.5 );
			throw new MWException( "Preprocessor hit 90% memory limit ($limit MB)" );
		}
		return $usage <= $this->memoryLimit * 0.8;
	}

	/**
	 * Preprocess some wikitext and return the document tree.
	 * This is the ghost of Parser::replace_variables().
	 *
	 * @param string $text The text to parse
	 * @param integer flags Bitwise combination of:
	 *          Parser::PTD_FOR_INCLUSION    Handle <noinclude>/<includeonly> as if the text is being
	 *                                     included. Default is to assume a direct page view.
	 *
	 * The generated DOM tree must depend only on the input text and the flags.
	 * The DOM tree must be the same in OT_HTML and OT_WIKI mode, to avoid a regression of bug 4899.
	 *
	 * Any flag added to the $flags parameter here, or any other parameter liable to cause a
	 * change in the DOM tree for a given text, must be passed through the section identifier
	 * in the section edit link and thus back to extractSections().
	 *
	 * The output of this function is currently only cached in process memory, but a persistent
	 * cache may be implemented at a later date which takes further advantage of these strict
	 * dependency requirements.
	 *
	 * @private
	 */
	function preprocessToObj( $text, $flags = 0 ) {
		wfProfileIn( __METHOD__ );
		wfProfileIn( __METHOD__.'-makexml' );

		$rules = array(
			'{' => array(
				'end' => '}',
				'names' => array(
					2 => 'template',
					3 => 'tplarg',
				),
				'min' => 2,
				'max' => 3,
			),
			'[' => array(
				'end' => ']',
				'names' => array( 2 => null ),
				'min' => 2,
				'max' => 2,
			)
		);

		$forInclusion = $flags & Parser::PTD_FOR_INCLUSION;

		$xmlishElements = $this->parser->getStripList();
		$enableOnlyinclude = false;
		if ( $forInclusion ) {
			$ignoredTags = array( 'includeonly', '/includeonly' );
			$ignoredElements = array( 'noinclude' );
			$xmlishElements[] = 'noinclude';
			if ( strpos( $text, '<onlyinclude>' ) !== false && strpos( $text, '</onlyinclude>' ) !== false ) {
				$enableOnlyinclude = true;
			}
		} else {
			$ignoredTags = array( 'noinclude', '/noinclude', 'onlyinclude', '/onlyinclude' );
			$ignoredElements = array( 'includeonly' );
			$xmlishElements[] = 'includeonly';
		}
		$xmlishRegex = implode( '|', array_merge( $xmlishElements, $ignoredTags ) );

		// Use "A" modifier (anchored) instead of "^", because ^ doesn't work with an offset
		$elementsRegex = "~($xmlishRegex)(?:\s|\/>|>)|(!--)~iA";

		$stack = new PPDStack;

		$searchBase = "[{<\n"; #}
		$revText = strrev( $text ); // For fast reverse searches

		$i = 0;                     # Input pointer, starts out pointing to a pseudo-newline before the start
		$accum =& $stack->getAccum();   # Current accumulator
		$accum = '<root>';
		$findEquals = false;            # True to find equals signs in arguments
		$findPipe = false;              # True to take notice of pipe characters
		$headingIndex = 1;
		$inHeading = false;        # True if $i is inside a possible heading
		$noMoreGT = false;         # True if there are no more greater-than (>) signs right of $i
		$findOnlyinclude = $enableOnlyinclude; # True to ignore all input up to the next <onlyinclude>
		$fakeLineStart = true;     # Do a line-start run without outputting an LF character

		while ( true ) {
			//$this->memCheck();

			if ( $findOnlyinclude ) {
				// Ignore all input up to the next <onlyinclude>
				$startPos = strpos( $text, '<onlyinclude>', $i );
				if ( $startPos === false ) {
					// Ignored section runs to the end
					$accum .= '<ignore>' . htmlspecialchars( substr( $text, $i ) ) . '</ignore>';
					break;
				}
				$tagEndPos = $startPos + strlen( '<onlyinclude>' ); // past-the-end
				$accum .= '<ignore>' . htmlspecialchars( substr( $text, $i, $tagEndPos - $i ) ) . '</ignore>';
				$i = $tagEndPos;
				$findOnlyinclude = false;
			}

			if ( $fakeLineStart ) {
				$found = 'line-start';
				$curChar = '';
			} else {
				# Find next opening brace, closing brace or pipe
				$search = $searchBase;
				if ( $stack->top === false ) {
					$currentClosing = '';
				} else {
					$currentClosing = $stack->top->close;
					$search .= $currentClosing;
				}
				if ( $findPipe ) {
					$search .= '|';
				}
				if ( $findEquals ) {
					// First equals will be for the template
					$search .= '=';
				}
				$rule = null;
				# Output literal section, advance input counter
				$literalLength = strcspn( $text, $search, $i );
				if ( $literalLength > 0 ) {
					$accum .= htmlspecialchars( substr( $text, $i, $literalLength ) );
					$i += $literalLength;
				}
				if ( $i >= strlen( $text ) ) {
					if ( $currentClosing == "\n" ) {
						// Do a past-the-end run to finish off the heading
						$curChar = '';
						$found = 'line-end';
					} else {
						# All done
						break;
					}
				} else {
					$curChar = $text[$i];
					if ( $curChar == '|' ) {
						$found = 'pipe';
					} elseif ( $curChar == '=' ) {
						$found = 'equals';
					} elseif ( $curChar == '<' ) {
						$found = 'angle';
					} elseif ( $curChar == "\n" ) {
						if ( $inHeading ) {
							$found = 'line-end';
						} else {
							$found = 'line-start';
						}
					} elseif ( $curChar == $currentClosing ) {
						$found = 'close';
					} elseif ( isset( $rules[$curChar] ) ) {
						$found = 'open';
						$rule = $rules[$curChar];
					} else {
						# Some versions of PHP have a strcspn which stops on null characters
						# Ignore and continue
						++$i;
						continue;
					}
				}
			}

			if ( $found == 'angle' ) {
				$matches = false;
				// Handle </onlyinclude>
				if ( $enableOnlyinclude && substr( $text, $i, strlen( '</onlyinclude>' ) ) == '</onlyinclude>' ) {
					$findOnlyinclude = true;
					continue;
				}

				// Determine element name
				if ( !preg_match( $elementsRegex, $text, $matches, 0, $i + 1 ) ) {
					// Element name missing or not listed
					$accum .= '&lt;';
					++$i;
					continue;
				}
				// Handle comments
				if ( isset( $matches[2] ) && $matches[2] == '!--' ) {
					// To avoid leaving blank lines, when a comment is both preceded
					// and followed by a newline (ignoring spaces), trim leading and
					// trailing spaces and one of the newlines.

					// Find the end
					$endPos = strpos( $text, '-->', $i + 4 );
					if ( $endPos === false ) {
						// Unclosed comment in input, runs to end
						$inner = substr( $text, $i );
						$accum .= '<comment>' . htmlspecialchars( $inner ) . '</comment>';
						$i = strlen( $text );
					} else {
						// Search backwards for leading whitespace
						$wsStart = $i ? ( $i - strspn( $revText, ' ', strlen( $text ) - $i ) ) : 0;
						// Search forwards for trailing whitespace
						// $wsEnd will be the position of the last space
						$wsEnd = $endPos + 2 + strspn( $text, ' ', $endPos + 3 );
						// Eat the line if possible
						// TODO: This could theoretically be done if $wsStart == 0, i.e. for comments at
						// the overall start. That's not how Sanitizer::removeHTMLcomments() did it, but
						// it's a possible beneficial b/c break.
						if ( $wsStart > 0 && substr( $text, $wsStart - 1, 1 ) == "\n"
							&& substr( $text, $wsEnd + 1, 1 ) == "\n" )
						{
							$startPos = $wsStart;
							$endPos = $wsEnd + 1;
							// Remove leading whitespace from the end of the accumulator
							// Sanity check first though
							$wsLength = $i - $wsStart;
							if ( $wsLength > 0 && substr( $accum, -$wsLength ) === str_repeat( ' ', $wsLength ) ) {
								$accum = substr( $accum, 0, -$wsLength );
							}
							// Do a line-start run next time to look for headings after the comment
							$fakeLineStart = true;
						} else {
							// No line to eat, just take the comment itself
							$startPos = $i;
							$endPos += 2;
						}

						if ( $stack->top ) {
							$part = $stack->top->getCurrentPart();
							if ( isset( $part->commentEnd ) && $part->commentEnd == $wsStart - 1 ) {
								// Comments abutting, no change in visual end
								$part->commentEnd = $wsEnd;
							} else {
								$part->visualEnd = $wsStart;
								$part->commentEnd = $endPos;
							}
						}
						$i = $endPos + 1;
						$inner = substr( $text, $startPos, $endPos - $startPos + 1 );
						$accum .= '<comment>' . htmlspecialchars( $inner ) . '</comment>';
					}
					continue;
				}
				$name = $matches[1];
				$lowerName = strtolower( $name );
				$attrStart = $i + strlen( $name ) + 1;

				// Find end of tag
				$tagEndPos = $noMoreGT ? false : strpos( $text, '>', $attrStart );
				if ( $tagEndPos === false ) {
					// Infinite backtrack
					// Disable tag search to prevent worst-case O(N^2) performance
					$noMoreGT = true;
					$accum .= '&lt;';
					++$i;
					continue;
				}

				// Handle ignored tags
				if ( in_array( $lowerName, $ignoredTags ) ) {
					$accum .= '<ignore>' . htmlspecialchars( substr( $text, $i, $tagEndPos - $i + 1 ) ) . '</ignore>';
					$i = $tagEndPos + 1;
					continue;
				}

				$tagStartPos = $i;
				if ( $text[$tagEndPos-1] == '/' ) {
					$attrEnd = $tagEndPos - 1;
					$inner = null;
					$i = $tagEndPos + 1;
					$close = '';
				} else {
					$attrEnd = $tagEndPos;
					// Find closing tag
					if ( preg_match( "/<\/$name\s*>/i", $text, $matches, PREG_OFFSET_CAPTURE, $tagEndPos + 1 ) ) {
						$inner = substr( $text, $tagEndPos + 1, $matches[0][1] - $tagEndPos - 1 );
						$i = $matches[0][1] + strlen( $matches[0][0] );
						$close = '<close>' . htmlspecialchars( $matches[0][0] ) . '</close>';
					} else {
						// No end tag -- let it run out to the end of the text.
						$inner = substr( $text, $tagEndPos + 1 );
						$i = strlen( $text );
						$close = '';
					}
				}
				// <includeonly> and <noinclude> just become <ignore> tags
				if ( in_array( $lowerName, $ignoredElements ) ) {
					$accum .= '<ignore>' . htmlspecialchars( substr( $text, $tagStartPos, $i - $tagStartPos ) )
						. '</ignore>';
					continue;
				}

				$accum .= '<ext>';
				if ( $attrEnd <= $attrStart ) {
					$attr = '';
				} else {
					$attr = substr( $text, $attrStart, $attrEnd - $attrStart );
				}
				$accum .= '<name>' . htmlspecialchars( $name ) . '</name>' .
					// Note that the attr element contains the whitespace between name and attribute,
					// this is necessary for precise reconstruction during pre-save transform.
					'<attr>' . htmlspecialchars( $attr ) . '</attr>';
				if ( $inner !== null ) {
					$accum .= '<inner>' . htmlspecialchars( $inner ) . '</inner>';
				}
				$accum .= $close . '</ext>';
			}

			elseif ( $found == 'line-start' ) {
				// Is this the start of a heading?
				// Line break belongs before the heading element in any case
				if ( $fakeLineStart ) {
					$fakeLineStart = false;
				} else {
					$accum .= $curChar;
					$i++;
				}

				$count = strspn( $text, '=', $i, 6 );
				if ( $count == 1 && $findEquals ) {
					// DWIM: This looks kind of like a name/value separator
					// Let's let the equals handler have it and break the potential heading
					// This is heuristic, but AFAICT the methods for completely correct disambiguation are very complex.
				} elseif ( $count > 0 ) {
					$piece = array(
						'open' => "\n",
						'close' => "\n",
						'parts' => array( new PPDPart( str_repeat( '=', $count ) ) ),
						'startPos' => $i,
						'count' => $count );
					$stack->push( $piece );
					$accum =& $stack->getAccum();
					extract( $stack->getFlags() );
					$i += $count;
				}
			}

			elseif ( $found == 'line-end' ) {
				$piece = $stack->top;
				// A heading must be open, otherwise \n wouldn't have been in the search list
				assert( $piece->open == "\n" );
				$part = $piece->getCurrentPart();
				// Search back through the input to see if it has a proper close
				// Do this using the reversed string since the other solutions (end anchor, etc.) are inefficient
				$wsLength = strspn( $revText, " \t", strlen( $text ) - $i );
				$searchStart = $i - $wsLength;
				if ( isset( $part->commentEnd ) && $searchStart - 1 == $part->commentEnd ) {
					// Comment found at line end
					// Search for equals signs before the comment
					$searchStart = $part->visualEnd;
					$searchStart -= strspn( $revText, " \t", strlen( $text ) - $searchStart );
				}
				$count = $piece->count;
				$equalsLength = strspn( $revText, '=', strlen( $text ) - $searchStart );
				if ( $equalsLength > 0 ) {
					if ( $i - $equalsLength == $piece->startPos ) {
						// This is just a single string of equals signs on its own line
						// Replicate the doHeadings behaviour /={count}(.+)={count}/
						// First find out how many equals signs there really are (don't stop at 6)
						$count = $equalsLength;
						if ( $count < 3 ) {
							$count = 0;
						} else {
							$count = min( 6, intval( ( $count - 1 ) / 2 ) );
						}
					} else {
						$count = min( $equalsLength, $count );
					}
					if ( $count > 0 ) {
						// Normal match, output <h>
						$element = "<h level=\"$count\" i=\"$headingIndex\">$accum</h>";
						$headingIndex++;
					} else {
						// Single equals sign on its own line, count=0
						$element = $accum;
					}
				} else {
					// No match, no <h>, just pass down the inner text
					$element = $accum;
				}
				// Unwind the stack
				$stack->pop();
				$accum =& $stack->getAccum();
				extract( $stack->getFlags() );

				// Append the result to the enclosing accumulator
				$accum .= $element;
				// Note that we do NOT increment the input pointer.
				// This is because the closing linebreak could be the opening linebreak of
				// another heading. Infinite loops are avoided because the next iteration MUST
				// hit the heading open case above, which unconditionally increments the
				// input pointer.
			}

			elseif ( $found == 'open' ) {
				# count opening brace characters
				$count = strspn( $text, $curChar, $i );

				# we need to add to stack only if opening brace count is enough for one of the rules
				if ( $count >= $rule['min'] ) {
					# Add it to the stack
					$piece = array(
						'open' => $curChar,
						'close' => $rule['end'],
						'count' => $count,
						'lineStart' => ($i > 0 && $text[$i-1] == "\n"),
					);

					$stack->push( $piece );
					$accum =& $stack->getAccum();
					extract( $stack->getFlags() );
				} else {
					# Add literal brace(s)
					$accum .= htmlspecialchars( str_repeat( $curChar, $count ) );
				}
				$i += $count;
			}

			elseif ( $found == 'close' ) {
				$piece = $stack->top;
				# lets check if there are enough characters for closing brace
				$maxCount = $piece->count;
				$count = strspn( $text, $curChar, $i, $maxCount );

				# check for maximum matching characters (if there are 5 closing
				# characters, we will probably need only 3 - depending on the rules)
				$matchingCount = 0;
				$rule = $rules[$piece->open];
				if ( $count > $rule['max'] ) {
					# The specified maximum exists in the callback array, unless the caller
					# has made an error
					$matchingCount = $rule['max'];
				} else {
					# Count is less than the maximum
					# Skip any gaps in the callback array to find the true largest match
					# Need to use array_key_exists not isset because the callback can be null
					$matchingCount = $count;
					while ( $matchingCount > 0 && !array_key_exists( $matchingCount, $rule['names'] ) ) {
						--$matchingCount;
					}
				}

				if ($matchingCount <= 0) {
					# No matching element found in callback array
					# Output a literal closing brace and continue
					$accum .= htmlspecialchars( str_repeat( $curChar, $count ) );
					$i += $count;
					continue;
				}
				$name = $rule['names'][$matchingCount];
				if ( $name === null ) {
					// No element, just literal text
					$element = $piece->breakSyntax( $matchingCount ) . str_repeat( $rule['end'], $matchingCount );
				} else {
					# Create XML element
					# Note: $parts is already XML, does not need to be encoded further
					$parts = $piece->parts;
					$title = $parts[0]->out;
					unset( $parts[0] );

					# The invocation is at the start of the line if lineStart is set in
					# the stack, and all opening brackets are used up.
					if ( $maxCount == $matchingCount && !empty( $piece->lineStart ) ) {
						$attr = ' lineStart="1"';
					} else {
						$attr = '';
					}

					$element = "<$name$attr>";
					$element .= "<title>$title</title>";
					$argIndex = 1;
					foreach ( $parts as $partIndex => $part ) {
						if ( isset( $part->eqpos ) ) {
							$argName = substr( $part->out, 0, $part->eqpos );
							$argValue = substr( $part->out, $part->eqpos + 1 );
							$element .= "<part><name>$argName</name>=<value>$argValue</value></part>";
						} else {
							$element .= "<part><name index=\"$argIndex\" /><value>{$part->out}</value></part>";
							$argIndex++;
						}
					}
					$element .= "</$name>";
				}

				# Advance input pointer
				$i += $matchingCount;

				# Unwind the stack
				$stack->pop();
				$accum =& $stack->getAccum();

				# Re-add the old stack element if it still has unmatched opening characters remaining
				if ($matchingCount < $piece->count) {
					$piece->parts = array( new PPDPart );
					$piece->count -= $matchingCount;
					# do we still qualify for any callback with remaining count?
					$names = $rules[$piece->open]['names'];
					$skippedBraces = 0;
					$enclosingAccum =& $accum;
					while ( $piece->count ) {
						if ( array_key_exists( $piece->count, $names ) ) {
							$stack->push( $piece );
							$accum =& $stack->getAccum();
							break;
						}
						--$piece->count;
						$skippedBraces ++;
					}
					$enclosingAccum .= str_repeat( $piece->open, $skippedBraces );
				}

				extract( $stack->getFlags() );

				# Add XML element to the enclosing accumulator
				$accum .= $element;
			}

			elseif ( $found == 'pipe' ) {
				$findEquals = true; // shortcut for getFlags()
				$stack->addPart();
				$accum =& $stack->getAccum();
				++$i;
			}

			elseif ( $found == 'equals' ) {
				$findEquals = false; // shortcut for getFlags()
				$stack->getCurrentPart()->eqpos = strlen( $accum );
				$accum .= '=';
				++$i;
			}
		}

		# Output any remaining unclosed brackets
		foreach ( $stack->stack as $piece ) {
			$stack->rootAccum .= $piece->breakSyntax();
		}
		$stack->rootAccum .= '</root>';
		$xml = $stack->rootAccum;

		wfProfileOut( __METHOD__.'-makexml' );
		wfProfileIn( __METHOD__.'-loadXML' );
		$dom = new DOMDocument;
		wfSuppressWarnings();
		$result = $dom->loadXML( $xml );
		wfRestoreWarnings();
		if ( !$result ) {
			// Try running the XML through UtfNormal to get rid of invalid characters
			$xml = UtfNormal::cleanUp( $xml );
			$result = $dom->loadXML( $xml );
			if ( !$result ) {
				throw new MWException( __METHOD__.' generated invalid XML' );
			}
		}
		$obj = new PPNode_DOM( $dom->documentElement );
		wfProfileOut( __METHOD__.'-loadXML' );
		wfProfileOut( __METHOD__ );
		return $obj;
	}
}

/**
 * Stack class to help Preprocessor::preprocessToObj()
 * @ingroup Parser
 */
class PPDStack {
	var $stack, $rootAccum, $top;
	var $out;
	var $elementClass = 'PPDStackElement';

	static $false = false;

	function __construct() {
		$this->stack = array();
		$this->top = false;
		$this->rootAccum = '';
		$this->accum =& $this->rootAccum;
	}

	function count() {
		return count( $this->stack );
	}

	function &getAccum() {
		return $this->accum;
	}

	function getCurrentPart() {
		if ( $this->top === false ) {
			return false;
		} else {
			return $this->top->getCurrentPart();
		}
	}

	function push( $data ) {
		if ( $data instanceof $this->elementClass ) {
			$this->stack[] = $data;
		} else {
			$class = $this->elementClass;
			$this->stack[] = new $class( $data );
		}
		$this->top = $this->stack[ count( $this->stack ) - 1 ];
		$this->accum =& $this->top->getAccum();
	}

	function pop() {
		if ( !count( $this->stack ) ) {
			throw new MWException( __METHOD__.': no elements remaining' );
		}
		$temp = array_pop( $this->stack );

		if ( count( $this->stack ) ) {
			$this->top = $this->stack[ count( $this->stack ) - 1 ];
			$this->accum =& $this->top->getAccum();
		} else {
			$this->top = self::$false;
			$this->accum =& $this->rootAccum;
		}
		return $temp;
	}

	function addPart( $s = '' ) {
		$this->top->addPart( $s );
		$this->accum =& $this->top->getAccum();
	}

	function getFlags() {
		if ( !count( $this->stack ) ) {
			return array(
				'findEquals' => false,
				'findPipe' => false,
				'inHeading' => false,
			);
		} else {
			return $this->top->getFlags();
		}
	}
}

/**
 * @ingroup Parser
 */
class PPDStackElement {
	var $open,		        // Opening character (\n for heading)
		$close,     	    // Matching closing character
		$count,             // Number of opening characters found (number of "=" for heading)
		$parts,             // Array of PPDPart objects describing pipe-separated parts.
		$lineStart;         // True if the open char appeared at the start of the input line. Not set for headings.

	var $partClass = 'PPDPart';

	function __construct( $data = array() ) {
		$class = $this->partClass;
		$this->parts = array( new $class );

		foreach ( $data as $name => $value ) {
			$this->$name = $value;
		}
	}

	function &getAccum() {
		return $this->parts[count($this->parts) - 1]->out;
	}

	function addPart( $s = '' ) {
		$class = $this->partClass;
		$this->parts[] = new $class( $s );
	}

	function getCurrentPart() {
		return $this->parts[count($this->parts) - 1];
	}

	function getFlags() {
		$partCount = count( $this->parts );
		$findPipe = $this->open != "\n" && $this->open != '[';
		return array(
			'findPipe' => $findPipe,
			'findEquals' => $findPipe && $partCount > 1 && !isset( $this->parts[$partCount - 1]->eqpos ),
			'inHeading' => $this->open == "\n",
		);
	}

	/**
	 * Get the output string that would result if the close is not found.
	 */
	function breakSyntax( $openingCount = false ) {
		if ( $this->open == "\n" ) {
			$s = $this->parts[0]->out;
		} else {
			if ( $openingCount === false ) {
				$openingCount = $this->count;
			}
			$s = str_repeat( $this->open, $openingCount );
			$first = true;
			foreach ( $this->parts as $part ) {
				if ( $first ) {
					$first = false;
				} else {
					$s .= '|';
				}
				$s .= $part->out;
			}
		}
		return $s;
	}
}

/**
 * @ingroup Parser
 */
class PPDPart {
	var $out; // Output accumulator string

	// Optional member variables:
	//   eqpos        Position of equals sign in output accumulator
	//   commentEnd   Past-the-end input pointer for the last comment encountered
	//   visualEnd    Past-the-end input pointer for the end of the accumulator minus comments

	function __construct( $out = '' ) {
		$this->out = $out;
	}
}

/**
 * An expansion frame, used as a context to expand the result of preprocessToObj()
 * @ingroup Parser
 */
class PPFrame_DOM implements PPFrame {
	var $preprocessor, $parser, $title;
	var $titleCache;

	/**
	 * Hashtable listing templates which are disallowed for expansion in this frame,
	 * having been encountered previously in parent frames.
	 */
	var $loopCheckHash;

	/**
	 * Recursion depth of this frame, top = 0
	 */
	var $depth;


	/**
	 * Construct a new preprocessor frame.
	 * @param Preprocessor $preprocessor The parent preprocessor
	 */
	function __construct( $preprocessor ) {
		$this->preprocessor = $preprocessor;
		$this->parser = $preprocessor->parser;
		$this->title = $this->parser->mTitle;
		$this->titleCache = array( $this->title ? $this->title->getPrefixedDBkey() : false );
		$this->loopCheckHash = array();
		$this->depth = 0;
	}

	/**
	 * Create a new child frame
	 * $args is optionally a multi-root PPNode or array containing the template arguments
	 */
	function newChild( $args = false, $title = false ) {
		$namedArgs = array();
		$numberedArgs = array();
		if ( $title === false ) {
			$title = $this->title;
		}
		if ( $args !== false ) {
			$xpath = false;
			if ( $args instanceof PPNode ) {
				$args = $args->node;
			}
			foreach ( $args as $arg ) {
				if ( !$xpath ) {
					$xpath = new DOMXPath( $arg->ownerDocument );
				}

				$nameNodes = $xpath->query( 'name', $arg );
				$value = $xpath->query( 'value', $arg );
				if ( $nameNodes->item( 0 )->hasAttributes() ) {
					// Numbered parameter
					$index = $nameNodes->item( 0 )->attributes->getNamedItem( 'index' )->textContent;
					$numberedArgs[$index] = $value->item( 0 );
					unset( $namedArgs[$index] );
				} else {
					// Named parameter
					$name = trim( $this->expand( $nameNodes->item( 0 ), PPFrame::STRIP_COMMENTS ) );
					$namedArgs[$name] = $value->item( 0 );
					unset( $numberedArgs[$name] );
				}
			}
		}
		return new PPTemplateFrame_DOM( $this->preprocessor, $this, $numberedArgs, $namedArgs, $title );
	}

	function expand( $root, $flags = 0 ) {
		static $depth = 0;
		if ( is_string( $root ) ) {
			return $root;
		}

		if ( ++$this->parser->mPPNodeCount > $this->parser->mOptions->mMaxPPNodeCount )
		{
			return '<span class="error">Node-count limit exceeded</span>';
		}

		if ( $depth > $this->parser->mOptions->mMaxPPExpandDepth ) {
			return '<span class="error">Expansion depth limit exceeded</span>';
		}
		++$depth;

		if ( $root instanceof PPNode_DOM ) {
			$root = $root->node;
		}
		if ( $root instanceof DOMDocument ) {
			$root = $root->documentElement;
		}

		$outStack = array( '', '' );
		$iteratorStack = array( false, $root );
		$indexStack = array( 0, 0 );

		while ( count( $iteratorStack ) > 1 ) {
			$level = count( $outStack ) - 1;
			$iteratorNode =& $iteratorStack[ $level ];
			$out =& $outStack[$level];
			$index =& $indexStack[$level];

			if ( $iteratorNode instanceof PPNode_DOM ) $iteratorNode = $iteratorNode->node;

			if ( is_array( $iteratorNode ) ) {
				if ( $index >= count( $iteratorNode ) ) {
					// All done with this iterator
					$iteratorStack[$level] = false;
					$contextNode = false;
				} else {
					$contextNode = $iteratorNode[$index];
					$index++;
				}
			} elseif ( $iteratorNode instanceof DOMNodeList ) {
				if ( $index >= $iteratorNode->length ) {
					// All done with this iterator
					$iteratorStack[$level] = false;
					$contextNode = false;
				} else {
					$contextNode = $iteratorNode->item( $index );
					$index++;
				}
			} else {
				// Copy to $contextNode and then delete from iterator stack,
				// because this is not an iterator but we do have to execute it once
				$contextNode = $iteratorStack[$level];
				$iteratorStack[$level] = false;
			}

			if ( $contextNode instanceof PPNode_DOM ) $contextNode = $contextNode->node;

			$newIterator = false;

			if ( $contextNode === false ) {
				// nothing to do
			} elseif ( is_string( $contextNode ) ) {
				$out .= $contextNode;
			} elseif ( is_array( $contextNode ) || $contextNode instanceof DOMNodeList ) {
				$newIterator = $contextNode;
			} elseif ( $contextNode instanceof DOMNode ) {
				if ( $contextNode->nodeType == XML_TEXT_NODE ) {
					$out .= $contextNode->nodeValue;
				} elseif ( $contextNode->nodeName == 'template' ) {
					# Double-brace expansion
					$xpath = new DOMXPath( $contextNode->ownerDocument );
					$titles = $xpath->query( 'title', $contextNode );
					$title = $titles->item( 0 );
					$parts = $xpath->query( 'part', $contextNode );
					if ( $flags & self::NO_TEMPLATES ) {
						$newIterator = $this->virtualBracketedImplode( '{{', '|', '}}', $title, $parts );
					} else {
						$lineStart = $contextNode->getAttribute( 'lineStart' );
						$params = array(
							'title' => new PPNode_DOM( $title ),
							'parts' => new PPNode_DOM( $parts ),
							'lineStart' => $lineStart );
						$ret = $this->parser->braceSubstitution( $params, $this );
						if ( isset( $ret['object'] ) ) {
							$newIterator = $ret['object'];
						} else {
							$out .= $ret['text'];
						}
					}
				} elseif ( $contextNode->nodeName == 'tplarg' ) {
					# Triple-brace expansion
					$xpath = new DOMXPath( $contextNode->ownerDocument );
					$titles = $xpath->query( 'title', $contextNode );
					$title = $titles->item( 0 );
					$parts = $xpath->query( 'part', $contextNode );
					if ( $flags & self::NO_ARGS ) {
						$newIterator = $this->virtualBracketedImplode( '{{{', '|', '}}}', $title, $parts );
					} else {
						$params = array(
							'title' => new PPNode_DOM( $title ),
							'parts' => new PPNode_DOM( $parts ) );
						$ret = $this->parser->argSubstitution( $params, $this );
						if ( isset( $ret['object'] ) ) {
							$newIterator = $ret['object'];
						} else {
							$out .= $ret['text'];
						}
					}
				} elseif ( $contextNode->nodeName == 'comment' ) {
					# HTML-style comment
					# Remove it in HTML, pre+remove and STRIP_COMMENTS modes
					if ( $this->parser->ot['html']
						|| ( $this->parser->ot['pre'] && $this->parser->mOptions->getRemoveComments() )
						|| ( $flags & self::STRIP_COMMENTS ) )
					{
						$out .= '';
					}
					# Add a strip marker in PST mode so that pstPass2() can run some old-fashioned regexes on the result
					# Not in RECOVER_COMMENTS mode (extractSections) though
					elseif ( $this->parser->ot['wiki'] && ! ( $flags & self::RECOVER_COMMENTS ) ) {
						$out .= $this->parser->insertStripItem( $contextNode->textContent );
					}
					# Recover the literal comment in RECOVER_COMMENTS and pre+no-remove
					else {
						$out .= $contextNode->textContent;
					}
				} elseif ( $contextNode->nodeName == 'ignore' ) {
					# Output suppression used by <includeonly> etc.
					# OT_WIKI will only respect <ignore> in substed templates.
					# The other output types respect it unless NO_IGNORE is set.
					# extractSections() sets NO_IGNORE and so never respects it.
					if ( ( !isset( $this->parent ) && $this->parser->ot['wiki'] ) || ( $flags & self::NO_IGNORE ) ) {
						$out .= $contextNode->textContent;
					} else {
						$out .= '';
					}
				} elseif ( $contextNode->nodeName == 'ext' ) {
					# Extension tag
					$xpath = new DOMXPath( $contextNode->ownerDocument );
					$names = $xpath->query( 'name', $contextNode );
					$attrs = $xpath->query( 'attr', $contextNode );
					$inners = $xpath->query( 'inner', $contextNode );
					$closes = $xpath->query( 'close', $contextNode );
					$params = array(
						'name' => new PPNode_DOM( $names->item( 0 ) ),
						'attr' => $attrs->length > 0 ? new PPNode_DOM( $attrs->item( 0 ) ) : null,
						'inner' => $inners->length > 0 ? new PPNode_DOM( $inners->item( 0 ) ) : null,
						'close' => $closes->length > 0 ? new PPNode_DOM( $closes->item( 0 ) ) : null,
					);
					$out .= $this->parser->extensionSubstitution( $params, $this );
				} elseif ( $contextNode->nodeName == 'h' ) {
					# Heading
					$s = $this->expand( $contextNode->childNodes, $flags );

                    # Insert a heading marker only for <h> children of <root>
                    # This is to stop extractSections from going over multiple tree levels
                    if ( $contextNode->parentNode->nodeName == 'root'
                      && $this->parser->ot['html'] )
                    {
						# Insert heading index marker
						$headingIndex = $contextNode->getAttribute( 'i' );
						$titleText = $this->title->getPrefixedDBkey();
						$this->parser->mHeadings[] = array( $titleText, $headingIndex );
						$serial = count( $this->parser->mHeadings ) - 1;
						$marker = "{$this->parser->mUniqPrefix}-h-$serial-" . Parser::MARKER_SUFFIX;
						$count = $contextNode->getAttribute( 'level' );
						$s = substr( $s, 0, $count ) . $marker . substr( $s, $count );
						$this->parser->mStripState->general->setPair( $marker, '' );
					}
					$out .= $s;
				} else {
					# Generic recursive expansion
					$newIterator = $contextNode->childNodes;
				}
			} else {
				throw new MWException( __METHOD__.': Invalid parameter type' );
			}

			if ( $newIterator !== false ) {
				if ( $newIterator instanceof PPNode_DOM ) {
					$newIterator = $newIterator->node;
				}
				$outStack[] = '';
				$iteratorStack[] = $newIterator;
				$indexStack[] = 0;
			} elseif ( $iteratorStack[$level] === false ) {
				// Return accumulated value to parent
				// With tail recursion
				while ( $iteratorStack[$level] === false && $level > 0 ) {
					$outStack[$level - 1] .= $out;
					array_pop( $outStack );
					array_pop( $iteratorStack );
					array_pop( $indexStack );
					$level--;
				}
			}
		}
		--$depth;
		return $outStack[0];
	}

	function implodeWithFlags( $sep, $flags /*, ... */ ) {
		$args = array_slice( func_get_args(), 2 );

		$first = true;
		$s = '';
		foreach ( $args as $root ) {
			if ( $root instanceof PPNode_DOM ) $root = $root->node;
			if ( !is_array( $root ) && !( $root instanceof DOMNodeList ) ) {
				$root = array( $root );
			}
			foreach ( $root as $node ) {
				if ( $first ) {
					$first = false;
				} else {
					$s .= $sep;
				}
				$s .= $this->expand( $node, $flags );
			}
		}
		return $s;
	}

	/**
	 * Implode with no flags specified
	 * This previously called implodeWithFlags but has now been inlined to reduce stack depth
	 */
	function implode( $sep /*, ... */ ) {
		$args = array_slice( func_get_args(), 1 );

		$first = true;
		$s = '';
		foreach ( $args as $root ) {
			if ( $root instanceof PPNode_DOM ) $root = $root->node;
			if ( !is_array( $root ) && !( $root instanceof DOMNodeList ) ) {
				$root = array( $root );
			}
			foreach ( $root as $node ) {
				if ( $first ) {
					$first = false;
				} else {
					$s .= $sep;
				}
				$s .= $this->expand( $node );
			}
		}
		return $s;
	}

	/**
	 * Makes an object that, when expand()ed, will be the same as one obtained
	 * with implode()
	 */
	function virtualImplode( $sep /*, ... */ ) {
		$args = array_slice( func_get_args(), 1 );
		$out = array();
		$first = true;
		if ( $root instanceof PPNode_DOM ) $root = $root->node;

		foreach ( $args as $root ) {
			if ( !is_array( $root ) && !( $root instanceof DOMNodeList ) ) {
				$root = array( $root );
			}
			foreach ( $root as $node ) {
				if ( $first ) {
					$first = false;
				} else {
					$out[] = $sep;
				}
				$out[] = $node;
			}
		}
		return $out;
	}

	/**
	 * Virtual implode with brackets
	 */
	function virtualBracketedImplode( $start, $sep, $end /*, ... */ ) {
		$args = array_slice( func_get_args(), 3 );
		$out = array( $start );
		$first = true;

		foreach ( $args as $root ) {
			if ( $root instanceof PPNode_DOM ) $root = $root->node;
			if ( !is_array( $root ) && !( $root instanceof DOMNodeList ) ) {
				$root = array( $root );
			}
			foreach ( $root as $node ) {
				if ( $first ) {
					$first = false;
				} else {
					$out[] = $sep;
				}
				$out[] = $node;
			}
		}
		$out[] = $end;
		return $out;
	}

	function __toString() {
		return 'frame{}';
	}

	function getPDBK( $level = false ) {
		if ( $level === false ) {
			return $this->title->getPrefixedDBkey();
		} else {
			return isset( $this->titleCache[$level] ) ? $this->titleCache[$level] : false;
		}
	}

	/**
	 * Returns true if there are no arguments in this frame
	 */
	function isEmpty() {
		return true;
	}

	function getArgument( $name ) {
		return false;
	}

	/**
	 * Returns true if the infinite loop check is OK, false if a loop is detected
	 */
	function loopCheck( $title ) {
		return !isset( $this->loopCheckHash[$title->getPrefixedDBkey()] );
	}

	/**
	 * Return true if the frame is a template frame
	 */
	function isTemplate() {
		return false;
	}
}

/**
 * Expansion frame with template arguments
 * @ingroup Parser
 */
class PPTemplateFrame_DOM extends PPFrame_DOM {
	var $numberedArgs, $namedArgs, $parent;
	var $numberedExpansionCache, $namedExpansionCache;

	function __construct( $preprocessor, $parent = false, $numberedArgs = array(), $namedArgs = array(), $title = false ) {
		$this->preprocessor = $preprocessor;
		$this->parser = $preprocessor->parser;
		$this->parent = $parent;
		$this->numberedArgs = $numberedArgs;
		$this->namedArgs = $namedArgs;
		$this->title = $title;
		$pdbk = $title ? $title->getPrefixedDBkey() : false;
		$this->titleCache = $parent->titleCache;
		$this->titleCache[] = $pdbk;
		$this->loopCheckHash = /*clone*/ $parent->loopCheckHash;
		if ( $pdbk !== false ) {
			$this->loopCheckHash[$pdbk] = true;
		}
		$this->depth = $parent->depth + 1;
		$this->numberedExpansionCache = $this->namedExpansionCache = array();
	}

	function __toString() {
		$s = 'tplframe{';
		$first = true;
		$args = $this->numberedArgs + $this->namedArgs;
		foreach ( $args as $name => $value ) {
			if ( $first ) {
				$first = false;
			} else {
				$s .= ', ';
			}
			$s .= "\"$name\":\"" .
				str_replace( '"', '\\"', $value->ownerDocument->saveXML( $value ) ) . '"';
		}
		$s .= '}';
		return $s;
	}
	/**
	 * Returns true if there are no arguments in this frame
	 */
	function isEmpty() {
		return !count( $this->numberedArgs ) && !count( $this->namedArgs );
	}

	function getNumberedArgument( $index ) {
		if ( !isset( $this->numberedArgs[$index] ) ) {
			return false;
		}
		if ( !isset( $this->numberedExpansionCache[$index] ) ) {
			# No trimming for unnamed arguments
			$this->numberedExpansionCache[$index] = $this->parent->expand( $this->numberedArgs[$index], self::STRIP_COMMENTS );
		}
		return $this->numberedExpansionCache[$index];
	}

	function getNamedArgument( $name ) {
		if ( !isset( $this->namedArgs[$name] ) ) {
			return false;
		}
		if ( !isset( $this->namedExpansionCache[$name] ) ) {
			# Trim named arguments post-expand, for backwards compatibility
			$this->namedExpansionCache[$name] = trim(
				$this->parent->expand( $this->namedArgs[$name], self::STRIP_COMMENTS ) );
		}
		return $this->namedExpansionCache[$name];
	}

	function getArgument( $name ) {
		$text = $this->getNumberedArgument( $name );
		if ( $text === false ) {
			$text = $this->getNamedArgument( $name );
		}
		return $text;
	}

	/**
	 * Return true if the frame is a template frame
	 */
	function isTemplate() {
		return true;
	}
}

/**
 * Expansion frame with custom arguments
 * @ingroup Parser
 */
class PPCustomFrame_DOM extends PPFrame_DOM {
	var $args;

	function __construct( $preprocessor, $args ) {
		$this->preprocessor = $preprocessor;
		$this->parser = $preprocessor->parser;
		$this->args = $args;
	}

	function __toString() {
		$s = 'cstmframe{';
		$first = true;
		foreach ( $this->args as $name => $value ) {
			if ( $first ) {
				$first = false;
			} else {
				$s .= ', ';
			}
			$s .= "\"$name\":\"" .
				str_replace( '"', '\\"', $value->__toString() ) . '"';
		}
		$s .= '}';
		return $s;
	}

	function isEmpty() {
		return !count( $this->args );
	}

	function getArgument( $index ) {
		return $this->args[$index];
	}
}

/**
 * @ingroup Parser
 */
class PPNode_DOM implements PPNode {
	var $node;

	function __construct( $node, $xpath = false ) {
		$this->node = $node;
	}

	function __get( $name ) {
		if ( $name == 'xpath' ) {
			$this->xpath = new DOMXPath( $this->node->ownerDocument );
		}
		return $this->xpath;
	}

	function __toString() {
		if ( $this->node instanceof DOMNodeList ) {
			$s = '';
			foreach ( $this->node as $node ) {
				$s .= $node->ownerDocument->saveXML( $node );
			}
		} else {
			$s = $this->node->ownerDocument->saveXML( $this->node );
		}
		return $s;
	}

	function getChildren() {
		return $this->node->childNodes ? new self( $this->node->childNodes ) : false;
	}

	function getFirstChild() {
		return $this->node->firstChild ? new self( $this->node->firstChild ) : false;
	}

	function getNextSibling() {
		return $this->node->nextSibling ? new self( $this->node->nextSibling ) : false;
	}

	function getChildrenOfType( $type ) {
		return new self( $this->xpath->query( $type, $this->node ) );
	}

	function getLength() {
		if ( $this->node instanceof DOMNodeList ) {
			return $this->node->length;
		} else {
			return false;
		}
	}

	function item( $i ) {
		$item = $this->node->item( $i );
		return $item ? new self( $item ) : false;
	}

	function getName() {
		if ( $this->node instanceof DOMNodeList ) {
			return '#nodelist';
		} else {
			return $this->node->nodeName;
		}
	}

	/**
	 * Split a <part> node into an associative array containing:
	 *    name          PPNode name
	 *    index         String index
	 *    value         PPNode value
	 */
	function splitArg() {
		$names = $this->xpath->query( 'name', $this->node );
		$values = $this->xpath->query( 'value', $this->node );
		if ( !$names->length || !$values->length ) {
			throw new MWException( 'Invalid brace node passed to ' . __METHOD__ );
		}
		$name = $names->item( 0 );
		$index = $name->getAttribute( 'index' );
		return array(
			'name' => new self( $name ),
			'index' => $index,
			'value' => new self( $values->item( 0 ) ) );
	}

	/**
	 * Split an <ext> node into an associative array containing name, attr, inner and close
	 * All values in the resulting array are PPNodes. Inner and close are optional.
	 */
	function splitExt() {
		$names = $this->xpath->query( 'name', $this->node );
		$attrs = $this->xpath->query( 'attr', $this->node );
		$inners = $this->xpath->query( 'inner', $this->node );
		$closes = $this->xpath->query( 'close', $this->node );
		if ( !$names->length || !$attrs->length ) {
			throw new MWException( 'Invalid ext node passed to ' . __METHOD__ );
		}
		$parts = array(
			'name' => new self( $names->item( 0 ) ),
			'attr' => new self( $attrs->item( 0 ) ) );
		if ( $inners->length ) {
			$parts['inner'] = new self( $inners->item( 0 ) );
		}
		if ( $closes->length ) {
			$parts['close'] = new self( $closes->item( 0 ) );
		}
		return $parts;
	}

	/**
	 * Split a <h> node
	 */
	function splitHeading() {
		if ( !$this->nodeName == 'h' ) {
			throw new MWException( 'Invalid h node passed to ' . __METHOD__ );
		}
		return array(
			'i' => $this->node->getAttribute( 'i' ),
			'level' => $this->node->getAttribute( 'level' ),
			'contents' => $this->getChildren()
		);
	}
}
