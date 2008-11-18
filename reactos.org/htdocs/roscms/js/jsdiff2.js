/*
 * Javascript Diff Algorithm
 *  By John Resig (http://ejohn.org/)
 *
 * More Info:
 *  http://ejohn.org/projects/javascript-diff-algorithm/
 */

function diffString_ver2( o, n ) {
  var out = diff_ver2( o.split(/\s+/), n.split(/\s+/) );
  var str = "";

  for ( var i = 0; i < out.n.length - 1; i++ ) {
    if ( out.n[i].text == null ) {
      if ( out.n[i].indexOf('"') == -1 && out.n[i].indexOf('<') == -1 )
        str += "<ins style='background:#E6FFE6;'> " + out.n[i] +"</ins>";
      else
        str += " " + out.n[i];
    } else {
      var pre = "";
      if ( out.n[i].text.indexOf('"') == -1 && out.n[i].text.indexOf('<') == -1 ) {
        
        var n = out.n[i].row + 1;
        while ( n < out.o.length && out.o[n].text == null ) {
          if ( out.o[n].indexOf('"') == -1 && out.o[n].indexOf('<') == -1 && out.o[n].indexOf(':') == -1 && out.o[n].indexOf(';') == -1 )
            pre += " <del style='background:#FFE6E6;'>" + out.o[n] +" </del>";
          n++;
        }
      }
      str += " " + out.n[i].text + pre;
    }
  }
  
  return str;
}

function diff_ver2( o, n ) {
  var ns = new Array();
  var os = new Array();
  
  for ( var i = 0; i < n.length; i++ ) {
    if ( ns[ n[i] ] == null )
      ns[ n[i] ] = { rows: new Array(), o: null };
    ns[ n[i] ].rows.push( i );
  }
  
  for ( var i = 0; i < o.length; i++ ) {
    if ( os[ o[i] ] == null )
      os[ o[i] ] = { rows: new Array(), n: null };
    os[ o[i] ].rows.push( i );
  }
  
  for ( var i in ns ) {
    if ( ns[i].rows.length == 1 && typeof(os[i]) != "undefined" && os[i].rows.length == 1 ) {
      n[ ns[i].rows[0] ] = { text: n[ ns[i].rows[0] ], row: os[i].rows[0] };
      o[ os[i].rows[0] ] = { text: o[ os[i].rows[0] ], row: ns[i].rows[0] };
    }
  }
  
  for ( var i = 0; i < n.length - 1; i++ ) {
    if ( n[i].text != null && n[i+1].text == null && o[ n[i].row + 1 ].text == null && 
         n[i+1] == o[ n[i].row + 1 ] ) {
      n[i+1] = { text: n[i+1], row: n[i].row + 1 };
      o[n[i].row+1] = { text: o[n[i].row+1], row: i + 1 };
    }
  }
  
  for ( var i = n.length - 1; i > 0; i-- ) {
    if ( n[i].text != null && n[i-1].text == null && o[ n[i].row - 1 ].text == null && 
         n[i-1] == o[ n[i].row - 1 ] ) {
      n[i-1] = { text: n[i-1], row: n[i].row - 1 };
      o[n[i].row-1] = { text: o[n[i].row-1], row: i - 1 };
    }
  }
  
  return { o: o, n: n };
}
