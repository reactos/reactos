<?php

class Text_Wiki_Render_Xhtml_tt extends Text_Wiki_Render {
    
    
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
        if ($options['type'] == 'start') {
            $css = $this->formatConf(' class="%s"', 'css');
            return "<tt$css>";
        }
        
        if ($options['type'] == 'end') {
            return '</tt>';
        }
    }
}
?>