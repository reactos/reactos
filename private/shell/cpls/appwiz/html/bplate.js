
// bplate.js



/*-------------------------------------------------------------------------
Purpose: Finds the next token, delimited by chDelim.  Returns index or -1.
*/
function _NextToken(sz, ichStart, chDelim)
{
    var ich = ichStart;
    var cch = sz.length;

    while (ich < cch && sz.charAt(ich) == chDelim)
    {
        ich++;
    }

    if (ich == cch)
        ich = -1;
        
    return ich;
}



// BoilerPlate class.  
//
// Applies the fields in a structured string record (provided to BoilerPlate_Parse)
// to the contents of a document, according to custom attributes in the html.

function BoilerPlate()
{
    this._rgszFields = new Array();
    this._cfield = 0;
}


/*-------------------------------------------------------------------------
Purpose: Take the string of arguments and parse it into separate fields.
         We'll use a sparse array to store this, indexed by field name.  
         szArgs is in the following format:

         <fieldname1 "value1"><fieldname2 value2>...

         
*/
function BoilerPlate_Parse(szArgs)
{
    szArgs += " ";  // Force szArgs to really be a string...

    // alert("Parse:" + szArgs);
    
    var ichTag = 0;

    while (true)
    {
        // Find tag
        ichTag = szArgs.indexOf("<", ichTag);
        if (-1 == ichTag)
            break;
        else 
        {
            // We're lazy here -- if there's a tag, we expect it to be properly formed
            ichTag++;

            var ichNameEnd = szArgs.indexOf(" ", ichTag);
            var ichValueBegin = _NextToken(szArgs, ichNameEnd, " ");
            var ichValueEnd;

            if (szArgs.charAt(ichValueBegin) == '"')
            {
                ichValueBegin++;
                
                ichValueEnd = szArgs.indexOf('"', ichValueBegin);
            }
            else
                ichValueEnd = szArgs.indexOf(">", ichValueBegin);
                
            var szField = szArgs.substring(ichTag, ichNameEnd);
            var szValue = szArgs.substring(ichValueBegin, ichValueEnd);

            // alert('<' + szField + ':' + szValue + '>');

            this._rgszFields[szField.toLowerCase()] = szValue;
            this._cfield++;

            // Skip over the value so we don't accidentally mistake the value
            // for the next tag.
            ichTag = ichValueEnd;
        }
    }
}


/*-------------------------------------------------------------------------
Purpose: Returns the value of the fieldname, or null if no fieldname exists
*/
function BoilerPlate_Get(szFieldName)
{
    var szValue = this._rgszFields[szFieldName.toLowerCase()];

    if ("" == szValue || "undefined" == typeof szValue)
        return null;
        
    return szValue;
}


/*-------------------------------------------------------------------------
Purpose: Returns the number of field values that we have
*/
function BoilerPlate_Length()
{
    return this._cfield;
}



/*-------------------------------------------------------------------------
Purpose: Finds and returns the immediate parent row, or null if not found
*/
function _FindParentRow(elem)
{
    // Walk up the chain of elements until we find the immediate parent row element
    var trLast = null;

    while (elem)
    {
        if ("TR" == elem.tagName.toUpperCase())
            return elem;
        elem = elem.parentElement;
    }
    
    return null;
}


/*-------------------------------------------------------------------------
Purpose: Apply the fields in the object to the html.  This function walks
         the html and looks for elements that have the following
         custom attributes.  The value of <fieldname> should be inserted...

            _bpInnerText=<fieldname>  - ...in the element's innertext property.
            _bpHref=<fieldname>       - ...in the element's href property.
            _bpValue=<fieldname>      - ...in the element's _bpVar property
            _bpNop=<fieldname>        - ...nowhere.  The element is hidden/displayed if
                                        value is valid.

*/
function BoilerPlate_Apply()
{
    var i;
    
    // Walk thru the document and look for our custom attributes
    for (i = document.all.length - 1; i >= 0; i--)
    {
        var elem = document.all.item(i);
        var szField;
        var bDeleteRow = false;
        var bIsElemMarked = false;
        var bHideElem = false;

        // Insert a field into the innertext property?
        if (null != elem._bpInnerText)
        {
            // Yes
            szField = this.Get(elem._bpInnerText);
            if (null == szField)
            {
                // Delete the row since there is no useful info to show
                bDeleteRow = true;
                bHideElem = true;
                szField = "n/a";
            }
            
            elem.innerText = szField;
            bIsElemMarked = true;
        }

        // Insert a field into the href property?
        if (null != elem._bpHref && "A" == elem.tagName)
        {
            szField = this.Get(elem._bpHref);
            
            bHideElem = false;      // Reset for this case

            // alert("href [" + elem._bpHref + "] = " + szField);
            
            if (null != szField && 
                ("http:" == szField.substring(0, 5) || 
                 "https:" == szField.substring(0, 6) ||
                 "file:" == szField.substring(0, 5) ||
                 "ftp:" == szField.substring(0, 4)))
            {
                // Yes
                elem.href = szField;

                // Was the friendly name unknown?
                if ("n/a" == elem.innerText)
                {
                    // Yes; make the friendly name identical to the url
                    elem.innerText = elem.href;
                }

                // Are the friendly name and href different?
                if (elem.href != elem.innerText)
                {
                    // Yes; let's show the href URL in the tooltip.
                    elem.title = elem.href;
                }

                bDeleteRow = false;     // cancel any pending deletion
            }
            else
            {
                // No; change the color to match the regular text
                elem.style.color = document.fgColor;
            }
            bIsElemMarked = true;

            if ("n/a" == szField)
                bHideElem = true;
        }

        
        // Insert an arbitrary value?
        if (null != elem._bpValue)
        {
            // Yes
            elem._bpVar = this.Get(elem._bpValue);
            bIsElemMarked = true;
        }

        // Hide element?
        if (null != elem._bpNop && null == this.Get(elem._bpNop))
        {
            // Yes
            bHideElem = true;
        }

        // Is this element worthy of showing at all?
        if (bHideElem)
        {
            // No; hide it
            elem.style.display = 'none';
        }
        
        if (bIsElemMarked)
        {
            // We may think this row can be deleted, but there might be other fields 
            // in this row that are valid, so let's mark this row and we'll make
            // a second pass after we're finished.
            var trElem = _FindParentRow(elem);

            if (trElem && false != trElem._bpDelete)
            {
                trElem._bpDelete = bDeleteRow;
                // alert("setting _bpDelete[" + trElem.rowIndex + "] = " + trElem._bpDelete);
            }
        }
    }

    // Make one final pass to delete any pending rows
    var rgrows = document.all.tags("TR");
    
    for (i = rgrows.length - 1; i >= 0; i--)
    {
        var trElem = rgrows[i];

        if (trElem && trElem._bpDelete)
        {
            var tbl = trElem.parentElement;

            tbl.deleteRow(trElem.rowIndex);
        }
    }
}


/*-------------------------------------------------------------------------
Purpose: Initialize class
*/
function InitBoilerPlateClass()
{
    // Create and discard an initial Panel object for prototypes
    new BoilerPlate(null);

    // BoilerPlate Methods
    BoilerPlate.prototype.Parse = BoilerPlate_Parse;
    BoilerPlate.prototype.Get = BoilerPlate_Get;
    BoilerPlate.prototype.Length = BoilerPlate_Length;
    BoilerPlate.prototype.Apply = BoilerPlate_Apply;
}

