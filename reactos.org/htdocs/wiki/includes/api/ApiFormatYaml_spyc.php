<?php
  /**
   * Spyc -- A Simple PHP YAML Class
   * @version 0.2.3 -- 2006-02-04
   * @author Chris Wanstrath <chris@ozmm.org>
   * @see http://spyc.sourceforge.net/
   * @copyright Copyright 2005-2006 Chris Wanstrath
   * @license http://www.opensource.org/licenses/mit-license.php MIT License
   */

  /**
   * A node, used by Spyc for parsing YAML.
   * @ingroup API
   */
  class YAMLNode {
    /**#@+
     * @access public
     * @var string
     */
    var $parent;
    var $id;
    /**#@-*/
    /**
     * @access public
     * @var mixed
     */
    var $data;
    /**
     * @access public
     * @var int
     */
    var $indent;
    /**
     * @access public
     * @var bool
     */
    var $children = false;

    /**
     * The constructor assigns the node a unique ID.
     * @access public
     * @return void
     */
    function YAMLNode() {
      $this->id = uniqid('');
    }
  }

  /**
   * The Simple PHP YAML Class.
   *
   * This class can be used to read a YAML file and convert its contents
   * into a PHP array.  It currently supports a very limited subsection of
   * the YAML spec.
   *
   * Usage:
   * <code>
   *   $parser = new Spyc;
   *   $array  = $parser->load($file);
   * </code>
   * @ingroup API
   */
  class Spyc {

    /**
     * Load YAML into a PHP array statically
     *
     * The load method, when supplied with a YAML stream (string or file),
     * will do its best to convert YAML in a file into a PHP array.  Pretty
     * simple.
     *  Usage:
     *  <code>
     *   $array = Spyc::YAMLLoad('lucky.yml');
     *   print_r($array);
     *  </code>
     * @access public
     * @return array
     * @param string $input Path of YAML file or string containing YAML
     */
    function YAMLLoad($input) {
      $spyc = new Spyc;
      return $spyc->load($input);
    }

    /**
     * Dump YAML from PHP array statically
     *
     * The dump method, when supplied with an array, will do its best
     * to convert the array into friendly YAML.  Pretty simple.  Feel free to
     * save the returned string as nothing.yml and pass it around.
     *
     * Oh, and you can decide how big the indent is and what the wordwrap
     * for folding is.  Pretty cool -- just pass in 'false' for either if
     * you want to use the default.
     *
     * Indent's default is 2 spaces, wordwrap's default is 40 characters.  And
     * you can turn off wordwrap by passing in 0.
     *
     * @access public
     * @static
     * @return string
     * @param array $array PHP array
     * @param int $indent Pass in false to use the default, which is 2
     * @param int $wordwrap Pass in 0 for no wordwrap, false for default (40)
     */
    public static function YAMLDump($array,$indent = false,$wordwrap = false) {
      $spyc = new Spyc;
      return $spyc->dump($array,$indent,$wordwrap);
    }

    /**
     * Load YAML into a PHP array from an instantiated object
     *
     * The load method, when supplied with a YAML stream (string or file path),
     * will do its best to convert the YAML into a PHP array.  Pretty simple.
     *  Usage:
     *  <code>
     *   $parser = new Spyc;
     *   $array  = $parser->load('lucky.yml');
     *   print_r($array);
     *  </code>
     * @access public
     * @return array
     * @param string $input Path of YAML file or string containing YAML
     */
    function load($input) {
      // See what type of input we're talking about
      // If it's not a file, assume it's a string
      if (!empty($input) && (strpos($input, "\n") === false)
          && file_exists($input)) {
        $yaml = file($input);
      } else {
        $yaml = explode("\n",$input);
      }
      // Initiate some objects and values
      $base              = new YAMLNode;
      $base->indent      = 0;
      $this->_lastIndent = 0;
      $this->_lastNode   = $base->id;
      $this->_inBlock    = false;
      $this->_isInline   = false;

      foreach ($yaml as $linenum => $line) {
        $ifchk = trim($line);

        // If the line starts with a tab (instead of a space), throw a fit.
        if (preg_match('/^(\t)+(\w+)/', $line)) {
          $err = 'ERROR: Line '. ($linenum + 1) .' in your input YAML begins'.
                 ' with a tab.  YAML only recognizes spaces.  Please reformat.';
          die($err);
        }

        if ($this->_inBlock === false && empty($ifchk)) {
          continue;
        } elseif ($this->_inBlock == true && empty($ifchk)) {
          $last =& $this->_allNodes[$this->_lastNode];
          $last->data[key($last->data)] .= "\n";
        } elseif ($ifchk{0} != '#' && substr($ifchk,0,3) != '---') {
          // Create a new node and get its indent
          $node         = new YAMLNode;
          $node->indent = $this->_getIndent($line);

          // Check where the node lies in the hierarchy
          if ($this->_lastIndent == $node->indent) {
            // If we're in a block, add the text to the parent's data
            if ($this->_inBlock === true) {
              $parent =& $this->_allNodes[$this->_lastNode];
              $parent->data[key($parent->data)] .= trim($line).$this->_blockEnd;
            } else {
              // The current node's parent is the same as the previous node's
              if (isset($this->_allNodes[$this->_lastNode])) {
                $node->parent = $this->_allNodes[$this->_lastNode]->parent;
              }
            }
          } elseif ($this->_lastIndent < $node->indent) {
            if ($this->_inBlock === true) {
              $parent =& $this->_allNodes[$this->_lastNode];
              $parent->data[key($parent->data)] .= trim($line).$this->_blockEnd;
            } elseif ($this->_inBlock === false) {
              // The current node's parent is the previous node
              $node->parent = $this->_lastNode;

              // If the value of the last node's data was > or | we need to
              // start blocking i.e. taking in all lines as a text value until
              // we drop our indent.
              $parent =& $this->_allNodes[$node->parent];
              $this->_allNodes[$node->parent]->children = true;
              if (is_array($parent->data)) {
                $chk = $parent->data[key($parent->data)];
                if ($chk === '>') {
                  $this->_inBlock  = true;
                  $this->_blockEnd = ' ';
                  $parent->data[key($parent->data)] =
                        str_replace('>','',$parent->data[key($parent->data)]);
                  $parent->data[key($parent->data)] .= trim($line).' ';
                  $this->_allNodes[$node->parent]->children = false;
                  $this->_lastIndent = $node->indent;
                } elseif ($chk === '|') {
                  $this->_inBlock  = true;
                  $this->_blockEnd = "\n";
                  $parent->data[key($parent->data)] =
                        str_replace('|','',$parent->data[key($parent->data)]);
                  $parent->data[key($parent->data)] .= trim($line)."\n";
                  $this->_allNodes[$node->parent]->children = false;
                  $this->_lastIndent = $node->indent;
                }
              }
            }
          } elseif ($this->_lastIndent > $node->indent) {
            // Any block we had going is dead now
            if ($this->_inBlock === true) {
              $this->_inBlock = false;
              if ($this->_blockEnd = "\n") {
                $last =& $this->_allNodes[$this->_lastNode];
                $last->data[key($last->data)] =
                      trim($last->data[key($last->data)]);
              }
            }

            // We don't know the parent of the node so we have to find it
            // foreach ($this->_allNodes as $n) {
            foreach ($this->_indentSort[$node->indent] as $n) {
              if ($n->indent == $node->indent) {
                $node->parent = $n->parent;
              }
            }
          }

          if ($this->_inBlock === false) {
            // Set these properties with information from our current node
            $this->_lastIndent = $node->indent;
            // Set the last node
            $this->_lastNode = $node->id;
            // Parse the YAML line and return its data
            $node->data = $this->_parseLine($line);
            // Add the node to the master list
            $this->_allNodes[$node->id] = $node;
            // Add a reference to the node in an indent array
            $this->_indentSort[$node->indent][] =& $this->_allNodes[$node->id];
            // Add a reference to the node in a References array if this node
            // has a YAML reference in it.
            if (
              ( (is_array($node->data)) &&
                isset($node->data[key($node->data)]) &&
                (!is_array($node->data[key($node->data)])) )
              &&
              ( (preg_match('/^&([^ ]+)/',$node->data[key($node->data)]))
                ||
                (preg_match('/^\*([^ ]+)/',$node->data[key($node->data)])) )
            ) {
                $this->_haveRefs[] =& $this->_allNodes[$node->id];
            } elseif (
              ( (is_array($node->data)) &&
                isset($node->data[key($node->data)]) &&
                 (is_array($node->data[key($node->data)])) )
            ) {
              // Incomplete reference making code.  Ugly, needs cleaned up.
              foreach ($node->data[key($node->data)] as $d) {
                if ( !is_array($d) &&
                  ( (preg_match('/^&([^ ]+)/',$d))
                    ||
                    (preg_match('/^\*([^ ]+)/',$d)) )
                  ) {
                    $this->_haveRefs[] =& $this->_allNodes[$node->id];
                }
              }
            }
          }
        }
      }
      unset($node);

      // Here we travel through node-space and pick out references (& and *)
      $this->_linkReferences();

      // Build the PHP array out of node-space
      $trunk = $this->_buildArray();
      return $trunk;
    }

    /**
     * Dump PHP array to YAML
     *
     * The dump method, when supplied with an array, will do its best
     * to convert the array into friendly YAML.  Pretty simple.  Feel free to
     * save the returned string as tasteful.yml and pass it around.
     *
     * Oh, and you can decide how big the indent is and what the wordwrap
     * for folding is.  Pretty cool -- just pass in 'false' for either if
     * you want to use the default.
     *
     * Indent's default is 2 spaces, wordwrap's default is 40 characters.  And
     * you can turn off wordwrap by passing in 0.
     *
     * @access public
     * @return string
     * @param array $array PHP array
     * @param int $indent Pass in false to use the default, which is 2
     * @param int $wordwrap Pass in 0 for no wordwrap, false for default (40)
     */
    function dump($array,$indent = false,$wordwrap = false) {
      // Dumps to some very clean YAML.  We'll have to add some more features
      // and options soon.  And better support for folding.

      // New features and options.
      if ($indent === false or !is_numeric($indent)) {
        $this->_dumpIndent = 2;
      } else {
        $this->_dumpIndent = $indent;
      }

      if ($wordwrap === false or !is_numeric($wordwrap)) {
        $this->_dumpWordWrap = 40;
      } else {
        $this->_dumpWordWrap = $wordwrap;
      }

      // New YAML document
      $string = "---\n";

      // Start at the base of the array and move through it.
      foreach ($array as $key => $value) {
        $string .= $this->_yamlize($key,$value,0);
      }
      return $string;
    }

    /**** Private Properties ****/

    /**#@+
     * @access private
     * @var mixed
     */
    var $_haveRefs;
    var $_allNodes;
    var $_lastIndent;
    var $_lastNode;
    var $_inBlock;
    var $_isInline;
    var $_dumpIndent;
    var $_dumpWordWrap;
    /**#@-*/

    /**** Private Methods ****/

    /**
     * Attempts to convert a key / value array item to YAML
     * @access private
     * @return string
     * @param $key The name of the key
     * @param $value The value of the item
     * @param $indent The indent of the current node
     */
    function _yamlize($key,$value,$indent) {
      if (is_array($value)) {
        // It has children.  What to do?
        // Make it the right kind of item
        $string = $this->_dumpNode($key,NULL,$indent);
        // Add the indent
        $indent += $this->_dumpIndent;
        // Yamlize the array
        $string .= $this->_yamlizeArray($value,$indent);
      } elseif (!is_array($value)) {
        // It doesn't have children.  Yip.
        $string = $this->_dumpNode($key,$value,$indent);
      }
      return $string;
    }

    /**
     * Attempts to convert an array to YAML
     * @access private
     * @return string
     * @param $array The array you want to convert
     * @param $indent The indent of the current level
     */
    function _yamlizeArray($array,$indent) {
      if (is_array($array)) {
        $string = '';
        foreach ($array as $key => $value) {
          $string .= $this->_yamlize($key,$value,$indent);
        }
        return $string;
      } else {
        return false;
      }
    }

    /**
     * Find out whether a string needs to be output as a literal rather than in plain style.
     * Added by Roan Kattouw 13-03-2008
     * @param $value The string to check
     * @return bool
     */
    function _needLiteral($value) {
      # Check whether the string contains # or : or begins with any of:
      # [ - ? , [ ] { } ! * & | > ' " % @ ` ]
      # or is a number or contains newlines
      return (bool)(gettype($value) == "string" &&
	(is_numeric($value)  ||
	strpos($value, "\n") ||
	preg_match("/[#:]/", $value) ||
	preg_match("/^[-?,[\]{}!*&|>'\"%@`]/", $value)));

    }

    /**
     * Returns YAML from a key and a value
     * @access private
     * @return string
     * @param $key The name of the key
     * @param $value The value of the item
     * @param $indent The indent of the current node
     */
    function _dumpNode($key,$value,$indent) {
      // do some folding here, for blocks
      if ($this->_needLiteral($value)) {
        $value = $this->_doLiteralBlock($value,$indent);
      } else {
        $value  = $this->_doFolding($value,$indent);
      }

      $spaces = str_repeat(' ',$indent);

      if (is_int($key)) {
        // It's a sequence
		if ($value)
			$string = $spaces.'- '.$value."\n";
		else
			$string = $spaces . "-\n";
      } else {
		// It's mapped
		if ($value)
	        $string = $spaces.$key.': '.$value."\n";
		else
			$string = $spaces . $key . ":\n";
      }
      return $string;
    }

    /**
     * Creates a literal block for dumping
     * @access private
     * @return string
     * @param $value
     * @param $indent int The value of the indent
     */
    function _doLiteralBlock($value,$indent) {
      $exploded = explode("\n",$value);
      $newValue = '|';
      $indent  += $this->_dumpIndent;
      $spaces   = str_repeat(' ',$indent);
      foreach ($exploded as $line) {
        $newValue .= "\n" . $spaces . trim($line);
      }
      return $newValue;
    }

    /**
     * Folds a string of text, if necessary
     * @access private
     * @return string
     * @param $value The string you wish to fold
     */
    function _doFolding($value,$indent) {
      // Don't do anything if wordwrap is set to 0
      if ($this->_dumpWordWrap === 0) {
        return $value;
      }

      if (strlen($value) > $this->_dumpWordWrap) {
        $indent += $this->_dumpIndent;
        $indent = str_repeat(' ',$indent);
        $wrapped = wordwrap($value,$this->_dumpWordWrap,"\n$indent");
        $value   = ">\n".$indent.$wrapped;
      }
      return $value;
    }

    /* Methods used in loading */

    /**
     * Finds and returns the indentation of a YAML line
     * @access private
     * @return int
     * @param string $line A line from the YAML file
     */
    function _getIndent($line) {
      $match = array();
      preg_match('/^\s{1,}/',$line,$match);
      if (!empty($match[0])) {
        $indent = substr_count($match[0],' ');
      } else {
        $indent = 0;
      }
      return $indent;
    }

    /**
     * Parses YAML code and returns an array for a node
     * @access private
     * @return array
     * @param string $line A line from the YAML file
     */
    function _parseLine($line) {
      $line = trim($line);

      $array = array();

      if (preg_match('/^-(.*):$/',$line)) {
        // It's a mapped sequence
        $key         = trim(substr(substr($line,1),0,-1));
        $array[$key] = '';
      } elseif ($line[0] == '-' && substr($line,0,3) != '---') {
        // It's a list item but not a new stream
        if (strlen($line) > 1) {
          $value   = trim(substr($line,1));
          // Set the type of the value.  Int, string, etc
          $value   = $this->_toType($value);
          $array[] = $value;
        } else {
          $array[] = array();
        }
      } elseif (preg_match('/^(.+):/',$line,$key)) {
        // It's a key/value pair most likely
        // If the key is in double quotes pull it out
        $matches = array();
        if (preg_match('/^(["\'](.*)["\'](\s)*:)/',$line,$matches)) {
          $value = trim(str_replace($matches[1],'',$line));
          $key   = $matches[2];
        } else {
          // Do some guesswork as to the key and the value
          $explode = explode(':',$line);
          $key     = trim($explode[0]);
          array_shift($explode);
          $value   = trim(implode(':',$explode));
        }

        // Set the type of the value.  Int, string, etc
        $value = $this->_toType($value);
        if (empty($key)) {
          $array[]     = $value;
        } else {
          $array[$key] = $value;
        }
      }
      return $array;
    }

    /**
     * Finds the type of the passed value, returns the value as the new type.
     * @access private
     * @param string $value
     * @return mixed
     */
    function _toType($value) {
      $matches = array();
      if (preg_match('/^("(.*)"|\'(.*)\')/',$value,$matches)) {
       $value = (string)preg_replace('/(\'\'|\\\\\')/',"'",end($matches));
       $value = preg_replace('/\\\\"/','"',$value);
      } elseif (preg_match('/^\\[(.+)\\]$/',$value,$matches)) {
        // Inline Sequence

        // Take out strings sequences and mappings
        $explode = $this->_inlineEscape($matches[1]);

        // Propogate value array
        $value  = array();
        foreach ($explode as $v) {
          $value[] = $this->_toType($v);
        }
      } elseif (strpos($value,': ')!==false && !preg_match('/^{(.+)/',$value)) {
          // It's a map
          $array = explode(': ',$value);
          $key   = trim($array[0]);
          array_shift($array);
          $value = trim(implode(': ',$array));
          $value = $this->_toType($value);
          $value = array($key => $value);
      } elseif (preg_match("/{(.+)}$/",$value,$matches)) {
        // Inline Mapping

        // Take out strings sequences and mappings
        $explode = $this->_inlineEscape($matches[1]);

        // Propogate value array
        $array = array();
        foreach ($explode as $v) {
          $array = $array + $this->_toType($v);
        }
        $value = $array;
      } elseif (strtolower($value) == 'null' or $value == '' or $value == '~') {
        $value = NULL;
      } elseif (ctype_digit($value)) {
        $value = (int)$value;
      } elseif (in_array(strtolower($value),
                  array('true', 'on', '+', 'yes', 'y'))) {
        $value = TRUE;
      } elseif (in_array(strtolower($value),
                  array('false', 'off', '-', 'no', 'n'))) {
        $value = FALSE;
      } elseif (is_numeric($value)) {
        $value = (float)$value;
      } else {
        // Just a normal string, right?
        $value = trim(preg_replace('/#(.+)$/','',$value));
      }

      return $value;
    }

    /**
     * Used in inlines to check for more inlines or quoted strings
     * @access private
     * @return array
     */
    function _inlineEscape($inline) {
      // There's gotta be a cleaner way to do this...
      // While pure sequences seem to be nesting just fine,
      // pure mappings and mappings with sequences inside can't go very
      // deep.  This needs to be fixed.

      // Check for strings
      $regex = '/(?:(")|(?:\'))((?(1)[^"]+|[^\']+))(?(1)"|\')/';
      $strings = array();
      if (preg_match_all($regex,$inline,$strings)) {
        $saved_strings[] = $strings[0][0];
        $inline  = preg_replace($regex,'YAMLString',$inline);
      }
      unset($regex);

      // Check for sequences
      $seqs = array();
      if (preg_match_all('/\[(.+)\]/U',$inline,$seqs)) {
        $inline = preg_replace('/\[(.+)\]/U','YAMLSeq',$inline);
        $seqs   = $seqs[0];
      }

      // Check for mappings
      $maps = array();
      if (preg_match_all('/{(.+)}/U',$inline,$maps)) {
        $inline = preg_replace('/{(.+)}/U','YAMLMap',$inline);
        $maps   = $maps[0];
      }

      $explode = explode(', ',$inline);

      // Re-add the strings
      if (!empty($saved_strings)) {
        $i = 0;
        foreach ($explode as $key => $value) {
          if (strpos($value,'YAMLString')) {
            $explode[$key] = str_replace('YAMLString',$saved_strings[$i],$value);
            ++$i;
          }
        }
      }

      // Re-add the sequences
      if (!empty($seqs)) {
        $i = 0;
        foreach ($explode as $key => $value) {
          if (strpos($value,'YAMLSeq') !== false) {
            $explode[$key] = str_replace('YAMLSeq',$seqs[$i],$value);
            ++$i;
          }
        }
      }

      // Re-add the mappings
      if (!empty($maps)) {
        $i = 0;
        foreach ($explode as $key => $value) {
          if (strpos($value,'YAMLMap') !== false) {
            $explode[$key] = str_replace('YAMLMap',$maps[$i],$value);
            ++$i;
          }
        }
      }

      return $explode;
    }

    /**
     * Builds the PHP array from all the YAML nodes we've gathered
     * @access private
     * @return array
     */
    function _buildArray() {
      $trunk = array();

      if (!isset($this->_indentSort[0])) {
        return $trunk;
      }

      foreach ($this->_indentSort[0] as $n) {
        if (empty($n->parent)) {
          $this->_nodeArrayizeData($n);
          // Check for references and copy the needed data to complete them.
          $this->_makeReferences($n);
          // Merge our data with the big array we're building
          $trunk = $this->_array_kmerge($trunk,$n->data);
        }
      }

      return $trunk;
    }

    /**
     * Traverses node-space and sets references (& and *) accordingly
     * @access private
     * @return bool
     */
    function _linkReferences() {
      if (is_array($this->_haveRefs)) {
        foreach ($this->_haveRefs as $node) {
          if (!empty($node->data)) {
            $key = key($node->data);
            // If it's an array, don't check.
            if (is_array($node->data[$key])) {
              foreach ($node->data[$key] as $k => $v) {
                $this->_linkRef($node,$key,$k,$v);
              }
            } else {
              $this->_linkRef($node,$key);
            }
          }
        }
      }
      return true;
    }

    function _linkRef(&$n,$key,$k = NULL,$v = NULL) {
      if (empty($k) && empty($v)) {
        // Look for &refs
        $matches = array();
        if (preg_match('/^&([^ ]+)/',$n->data[$key],$matches)) {
          // Flag the node so we know it's a reference
          $this->_allNodes[$n->id]->ref = substr($matches[0],1);
          $this->_allNodes[$n->id]->data[$key] =
                   substr($n->data[$key],strlen($matches[0])+1);
        // Look for *refs
        } elseif (preg_match('/^\*([^ ]+)/',$n->data[$key],$matches)) {
          $ref = substr($matches[0],1);
          // Flag the node as having a reference
          $this->_allNodes[$n->id]->refKey =  $ref;
        }
      } elseif (!empty($k) && !empty($v)) {
        if (preg_match('/^&([^ ]+)/',$v,$matches)) {
          // Flag the node so we know it's a reference
          $this->_allNodes[$n->id]->ref = substr($matches[0],1);
          $this->_allNodes[$n->id]->data[$key][$k] =
                              substr($v,strlen($matches[0])+1);
        // Look for *refs
        } elseif (preg_match('/^\*([^ ]+)/',$v,$matches)) {
          $ref = substr($matches[0],1);
          // Flag the node as having a reference
          $this->_allNodes[$n->id]->refKey =  $ref;
        }
      }
    }

    /**
     * Finds the children of a node and aids in the building of the PHP array
     * @access private
     * @param int $nid The id of the node whose children we're gathering
     * @return array
     */
    function _gatherChildren($nid) {
      $return = array();
      $node   =& $this->_allNodes[$nid];
      foreach ($this->_allNodes as $z) {
        if ($z->parent == $node->id) {
          // We found a child
          $this->_nodeArrayizeData($z);
          // Check for references
          $this->_makeReferences($z);
          // Merge with the big array we're returning
          // The big array being all the data of the children of our parent node
          $return = $this->_array_kmerge($return,$z->data);
        }
      }
      return $return;
    }

    /**
     * Turns a node's data and its children's data into a PHP array
     *
     * @access private
     * @param array $node The node which you want to arrayize
     * @return boolean
     */
    function _nodeArrayizeData(&$node) {
      if (is_array($node->data) && $node->children == true) {
        // This node has children, so we need to find them
        $childs = $this->_gatherChildren($node->id);
        // We've gathered all our children's data and are ready to use it
        $key = key($node->data);
        $key = empty($key) ? 0 : $key;
        // If it's an array, add to it of course
        if (is_array($node->data[$key])) {
          $node->data[$key] = $this->_array_kmerge($node->data[$key],$childs);
        } else {
          $node->data[$key] = $childs;
        }
      } elseif (!is_array($node->data) && $node->children == true) {
        // Same as above, find the children of this node
        $childs       = $this->_gatherChildren($node->id);
        $node->data   = array();
        $node->data[] = $childs;
      }

      // We edited $node by reference, so just return true
      return true;
    }

    /**
     * Traverses node-space and copies references to / from this object.
     * @access private
     * @param object $z A node whose references we wish to make real
     * @return bool
     */
    function _makeReferences(&$z) {
      // It is a reference
      if (isset($z->ref)) {
        $key                = key($z->data);
        // Copy the data to this object for easy retrieval later
        $this->ref[$z->ref] =& $z->data[$key];
      // It has a reference
      } elseif (isset($z->refKey)) {
        if (isset($this->ref[$z->refKey])) {
          $key           = key($z->data);
          // Copy the data from this object to make the node a real reference
          $z->data[$key] =& $this->ref[$z->refKey];
        }
      }
      return true;
    }


    /**
     * Merges arrays and maintains numeric keys.
     *
     * An ever-so-slightly modified version of the array_kmerge() function posted
     * to php.net by mail at nospam dot iaindooley dot com on 2004-04-08.
     *
     * http://www.php.net/manual/en/function.array-merge.php#41394
     *
     * @access private
     * @param array $arr1
     * @param array $arr2
     * @return array
     */
    function _array_kmerge($arr1,$arr2) {
      if(!is_array($arr1))
        $arr1 = array();

      if(!is_array($arr2))
        $arr2 = array();

      $keys1 = array_keys($arr1);
      $keys2 = array_keys($arr2);
      $keys  = array_merge($keys1,$keys2);
      $vals1 = array_values($arr1);
      $vals2 = array_values($arr2);
      $vals  = array_merge($vals1,$vals2);
      $ret   = array();

      foreach($keys as $key) {
        list( /* unused */ ,$val) = each($vals);
        // This is the good part!  If a key already exists, but it's part of a
        // sequence (an int), just keep addin numbers until we find a fresh one.
        if (isset($ret[$key]) and is_int($key)) {
          while (array_key_exists($key, $ret)) {
            $key++;
          }
        }
        $ret[$key] = $val;
      }

      return $ret;
    }
  }
