<?php
/* vim: set expandtab tabstop=4 shiftwidth=4: */
// +----------------------------------------------------------------------+
// | PHP version 4                                                        |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2003 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available through the world-wide-web at                              |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Paul M. Jones <pmjones@ciaweb.net>                          |
// +----------------------------------------------------------------------+
//
// $Id: Rule.php,v 1.3 2004/12/02 10:54:31 nohn Exp $

/**
* 
* Baseline rule class for extension into a "real" wiki rule.
* 
* Text_Wiki_Rule classes do not stand on their own; they are called by a
* Text_Wiki object, typcially in the transform()method. Each rule class
* performs three main activities: parse, process, and render.
* 
* The parse() method takes a regex and applies it to the whole block of
* source text at one time. Each match is sent as $matches to the
* process() method.
* 
* The process() method acts on the matched text from the source, and
* then processes the source text is some way.  This may mean the
* creation of a delimited token using addToken().  In every case, the
* process() method returns the text that should replace the matched text
* from parse().
* 
* Finally, the render*() methods take any token created by the rule and
* creates output text matching a specific format.  Individual rules may
* or may not have options for specific formats.
*
* Typically, the extended rule only needs to define the process() and
* render() methods; the parse() method is generally the same from rule
* to rule, and has been included here.  However, there is no reason that
* rules cannot override these methods, so long as the new methods operate
* in substantially the same manner; similarly, the rule need not generate
* any tokens at all, and may use a specialized parse() method to filter
* the source text.  See the default rule classes for many examples.
* 
* @author Paul M. Jones <pmjones@ciaweb.net>
* 
* @package Text_Wiki
* 
*/

class Text_Wiki_Rule {
    
    
    /**
    * 
    * The regular expression used to parse the source text and find
    * matches conforming to this rule.  Used by the parse() method.
    * 
    * @access public
    * 
    * @var string
    * 
    * @see parse()
    * 
    */
    
    var $regex = null;
    
    
    /**
    * 
    * A reference to the calling Text_Wiki object.  This is needed so
    * that each rule has access to the same source text, token set,
    * URLs, interwiki maps, page names, etc.
    * 
    * @access public
    * 
    * @var object
    */
    
    var $_wiki = null;
    
    
    /**
    * 
    * The name of this rule; used when inserting new token array
    * elements.
    * 
    * @access private
    * 
    * @var string
    * 
    */
    
    var $_rule = null;
    
    
    /**
    * 
    * Configuration options for this rule.
    * 
    * @access private
    * 
    * @var string
    * 
    */
    
    var $_conf = null;
    
    
    /**
    * 
    * Constructor for the rule.
    * 
    * @access public
    * 
    * @param object &$obj The calling "parent" Text_Wiki object.
    * 
    * @param string $name The token name to use for this rule.
    * 
    */
    
    function Text_Wiki_Rule(&$obj, $name)
    {
        // set the reference to the calling Text_Wiki object;
        // this allows us access to the shared source text, token
        // array, etc.
        $this->_wiki =& $obj;
        
        // set the name of this rule; generally used when adding
        // to the tokens array.
        $this->_rule = $name;
        
        // config options reference
        $this->_conf =& $this->_wiki->rules[$this->_rule]['conf'];
    }
    
    
    /**
    * 
    * Add a token to the Text_Wiki tokens array, and return a delimited
    * token number.
    * 
    * @access public
    * 
    * @param array $options An associative array of options for the new
    * token array element.  The keys and values are specific to the
    * rule, and may or may not be common to other rule options.  Typical
    * options keys are 'text' and 'type' but may include others.
    * 
    * @param boolean $id_only If true, return only the token number, not
    * a delimited token string.
    * 
    * @return string|int By default, return the number of the
    * newly-created token array element with a delimiter prefix and
    * suffix; however, if $id_only is set to true, return only the token
    * number (no delimiters).
    * 
    */
    
