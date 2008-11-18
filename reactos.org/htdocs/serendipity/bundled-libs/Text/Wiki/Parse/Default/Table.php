<?php
// $Id: Table.php,v 1.1 2005/01/31 15:46:52 pmjones Exp $


/**
* 
* This class implements a Text_Wiki_Parse to find source text marked as a
* set of table rows, where a line start and ends with double-pipes (||)
* and uses double-pipes to separate table cells.  The rows must be on
* sequential lines (no blank lines between them) -- a blank line
* indicates the beginning of a new table.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Parse_Table extends Text_Wiki_Parse {
    
    
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
    
    var $regex = '/\n((\|\|).*)(\n)(?!(\|\|))/Us';
    
    
    /**
    * 
    * Generates a replacement for the matched text.
    * 
    * Token options are:
    * 
    * 'type' =>
    *     'table_start' : the start of a bullet list
    *     'table_end'   : the end of a bullet list
    *     'row_start' : the start of a number list
    *     'row_end'   : the end of a number list
    *     'cell_start'   : the start of item text (bullet or number)
    *     'cell_end'     : the end of item text (bullet or number)
    * 
    * 'cols' => the number of columns in the table (for 'table_start')
    * 
    * 'rows' => the number of rows in the table (for 'table_start')
    * 
    * 'span' => column span (for 'cell_start')
    * 
    * 'attr' => column attribute flag (for 'cell_start')
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return A series of text and delimited tokens marking the different
    * table elements and cell text.
    *
    */
    
    function process(&$matches)
    {
        // our eventual return value
        $return = '';
        
        // the number of columns in the table
        $num_cols = 0;
        
        // the number of rows in the table
        $num_rows = 0;
        
        // rows are separated by newlines in the matched text
        $rows = explode("\n", $matches[1]);
        
        // loop through each row
        foreach ($rows as $row) {
            
            // increase the row count
            $num_rows ++;
            
            // start a new row
            $return .= $this->wiki->addToken(
                $this->rule,
                array('type' => 'row_start')
            );
            
            // cells are separated by double-pipes
            $cell = explode("||", $row);
            
            // get the number of cells (columns) in this row
            $last = count($cell) - 1;
            
            // is this more than the current column count?
            // (we decrease by 1 because we never use cell zero)
            if ($last - 1 > $num_cols) {
                // increase the column count
                $num_cols = $last - 1;
            }
            
            // by default, cells span only one column (their own)
            $span = 1;
            
            // ignore cell zero, and ignore the "last" cell; cell zero
            // is before the first double-pipe, and the "last" cell is
            // after the last double-pipe. both are always empty.
            for ($i = 1; $i < $last; $i ++) {
                
                // if there is no content at all, then it's an instance
                // of two sets of || next to each other, indicating a
                // span.
                if ($cell[$i] == '') {
                    
                    // add to the span and loop to the next cell
                    $span += 1;
                    continue;
                    
                } else {
                    
                    // this cell has content.
                    
                    // find any special "attr"ibute cell markers
                    if (substr($cell[$i], 0, 2) == '> ') {
                        // right-align
                        $attr = 'right';
                        $cell[$i] = substr($cell[$i], 2);
                    } elseif (substr($cell[$i], 0, 2) == '= ') {
                        // center-align
                        $attr = 'center';
                        $cell[$i] = substr($cell[$i], 2);
                    } elseif (substr($cell[$i], 0, 2) == '< ') {
                        // left-align
                        $attr = 'left';
                        $cell[$i] = substr($cell[$i], 2);
                    } elseif (substr($cell[$i], 0, 2) == '~ ') {
                        $attr = 'header';
                        $cell[$i] = substr($cell[$i], 2);
                    } else {
                        $attr = null;
                    }
                    
                    // start a new cell...
                    $return .= $this->wiki->addToken(
                        $this->rule, 
                        array (
                            'type' => 'cell_start',
                            'attr' => $attr,
                            'span' => $span
                        )
                    );
                    
                    // ...add the content...
                    $return .= trim($cell[$i]);
                    
                    // ...and end the cell.
                    $return .= $this->wiki->addToken(
                        $this->rule, 
                        array (
                            'type' => 'cell_end',
                            'attr' => $attr,
                            'span' => $span
                        )
                    );
                    
                    // reset the span.
                    $span = 1;
                }
                    
            }
            
            // end the row
            $return .= $this->wiki->addToken(
                $this->rule,
                array('type' => 'row_end')
            );
            
        }
        
        // wrap the return value in start and end tokens 
        $return =
            $this->wiki->addToken(
                $this->rule,
                array(
                    'type' => 'table_start',
                    'rows' => $num_rows,
                    'cols' => $num_cols
                )
            )
            . $return .
            $this->wiki->addToken(
                $this->rule,
                array(
                    'type' => 'table_end'
                )
            );
        
        // we're done!
        return "\n$return\n\n";
    }
}
?>