<?php
/**
 * Contain everything related to <math> </math> parsing
 * @file
 * @ingroup Parser
 */

/**
 * Takes LaTeX fragments, sends them to a helper program (texvc) for rendering
 * to rasterized PNG and HTML and MathML approximations. An appropriate
 * rendering form is picked and returned.
 *
 * @author Tomasz Wegrzanowski, with additions by Brion Vibber (2003, 2004)
 * @ingroup Parser
 */
class MathRenderer {
	var $mode = MW_MATH_MODERN;
	var $tex = '';
	var $inputhash = '';
	var $hash = '';
	var $html = '';
	var $mathml = '';
	var $conservativeness = 0;

	function __construct( $tex, $params=array() ) {
		$this->tex = $tex;
		$this->params = $params;
 	}

	function setOutputMode( $mode ) {
		$this->mode = $mode;
	}

	function render() {
		global $wgTmpDirectory, $wgInputEncoding;
		global $wgTexvc;
		$fname = 'MathRenderer::render';

		if( $this->mode == MW_MATH_SOURCE ) {
			# No need to render or parse anything more!
			return ('$ '.htmlspecialchars( $this->tex ).' $');
		}
		if( $this->tex == '' ) {
			return; # bug 8372
		}

		if( !$this->_recall() ) {
			# Ensure that the temp and output directories are available before continuing...
			if( !file_exists( $wgTmpDirectory ) ) {
				if( !@mkdir( $wgTmpDirectory ) ) {
					return $this->_error( 'math_bad_tmpdir' );
				}
			} elseif( !is_dir( $wgTmpDirectory ) || !is_writable( $wgTmpDirectory ) ) {
				return $this->_error( 'math_bad_tmpdir' );
			}

			if( function_exists( 'is_executable' ) && !is_executable( $wgTexvc ) ) {
				return $this->_error( 'math_notexvc' );
			}
			$cmd = $wgTexvc . ' ' .
					escapeshellarg( $wgTmpDirectory ).' '.
					escapeshellarg( $wgTmpDirectory ).' '.
					escapeshellarg( $this->tex ).' '.
					escapeshellarg( $wgInputEncoding );

			if ( wfIsWindows() ) {
				# Invoke it within cygwin sh, because texvc expects sh features in its default shell
				$cmd = 'sh -c ' . wfEscapeShellArg( $cmd );
			}

			wfDebug( "TeX: $cmd\n" );
			$contents = `$cmd`;
			wfDebug( "TeX output:\n $contents\n---\n" );

			if (strlen($contents) == 0) {
				return $this->_error( 'math_unknown_error' );
			}

			$retval = substr ($contents, 0, 1);
			$errmsg = '';
			if (($retval == 'C') || ($retval == 'M') || ($retval == 'L')) {
				if ($retval == 'C') {
					$this->conservativeness = 2;
				} else if ($retval == 'M') {
					$this->conservativeness = 1;
				} else {
					$this->conservativeness = 0;
				}
				$outdata = substr ($contents, 33);

				$i = strpos($outdata, "\000");

				$this->html = substr($outdata, 0, $i);
				$this->mathml = substr($outdata, $i+1);
			} else if (($retval == 'c') || ($retval == 'm') || ($retval == 'l'))  {
				$this->html = substr ($contents, 33);
				if ($retval == 'c') {
					$this->conservativeness = 2;
				} else if ($retval == 'm') {
					$this->conservativeness = 1;
				} else {
					$this->conservativeness = 0;
				}
				$this->mathml = NULL;
			} else if ($retval == 'X') {
				$this->html = NULL;
				$this->mathml = substr ($contents, 33);
				$this->conservativeness = 0;
			} else if ($retval == '+') {
				$this->html = NULL;
				$this->mathml = NULL;
				$this->conservativeness = 0;
			} else {
				$errbit = htmlspecialchars( substr($contents, 1) );
				switch( $retval ) {
					case 'E':
						$errmsg = $this->_error( 'math_lexing_error', $errbit );
						break;
					case 'S':
						$errmsg = $this->_error( 'math_syntax_error', $errbit );
						break;
					case 'F':
						$errmsg = $this->_error( 'math_unknown_function', $errbit );
						break;
					default:
						$errmsg = $this->_error( 'math_unknown_error', $errbit );
				}
			}

			if ( !$errmsg ) {
				 $this->hash = substr ($contents, 1, 32);
			}

			wfRunHooks( 'MathAfterTexvc', array( &$this, &$errmsg ) );

			if ( $errmsg ) {
				 return $errmsg;
			}

			if (!preg_match("/^[a-f0-9]{32}$/", $this->hash)) {
				return $this->_error( 'math_unknown_error' );
			}

			if( !file_exists( "$wgTmpDirectory/{$this->hash}.png" ) ) {
				return $this->_error( 'math_image_error' );
			}

			$hashpath = $this->_getHashPath();
			if( !file_exists( $hashpath ) ) {
				if( !@wfMkdirParents( $hashpath, 0755 ) ) {
					return $this->_error( 'math_bad_output' );
				}
			} elseif( !is_dir( $hashpath ) || !is_writable( $hashpath ) ) {
				return $this->_error( 'math_bad_output' );
			}

			if( !rename( "$wgTmpDirectory/{$this->hash}.png", "$hashpath/{$this->hash}.png" ) ) {
				return $this->_error( 'math_output_error' );
			}

			# Now save it back to the DB:
			if ( !wfReadOnly() ) {
				$outmd5_sql = pack('H32', $this->hash);

				$md5_sql = pack('H32', $this->md5); # Binary packed, not hex

				$dbw = wfGetDB( DB_MASTER );
				$dbw->replace( 'math', array( 'math_inputhash' ),
				  array(
					'math_inputhash' => $dbw->encodeBlob($md5_sql),
					'math_outputhash' => $dbw->encodeBlob($outmd5_sql),
					'math_html_conservativeness' => $this->conservativeness,
					'math_html' => $this->html,
					'math_mathml' => $this->mathml,
				  ), $fname, array( 'IGNORE' )
				);
			}

		}

		return $this->_doRender();
	}