    function addToken($options = array(), $id_only = false)
    {
        // force the options to be an array
        settype($options, 'array');
        
        // find the next token number
        $id = count($this->_wiki->_tokens);
        
        // add the token
        $this->_wiki->_tokens[$id] = array(
            0 => $this->_rule,
            1 => $options
        );
        
        // return a value
        if ($id_only) {
            // return the last token number
            return $id;
        } else {
            // return the token number with delimiters
            return $this->_wiki->delim . $id . $this->_wiki->delim;
        }
    }
    
    
    /**
    * 
    * Set or re-set a token with specific information, overwriting any
    * previous rule name and rule options.
    * 
    * @access public
    * 
    * @param int $id The token number to reset.
    * 
    * @param int $rule The rule name to use; by default, use the current
    * rule name, although you can specify any rule.
    * 
    * @param array $options An associative array of options for the
    * token array element.  The keys and values are specific to the
    * rule, and may or may not be common to other rule options.  Typical
    * options keys are 'text' and 'type' but may include others.
    * 
    * @return void
    * 
    */
    
    function setToken($id, $rule = null, $options = array())
    {
        // get a rule name
        if (is_null($rule)) {
            $rule = $this->_rule;
        }
        
        // reset the token
        $this->_wiki->_tokens[$id] = array(
            0 => $rule,
            1 => $options
        );
    }
    
    
    /**
    * 
    * Simple parsing method to apply the rule's regular expression to
    * the source text, pass every match to the process() method, and
    * replace the matched text with the results of the processing.
    *
    * @access public
    * 
    */
    
    function parse()
    {
        $this->_wiki->_source = preg_replace_callback(
            $this->regex,
            array(&$this, 'process'),
            $this->_wiki->_source
        );
    }
    
    
    /**
    * 
    * Simple processing mathod to take matched text and generate
    * replacement text. This is one of the methods you will definitely
    * want to override in your rule class extensions.
    * 
    * @access public
    * 
    * @param array $matches An array of matches from the parse() method
    * as generated by preg_replace_callback.  $matches[0] is the full
    * matched string, $matches[1] is the first matched pattern,
    * $matches[2] is the second matched pattern, and so on.
    * 
    * @return string The processed text replacement; defaults to the
    * full matched string (i.e., no changes to the text).
    * 
    */
    
    function process(&$matches)
    {
        return $matches[0];
    }
    
    
    /**
    * 
    * Simple rendering method to take a set of token options and
    * generate replacement text for it.  This is another method you will
    * definitely want to override in your rule subclass extensions.
    *
    * @access public
    * 
    * @param array $options The "options" portion of the token (second element).
    * 
    * @return string The text rendered from the token options; by default,
    * no text is returned.  You should change this in your subclass.  ;-)
    * 
    */
    
    function renderXhtml($options)
    {
        return '';
    }
    
    
    /**
    * 
	* Simple method to extract 'option="value"' portions of wiki markup,
	* typically used only in macros.
	* 
	* The syntax is pretty strict; there can be no spaces between the
	* option name, the equals, and the first double-quote; the value
	* must be surrounded by double-quotes.  You can escape characters in
	* the value with a backslash, and the backslash will be stripped for
	* you.
	* 
	* @access public
	* 
	* @param string $text The "macro options" portion of macro markup.
	* 
	* @return array An associative array of key-value pairs where the
	* key is the option name and the value is the option value.
    * 
    */
    
    function getMacroArgs($text)
    {
    	// find the =" sections;
		$tmp = explode('="', trim($text));
		
		// basic setup
		$k = count($tmp) - 1;
		$arg = array();
		$key = null;
		
		// loop through the sections
		foreach ($tmp as $i => $val) {
			
			// first element is always the first key
			if ($i == 0) {
				$key = trim($val);
				continue;
			}
			
			// find the last double-quote in the value.
			// the part to the left is the value for the last key,
			// the part to the right is the next key name
			$pos = strrpos($val, '"');
			$arg[$key] = stripslashes(substr($val, 0, $pos));
			$key = trim(substr($val, $pos+1));
			
		}
		
		return $arg;
		
    }
}
?>
