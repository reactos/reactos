// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// Description: This is a place to add generic helper methods for the
//              SimpleGenerator CG model.
//
//              SimpleGenerator always expects the root of the deserialized
//              XML tree to be MS.Internal.MilCodeGen.ResourceModel.CG.
//              

namespace MS.Internal.MilCodeGen.ResourceModel
{
    public partial class CG
    {
        // SimpleGenerator expects an OutputDirectory field to store the
        // the value passed on the commandline as -outputDirectory:
        public string OutputDirectory;
    }
}



