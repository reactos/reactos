<?php
// $Id: Embed.php,v 1.1 2005/01/31 15:46:52 pmjones Exp $


/**
* 
* This class implements a Text_Wiki_Parse to embed the contents of a URL
* inside the page at render-time.  Typically used to get script output.
* This differs from the 'include' rule, which incorporates results at
* parse-time; 'embed' output does not get parsed by Text_Wiki, while
* 'include' ouput does.
*
* This rule is inherently not secure; it allows cross-site scripting to
* occur if the embedded output has <script> or other similar tags.  Be
* careful.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Parse_Embed extends Text_Wiki_Parse {
    
    var $conf = array(
        'base' => '/path/to/scripts/'
    );
    
    var $file = null;

    var $output = null;

    var $vars = null;


    /**
    * 
    * The regular expression used to find source text matching this
    * rule.
    * 
    * @access public
    * 
    * @var string
    * 
    */
    
    var $regex = '/(\[\[embed )(.+?)( .+?)?(\]\])/i';
    
    
    /**
    * 
    * Generates a token entry for the matched text.  Token options are:
    * 
    * 'text' => The full matched text, not including the <code></code> tags.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return A delimited token number to be used as a placeholder in
    * the source text.
    *
    */
    
    function process(&$matches)
    {    
        // save the file location
        $this->file = $this->getConf('base', './') . $matches[2];
        
        // extract attribs as variables in the local space
        $this->vars = $this->getAttrs($matches[3]);
        unset($this->vars['this']);
        extract($this->vars);
        
        // run the script
        ob_start();
        include($this->file);
        $this->output = ob_get_contents();
        ob_end_clean();
        
        // done, place the script output directly in the source
        return $this->wiki->addToken(
            $this->rule,
            array('text' => $this->output)
        );
    }
}
?>