	function _error( $msg, $append = '' ) {
		$mf   = htmlspecialchars( wfMsg( 'math_failure' ) );
		$errmsg = htmlspecialchars( wfMsg( $msg ) );
		$source = htmlspecialchars( str_replace( "\n", ' ', $this->tex ) );
		return "<strong class='error'>$mf ($errmsg$append): $source</strong>\n";
	}

	function _recall() {
		global $wgMathDirectory;
		$fname = 'MathRenderer::_recall';

		$this->md5 = md5( $this->tex );
		$dbr = wfGetDB( DB_SLAVE );
		$rpage = $dbr->selectRow( 'math',
			array( 'math_outputhash','math_html_conservativeness','math_html','math_mathml' ),
			array( 'math_inputhash' => $dbr->encodeBlob(pack("H32", $this->md5))), # Binary packed, not hex
			$fname
		);

		if( $rpage !== false ) {
			# Tailing 0x20s can get dropped by the database, add it back on if necessary:
			$xhash = unpack( 'H32md5', $dbr->decodeBlob($rpage->math_outputhash) . "                " );
			$this->hash = $xhash ['md5'];

			$this->conservativeness = $rpage->math_html_conservativeness;
			$this->html = $rpage->math_html;
			$this->mathml = $rpage->math_mathml;

			if( file_exists( $this->_getHashPath() . "/{$this->hash}.png" ) ) {
				return true;
			}

			if( file_exists( $wgMathDirectory . "/{$this->hash}.png" ) ) {
				$hashpath = $this->_getHashPath();

				if( !file_exists( $hashpath ) ) {
					if( !@wfMkdirParents( $hashpath, 0755 ) ) {
						return false;
					}
				} elseif( !is_dir( $hashpath ) || !is_writable( $hashpath ) ) {
					return false;
				}
				if ( function_exists( "link" ) ) {
					return link ( $wgMathDirectory . "/{$this->hash}.png",
							$hashpath . "/{$this->hash}.png" );
				} else {
					return rename ( $wgMathDirectory . "/{$this->hash}.png",
							$hashpath . "/{$this->hash}.png" );
				}
			}

		}

		# Missing from the database and/or the render cache
		return false;
	}

	/**
	 * Select among PNG, HTML, or MathML output depending on
	 */
	function _doRender() {
		if( $this->mode == MW_MATH_MATHML && $this->mathml != '' ) {
			return Xml::tags( 'math',
				$this->_attribs( 'math',
					array( 'xmlns' => 'http://www.w3.org/1998/Math/MathML' ) ),
				$this->mathml );
		}
		if (($this->mode == MW_MATH_PNG) || ($this->html == '') ||
		   (($this->mode == MW_MATH_SIMPLE) && ($this->conservativeness != 2)) ||
		   (($this->mode == MW_MATH_MODERN || $this->mode == MW_MATH_MATHML) && ($this->conservativeness == 0))) {
			return $this->_linkToMathImage();
		} else {
			return Xml::tags( 'span',
				$this->_attribs( 'span',
					array( 'class' => 'texhtml' ) ),
				$this->html );
		}
	}

	function _attribs( $tag, $defaults=array(), $overrides=array() ) {
		$attribs = Sanitizer::validateTagAttributes( $this->params, $tag );
		$attribs = Sanitizer::mergeAttributes( $defaults, $attribs );
		$attribs = Sanitizer::mergeAttributes( $attribs, $overrides );
		return $attribs;
	}

	function _linkToMathImage() {
		global $wgMathPath;
		$url = "$wgMathPath/" . substr($this->hash, 0, 1)
					.'/'. substr($this->hash, 1, 1) .'/'. substr($this->hash, 2, 1)
					. "/{$this->hash}.png";

		return Xml::element( 'img',
			$this->_attribs(
				'img',
				array(
					'class' => 'tex',
					'alt' => $this->tex ),
				array(
					'src' => $url ) ) );
	}

	function _getHashPath() {
		global $wgMathDirectory;
		$path = $wgMathDirectory .'/'. substr($this->hash, 0, 1)
					.'/'. substr($this->hash, 1, 1)
					.'/'. substr($this->hash, 2, 1);
		wfDebug( "TeX: getHashPath, hash is: $this->hash, path is: $path\n" );
		return $path;
	}

	public static function renderMath( $tex, $params=array() ) {
		global $wgUser;
		$math = new MathRenderer( $tex, $params );
		$math->setOutputMode( $wgUser->getOption('math'));
		return $math->render();
	}
}
