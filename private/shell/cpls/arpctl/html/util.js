// util.js


function EatErrors()
{
    // Prevent scripting errors from displaying messages
    return true;
}


// Synchronize the selected row in the table with the current record in 
// the recordset
// 
//  oTable - the table to sync
//  oRecordSet - the recordset with which to sync

function SyncTableUI(oTable, oDSO)
{
    // snag the current record number from the ADO recordset
    var nCurrentRecord = oDSO.recordset.absoluteposition;

    // find the corresponding row in the table by comparing 
    // absolute position to recordNumber property of each table row
    var cRows = oTable.rows.length;
    for (i = 0; i < cRows; i++)
    {
        var oRow = oTable.rows[i];
        if (oRow.recordNumber == nCurrentRecord)
        {
            SelectRow(oRow, oDSO);
            break;
        }
    }
}


// Highlight the row of the selected item and update
// the recordset accordingly
//
//  oTR - the table-row element that is selected
//  oDSO - the data source object to update

function SetRecordSet(oTR, oDSO)
{
    window.event.cancelBubble = true;

    oDSO.recordset.absoluteposition = oTR.recordNumber;

    SelectRow(oTR, oDSO.recordset);
}
