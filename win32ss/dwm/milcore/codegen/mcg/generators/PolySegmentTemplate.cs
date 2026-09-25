// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// Description: This file contains the definition of template-based generation of PolySegments
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
    public class PolySegmentTemplate: Template
    {
        private struct PolySegmentTemplateInstance
        {
            public PolySegmentTemplateInstance(
                string mangedDestinationDir,
                McgType className,
                string typeName,
                int verifyCount,
                bool isCurved)
            {
                ManagedDestinationDir = mangedDestinationDir;
                ClassName = className;
                TypeName = typeName;
                VerifyCount = verifyCount;
                IsCurved = isCurved;
            }

            public string ManagedDestinationDir;
            public McgType ClassName;
            public string TypeName;
            public int VerifyCount;
            public bool IsCurved;
        }

        public override void AddTemplateInstance(ResourceModel resourceModel, XmlNode node)
        {
            Instances.Add(new PolySegmentTemplateInstance(
                ResourceModel.ToString(node, "ManagedDestinationDir"),
                resourceModel.FindType(ResourceModel.ToString(node, "ClassName")),
                ResourceModel.ToString(node, "TypeName"),
                ResourceModel.ToInt32(node, "VerifyCount"),
                ResourceModel.ToBoolean(node, "IsCurved", false)));
        }

        public override void Go(ResourceModel resourceModel)
        {
            foreach (PolySegmentTemplateInstance instance in Instances)
            {
                string fileName = instance.ClassName.Name + "FigureLogic.cs";

                string fullPath = Path.Combine(resourceModel.OutputDirectory, instance.ManagedDestinationDir);

                using (FileCodeSink csFile = new FileCodeSink(fullPath, fileName, true /* Create dir if necessary */))
                {
                    csFile.WriteBlock(
                        [[inline]]
                            [[Helpers.ManagedStyle.WriteFileHeader(fileName,  @"wpf\src\Graphics\codegen\mcg\generators\PolySegmentTemplate.cs")]]

                            using System;
                            using System.Collections;
                            using System.Collections.Generic;
                            using System.ComponentModel;
                            using System.Security.Permissions;
                            using System.Windows;
                            using System.Windows.Markup;
                            using System.Windows.Media.Animation;
                            using System.ComponentModel.Design.Serialization;
                            using System.Windows.Media.Composition;
                            using System.Reflection;
                            using MS.Internal;
                            using System.Security;

                            using SR=MS.Internal.PresentationCore.SR;
                            using SRID=MS.Internal.PresentationCore.SRID;

                            namespace System.Windows.Media
                            {
                                #region [[instance.ClassName.Name]]
                                
                                /// <summary>
                                /// [[instance.ClassName.Name]]
                                /// </summary>
                                public sealed partial class [[instance.ClassName.Name]] : PathSegment
                                {
                                    #region Constructors
                                    /// <summary>
                                    /// [[instance.ClassName.Name]] constructor
                                    /// </summary>
                                    public [[instance.ClassName.Name]]()
                                    {
                                    }

                                    /// <summary>
                                    ///
                                    /// </summary>
                                    public [[instance.ClassName.Name]](IEnumerable<Point> points, bool isStroked)
                                    {
                                        if (points == null)
                                        {
                                            throw new System.ArgumentNullException("points");
                                        }

                                        Points = new PointCollection(points);
                                        IsStroked = isStroked;
                                    }

                                    /// <summary>
                                    ///
                                    /// </summary>
                                    internal [[instance.ClassName.Name]](IEnumerable<Point> points, bool isStroked, bool isSmoothJoin)
                                    {
                                        if (points == null)
                                        {
                                            throw new System.ArgumentNullException("points");
                                        }

                                        Points = new PointCollection(points);
                                        IsStroked = isStroked;
                                        IsSmoothJoin = isSmoothJoin;
                                    }

                                    #endregion

                                    #region AddToFigure
                                    internal override void AddToFigure(
                                        Matrix matrix,          // The transformation matrix
                                        PathFigure figure,      // The figure to add to
                                        ref Point current)      // Out: Segment endpoint, not transformed
                                    {            
                                        PointCollection points = Points;
                                        
                                        if (points != null  && points.Count >= [[instance.VerifyCount]])
                                        {
                                            if (matrix.IsIdentity)
                                            {
                                                figure.Segments.Add(this);
                                            }
                                            else
                                            {
                                                PointCollection copy = new PointCollection();
                                                Point pt = new Point();
                                                int count = points.Count;             
                                                
                                                for (int i=0; i<count; i++)
                                                {
                                                    pt = points.Internal_GetItem(i);
                                                    pt *= matrix;
                                                    copy.Add(pt);
                                                }
                                                
                                                figure.Segments.Add(new [[instance.ClassName.Name]](copy, IsStroked, IsSmoothJoin));
                                            }
                                            current = points.Internal_GetItem(points.Count - 1);
                                        }
                                    }
                                    #endregion

                                    internal override bool IsEmpty()
                                    {
                                        return (Points == null) || (Points.Count < [[instance.VerifyCount]]);
                                    }

                                    internal override bool IsCurved()
                                    {
                                        return [[instance.IsCurved ? "!IsEmpty()" : "false"]];
                                    }

                                    #region Resource
                                    /// <summary>
                                    /// SerializeData - Serialize the contents of this Segment to the provided context.
                                    /// </summary>
                                    internal override void SerializeData(StreamGeometryContext ctx)
                                    {
                                        ctx.[[instance.TypeName]]To(Points, IsStroked, IsSmoothJoin);
                                    }                                    
                                    #endregion
                                }
                                #endregion
                            }
                        [[/inline]]
                        );
                }
            }

        }

        private List<PolySegmentTemplateInstance> Instances = new List<PolySegmentTemplateInstance>();
    }
}


