// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// Description: This file contains the definition of template-based generation of 
//              the type-specific keyframe collections (ByteKeyFrameCollection, etc)
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
    /// <summary>
    /// KeyFrameCollectionTemplate: This class represents one instantiation of the KeyFrameCollection template.
    /// </summary>
    public class KeyFrameCollectionTemplate: Template
    {
        private struct KeyFrameCollectionTemplateInstance
        {
            public KeyFrameCollectionTemplateInstance(
                string moduleName,
                string typeName
                )
            {
                ModuleName = moduleName;
                TypeName = typeName;
            }

            public string ModuleName;
            public string TypeName;
        }

        /// <summary>
        /// AddTemplateInstance - This is called by the code which parses the generation control.
        /// It is called on each TemplateInstance XMLNode encountered.
        /// </summary>
        public override void AddTemplateInstance(ResourceModel resourceModel, XmlNode node)
        {
            Instances.Add(new KeyFrameCollectionTemplateInstance(
                ResourceModel.ToString(node, "ModuleName"),
                ResourceModel.ToString(node, "TypeName")));
        }

        public override void Go(ResourceModel resourceModel)
        {
            foreach (KeyFrameCollectionTemplateInstance instance in Instances)
            {
                string fileName = instance.TypeName + "KeyFrameCollection" + ".cs";
                string path = "src\\" + instance.ModuleName + "\\System\\Windows\\Media\\Animation\\Generated";
                string fullPath = Path.Combine(resourceModel.OutputDirectory, path);

                using (FileCodeSink csFile = new FileCodeSink(fullPath, fileName, true /* Create dir if necessary */))
                {
                    csFile.WriteBlock(
                        [[inline]]
                            [[Helpers.ManagedStyle.WriteFileHeader(fileName)]]

                            using MS.Internal;

                            using System;
                            using System.Collections;
                            using System.Collections.Generic;
                            using System.ComponentModel;
                            using System.Diagnostics;
                            using System.Globalization;
                            using System.Windows.Media.Animation;
                            using System.Windows.Media.Media3D;

                            namespace System.Windows.Media.Animation
                            {
                                /// <summary>
                                /// This collection is used in conjunction with a KeyFrame[[instance.TypeName]]Animation
                                /// to animate a [[instance.TypeName]] property value along a set of key frames.
                                /// </summary>
                                public class [[instance.TypeName]]KeyFrameCollection : Freezable, IList
                                {
                                    #region Data
                                    
                                    private List<[[instance.TypeName]]KeyFrame> _keyFrames;
                                    private static [[instance.TypeName]]KeyFrameCollection s_emptyCollection;
                                    
                                    #endregion
                                    
                                    #region Constructors
                                    
                                    /// <Summary>
                                    /// Creates a new [[instance.TypeName]]KeyFrameCollection.
                                    /// </Summary>
                                    public [[instance.TypeName]]KeyFrameCollection()
                                        : base()
                                    {
                                        _keyFrames = new List< [[instance.TypeName]]KeyFrame>(2);
                                    }
                                    
                                    #endregion
                                    
                                    #region Static Methods
                                    
                                    /// <summary>
                                    /// An empty [[instance.TypeName]]KeyFrameCollection.
                                    /// </summary>
                                    public static [[instance.TypeName]]KeyFrameCollection Empty
                                    {
                                        get
                                        {
                                            if (s_emptyCollection == null)
                                            {
                                                [[instance.TypeName]]KeyFrameCollection emptyCollection = new [[instance.TypeName]]KeyFrameCollection();
                                                
                                                emptyCollection._keyFrames = new List< [[instance.TypeName]]KeyFrame>(0);
                                                emptyCollection.Freeze();
                                                
                                                s_emptyCollection = emptyCollection;
                                            }
                                            
                                            return s_emptyCollection;
                                        }
                                    }
                                    
                                    #endregion
                                    
                                    #region Freezable
                                    
                                    /// <summary>
                                    /// Creates a freezable copy of this [[instance.TypeName]]KeyFrameCollection.
                                    /// </summary>
                                    /// <returns>The copy</returns>
                                    public new [[instance.TypeName]]KeyFrameCollection Clone()
                                    {
                                        return ([[instance.TypeName]]KeyFrameCollection)base.Clone();
                                    }
                                    
                                    /// <summary>
                                    /// Implementation of <see cref="System.Windows.Freezable.CreateInstanceCore">Freezable.CreateInstanceCore</see>.
                                    /// </summary>
                                    /// <returns>The new Freezable.</returns>
                                    protected override Freezable CreateInstanceCore()
                                    {
                                        return new [[instance.TypeName]]KeyFrameCollection();
                                    }
                                    
                                    /// <summary>
                                    /// Implementation of <see cref="System.Windows.Freezable.CloneCore(System.Windows.Freezable)">Freezable.CloneCore</see>.
                                    /// </summary>
                                    protected override void CloneCore(Freezable sourceFreezable)
                                    {
                                        [[instance.TypeName]]KeyFrameCollection sourceCollection = ([[instance.TypeName]]KeyFrameCollection) sourceFreezable;
                                        base.CloneCore(sourceFreezable);
                                        
                                        int count = sourceCollection._keyFrames.Count;

                                        _keyFrames = new List< [[instance.TypeName]]KeyFrame>(count);

                                        for (int i = 0; i < count; i++)
                                        {
                                            [[instance.TypeName]]KeyFrame keyFrame = ([[instance.TypeName]]KeyFrame)sourceCollection._keyFrames[i].Clone();
                                            _keyFrames.Add(keyFrame);
                                            OnFreezablePropertyChanged(null, keyFrame);
                                        }
                                    }
        
        
                                    /// <summary>
                                    /// Implementation of <see cref="System.Windows.Freezable.CloneCurrentValueCore(System.Windows.Freezable)">Freezable.CloneCurrentValueCore</see>.
                                    /// </summary>
                                    protected override void CloneCurrentValueCore(Freezable sourceFreezable)
                                    {
                                        [[instance.TypeName]]KeyFrameCollection sourceCollection = ([[instance.TypeName]]KeyFrameCollection) sourceFreezable;
                                        base.CloneCurrentValueCore(sourceFreezable);
                                        
                                        int count = sourceCollection._keyFrames.Count;

                                        _keyFrames = new List< [[instance.TypeName]]KeyFrame>(count);

                                        for (int i = 0; i < count; i++)
                                        {
                                            [[instance.TypeName]]KeyFrame keyFrame = ([[instance.TypeName]]KeyFrame)sourceCollection._keyFrames[i].CloneCurrentValue();
                                            _keyFrames.Add(keyFrame);
                                            OnFreezablePropertyChanged(null, keyFrame);
                                        }
                                    }
                                    
                                    
                                    /// <summary>
                                    /// Implementation of <see cref="System.Windows.Freezable.GetAsFrozenCore(System.Windows.Freezable)">Freezable.GetAsFrozenCore</see>.
                                    /// </summary>
                                    protected override void GetAsFrozenCore(Freezable sourceFreezable)
                                    {
                                        [[instance.TypeName]]KeyFrameCollection sourceCollection = ([[instance.TypeName]]KeyFrameCollection) sourceFreezable;
                                        base.GetAsFrozenCore(sourceFreezable);
                                        
                                        int count = sourceCollection._keyFrames.Count;

                                        _keyFrames = new List< [[instance.TypeName]]KeyFrame>(count);

                                        for (int i = 0; i < count; i++)
                                        {
                                            [[instance.TypeName]]KeyFrame keyFrame = ([[instance.TypeName]]KeyFrame)sourceCollection._keyFrames[i].GetAsFrozen();
                                            _keyFrames.Add(keyFrame);
                                            OnFreezablePropertyChanged(null, keyFrame);
                                        }
                                    }
                                    
                                    
                                    /// <summary>
                                    /// Implementation of <see cref="System.Windows.Freezable.GetCurrentValueAsFrozenCore(System.Windows.Freezable)">Freezable.GetCurrentValueAsFrozenCore</see>.
                                    /// </summary>
                                    protected override void GetCurrentValueAsFrozenCore(Freezable sourceFreezable)
                                    {
                                        [[instance.TypeName]]KeyFrameCollection sourceCollection = ([[instance.TypeName]]KeyFrameCollection) sourceFreezable;
                                        base.GetCurrentValueAsFrozenCore(sourceFreezable);
                                        
                                        int count = sourceCollection._keyFrames.Count;

                                        _keyFrames = new List< [[instance.TypeName]]KeyFrame>(count);

                                        for (int i = 0; i < count; i++)
                                        {
                                            [[instance.TypeName]]KeyFrame keyFrame = ([[instance.TypeName]]KeyFrame)sourceCollection._keyFrames[i].GetCurrentValueAsFrozen();
                                            _keyFrames.Add(keyFrame);
                                            OnFreezablePropertyChanged(null, keyFrame);
                                        }
                                    }

                                    /// <summary>
                                    ///
                                    /// </summary>
                                    protected override bool FreezeCore(bool isChecking)
                                    {
                                        bool canFreeze = base.FreezeCore(isChecking);

                                        for (int i = 0; i < _keyFrames.Count && canFreeze; i++)
                                        {
                                            canFreeze &= Freezable.Freeze(_keyFrames[i], isChecking);
                                        }

                                        return canFreeze;
                                    }
                                    
                                    #endregion
                                    
                                    #region IEnumerable
                                    
                                    /// <summary>
                                    /// Returns an enumerator of the [[instance.TypeName]]KeyFrames in the collection.
                                    /// </summary>
                                    public IEnumerator GetEnumerator()
                                    {
                                        ReadPreamble();
                                        
                                        return _keyFrames.GetEnumerator();
                                    }
                                    
                                    #endregion
                                    
                                    #region ICollection
                                    
                                    /// <summary>
                                    /// Returns the number of [[instance.TypeName]]KeyFrames in the collection.
                                    /// </summary>
                                    public int Count
                                    {
                                        get
                                        {
                                            ReadPreamble();
                                            
                                            return _keyFrames.Count;
                                        }
                                    }
                                    
                                    /// <summary>
                                    /// See <see cref="System.Collections.ICollection.IsSynchronized">ICollection.IsSynchronized</see>.
                                    /// </summary>
                                    public bool IsSynchronized
                                    {
                                        get
                                        {
                                            ReadPreamble();
                                            
                                            return (IsFrozen || Dispatcher != null);
                                        }
                                    }
                                    
                                    /// <summary>
                                    /// See <see cref="System.Collections.ICollection.SyncRoot">ICollection.SyncRoot</see>.
                                    /// </summary>
                                    public object SyncRoot
                                    {
                                        get
                                        {
                                            ReadPreamble();
                                            
                                            return ((ICollection)_keyFrames).SyncRoot;
                                        }
                                    }
                                    
                                    /// <summary>
                                    /// Copies all of the [[instance.TypeName]]KeyFrames in the collection to an
                                    /// array.
                                    /// </summary>
                                    void ICollection.CopyTo(Array array, int index)
                                    {
                                        ReadPreamble();
                                        
                                        ((ICollection)_keyFrames).CopyTo(array, index);
                                    }
                                    
                                    /// <summary>
                                    /// Copies all of the [[instance.TypeName]]KeyFrames in the collection to an
                                    /// array of [[instance.TypeName]]KeyFrames.
                                    /// </summary>
                                    public void CopyTo([[instance.TypeName]]KeyFrame[] array, int index)
                                    {
                                        ReadPreamble();
                                        
                                        _keyFrames.CopyTo(array, index);
                                    }
                                    
                                    #endregion
                                    
                                    #region IList
                                    
                                    /// <summary>
                                    /// Adds a [[instance.TypeName]]KeyFrame to the collection.
                                    /// </summary>
                                    int IList.Add(object keyFrame)
                                    {
                                        return Add(([[instance.TypeName]]KeyFrame)keyFrame);
                                    }
                                    
                                    /// <summary>
                                    /// Adds a [[instance.TypeName]]KeyFrame to the collection.
                                    /// </summary>
                                    public int Add([[instance.TypeName]]KeyFrame keyFrame)
                                    {
                                        if (keyFrame == null)
                                        {
                                            throw new ArgumentNullException("keyFrame");
                                        }

                                        WritePreamble();
                                        
                                        OnFreezablePropertyChanged(null, keyFrame);
                                        _keyFrames.Add(keyFrame);
                                        
                                        WritePostscript();
                                        
                                        return _keyFrames.Count - 1;
                                    }
                                    
                                    /// <summary>
                                    /// Removes all [[instance.TypeName]]KeyFrames from the collection.
                                    /// </summary>
                                    public void Clear()
                                    {
                                        WritePreamble();

                                        if (_keyFrames.Count > 0)
                                        {            
                                            for (int i = 0; i < _keyFrames.Count; i++)
                                            {
                                                OnFreezablePropertyChanged(_keyFrames[i], null);
                                            }
                                            
                                            _keyFrames.Clear();
                                            
                                            WritePostscript();
                                        }
                                    }
                                    
                                    /// <summary>
                                    /// Returns true of the collection contains the given [[instance.TypeName]]KeyFrame.
                                    /// </summary>
                                    bool IList.Contains(object keyFrame)
                                    {
                                        return Contains(([[instance.TypeName]]KeyFrame)keyFrame);
                                    }
                                    
                                    /// <summary>
                                    /// Returns true of the collection contains the given [[instance.TypeName]]KeyFrame.
                                    /// </summary>
                                    public bool Contains([[instance.TypeName]]KeyFrame keyFrame)
                                    {
                                        ReadPreamble();
                                        
                                        return _keyFrames.Contains(keyFrame);
                                    }
                                    
                                    /// <summary>
                                    /// Returns the index of a given [[instance.TypeName]]KeyFrame in the collection. 
                                    /// </summary>
                                    int IList.IndexOf(object keyFrame)
                                    {
                                        return IndexOf(([[instance.TypeName]]KeyFrame)keyFrame);
                                    }
                                    
                                    /// <summary>
                                    /// Returns the index of a given [[instance.TypeName]]KeyFrame in the collection. 
                                    /// </summary>
                                    public int IndexOf([[instance.TypeName]]KeyFrame keyFrame)
                                    {
                                        ReadPreamble();
                                        
                                        return _keyFrames.IndexOf(keyFrame);
                                    }
                                    
                                    /// <summary>
                                    /// Inserts a [[instance.TypeName]]KeyFrame into a specific location in the collection. 
                                    /// </summary>
                                    void IList.Insert(int index, object keyFrame)
                                    {
                                        Insert(index, ([[instance.TypeName]]KeyFrame)keyFrame);
                                    }
                                    
                                    /// <summary>
                                    /// Inserts a [[instance.TypeName]]KeyFrame into a specific location in the collection. 
                                    /// </summary>
                                    public void Insert(int index, [[instance.TypeName]]KeyFrame keyFrame)
                                    {
                                        if (keyFrame == null)
                                        {
                                            throw new ArgumentNullException("keyFrame");
                                        }

                                        WritePreamble();
                                        
                                        OnFreezablePropertyChanged(null, keyFrame);
                                        _keyFrames.Insert(index, keyFrame);
                                        
                                        WritePostscript();
                                    }
                                    
                                    /// <summary>
                                    /// Returns true if the collection is frozen.
                                    /// </summary>
                                    public bool IsFixedSize
                                    {
                                        get
                                        {
                                            ReadPreamble();
                                            
                                            return IsFrozen;
                                        }
                                    }
                                    
                                    /// <summary>
                                    /// Returns true if the collection is frozen.
                                    /// </summary>
                                    public bool IsReadOnly
                                    {
                                        get
                                        {
                                            ReadPreamble();
                                            
                                            return IsFrozen;
                                        }
                                    }
                                    
                                    /// <summary>
                                    /// Removes a [[instance.TypeName]]KeyFrame from the collection.
                                    /// </summary>
                                    void IList.Remove(object keyFrame)
                                    {
                                        Remove(([[instance.TypeName]]KeyFrame)keyFrame);
                                    }
                                    
                                    /// <summary>
                                    /// Removes a [[instance.TypeName]]KeyFrame from the collection.
                                    /// </summary>
                                    public void Remove([[instance.TypeName]]KeyFrame keyFrame)
                                    {
                                        WritePreamble();
                                        
                                        if (_keyFrames.Contains(keyFrame))
                                        {
                                            OnFreezablePropertyChanged(keyFrame, null);
                                            _keyFrames.Remove(keyFrame);
                                            
                                            WritePostscript();
                                        }
                                    }
                                    
                                    /// <summary>
                                    /// Removes the [[instance.TypeName]]KeyFrame at the specified index from the collection.
                                    /// </summary>
                                    public void RemoveAt(int index)
                                    {
                                        WritePreamble();
                                        
                                        OnFreezablePropertyChanged(_keyFrames[index], null);
                                        _keyFrames.RemoveAt(index);
                                        
                                        WritePostscript();
                                    }
                                    
                                    /// <summary>
                                    /// Gets or sets the [[instance.TypeName]]KeyFrame at a given index.
                                    /// </summary>
                                    object IList.this[int index]
                                    {
                                        get
                                        {
                                            return this[index];
                                        }
                                        set
                                        {
                                            this[index] = ([[instance.TypeName]]KeyFrame)value;
                                        }
                                    }
                                    
                                    /// <summary>
                                    /// Gets or sets the [[instance.TypeName]]KeyFrame at a given index.
                                    /// </summary>
                                    public [[instance.TypeName]]KeyFrame this[int index]
                                    {
                                        get
                                        {
                                            ReadPreamble();
                                            
                                            return _keyFrames[index];
                                        }
                                        set
                                        {
                                            if (value == null)
                                            {
                                                throw new ArgumentNullException(String.Format(CultureInfo.InvariantCulture, "[[instance.TypeName]]KeyFrameCollection[{0}]", index));
                                            }
                                            
                                            WritePreamble();
                                            
                                            if (value != _keyFrames[index])
                                            {
                                                OnFreezablePropertyChanged(_keyFrames[index], value);
                                                _keyFrames[index] = value;
                                                                    
                                                Debug.Assert(_keyFrames[index] != null);
                                                
                                                WritePostscript();
                                            }
                                        }
                                    }
                                    
                                    #endregion
                                }
                            }
                        [[/inline]]
                        );
                }
            }
        }

        private List<KeyFrameCollectionTemplateInstance> Instances = new List<KeyFrameCollectionTemplateInstance>();
    }
}


