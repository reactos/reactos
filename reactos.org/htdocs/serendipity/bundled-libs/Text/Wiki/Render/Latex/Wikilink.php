<?php

class Text_Wiki_Render_Latex_Wikilink extends Text_Wiki_Render {
    var $conf = array(
        'pages' => array(),
        'view_url' => 'http://example.com/index.php?page=%s',
        'new_url'  => 'http://example.com/new.php?page=%s',
        'new_text' => '?'
    );
    
    /**
    * 
    * Renders a token into XHTML.
    * 
    * @access public
    * 
    * @param array $options The "options" portion of the token (second
    * element).
    * 
    * @return string The text rendered from the token options.
    * 
    */
    
    function token($options)
    {
        // make nice variable names (page, anchor, text)
        extract($options);
        
        // are we checking page existence?
        $list =& $this->getConf('pages');
        if (is_array($list)) {
            // yes, check against the page list
            $exists = in_array($page, $list);
        } else {
            // no, assume it exists
            $exists = true;
        }
        
        // convert *after* checking against page names so as not to mess
        // up what the user typed and what we're checking.
        $page = htmlspecialchars($page);
        $anchor = htmlspecialchars($anchor);
        $text = htmlspecialchars($text);
        
        $href = $this->getConf('view_url');
            
        if (strpos($href, '%s') === false) {
            // use the old form (page-at-end)
            $href = $href . $page . $anchor;
        } else {
            // use the new form (sprintf format string)
            $href = sprintf($href, $page . $anchor);
        }
        
        // get the CSS class and generate output
        $css = $this->formatConf(' class="%s"', 'css');
        return "$text\\footnote\{$href}";
    }
}
?>