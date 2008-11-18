<?php

class Text_Wiki_Render_Xhtml_Paragraph extends Text_Wiki_Render {
    
    var $conf = array(
        'css' => null
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
        extract($options); //type
        
        if ($type == 'start') {
            $css = $this->formatConf(' class="%s"', 'css');
            return "<p$css>";
        }
        
        if ($type == 'end') {
            return "</p>\n\n";
        }
    }
}
?>