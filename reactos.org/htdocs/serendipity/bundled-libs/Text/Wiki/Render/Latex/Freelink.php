<?php

class Text_Wiki_Render_Latex_Freelink extends Text_Wiki_Render {
    
    var $conf = array(
        'pages' => array(),
        'view_url' => 'http://example.com/index.php?page=%s',
        'new_url'  => 'http://example.com/new.php?page=%s',
        'new_text' => '?'
    );
    
    
    /**
    * 
    * Renders a token into text matching the requested format.
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
        // get nice variable names (page, text, anchor)
        extract($options);
        
        return "$text\\footnote\{$anchor} ";
    }
}
?>