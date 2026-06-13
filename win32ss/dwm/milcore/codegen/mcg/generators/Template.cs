// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// Description: This file contains the definition of template-based generation
//              
//---------------------------------------------------------------------------

using System;
using System.IO;
using System.Xml;
using System.Collections.Generic;

using MS.Internal.MilCodeGen;
using MS.Internal.MilCodeGen.Runtime;
using MS.Internal.MilCodeGen.ResourceModel;
using MS.Internal.MilCodeGen.Helpers;

namespace MS.Internal.MilCodeGen.ResourceModel
{
    public abstract class Template
    {
        public abstract void Go(ResourceModel resourceModel);
        public abstract void AddTemplateInstance(ResourceModel resourceModel, XmlNode node);
    }

    public class TemplateGeneration: Main.GeneratorBase
    {
        public TemplateGeneration(ResourceModel resourceModel): base(resourceModel)
        {
            
        }

        public override void Go()
        {
            foreach (Template template in _resourceModel.TemplateGenerationControl)
            {
                template.Go(_resourceModel);
            }
        }
    }
}


