// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Generate files for UCE master resources.
//

namespace MS.Internal.MilCodeGen.Helpers
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;

    public class CollectionHelper : GeneratorMethods
    {
        public static string UpdateResource(McgResource resource)
        {
            if (!resource.HasUnmanagedResource) return String.Empty;

            return
                [[inline]]
                    UpdateResource();
                [[/inline]];
        }

        public static string Collection_Clear(McgResource resource, string type)
        {
            if (resource.CollectionType.IsFreezable)
            {
                string onRemove = String.Empty;

                if (resource.IsCollectionOfHandles)
                {
                    return
                        [[inline]]
                            WritePreamble();

                            // As part of Clear()'ing the collection, we will iterate it and call
                            // OnFreezablePropertyChanged and OnRemove for each item.
                            // However, OnRemove assumes that the item to be removed has already been
                            // pulled from the underlying collection.  To statisfy this condition,
                            // we store the old collection and clear _collection before we call these methods.
                            // As Clear() semantics do not include TrimToFit behavior, we create the new
                            // collection storage at the same size as the previous.  This is to provide
                            // as close as possible the same perf characteristics as less complicated collections.
                            FrugalStructList<[[type]]> oldCollection = _collection;
                            _collection = new FrugalStructList<[[type]]>(_collection.Capacity);

                            for (int i = oldCollection.Count - 1; i >= 0; i--)
                            {
                                OnFreezablePropertyChanged(/* oldValue = */ oldCollection[i], /* newValue = */ null);

                                // Fire the OnRemove handlers for each item.  We're not ensuring that
                                // all OnRemove's get called if a resumable exception is thrown.
                                // At this time, these call-outs are not public, so we do not handle exceptions.
                                OnRemove( /* oldValue */ oldCollection[i]);
                            }
                        [[/inline]];
                }
                else
                {
                    return
                        [[inline]]
                            WritePreamble();

                            for (int i = _collection.Count - 1; i >= 0; i--)
                            {
                                OnFreezablePropertyChanged(/* oldValue = */ _collection[i], /* newValue = */ null);
                            }

                            _collection.Clear();

                            Debug.Assert(_collection.Count == 0);
                        [[/inline]];
                }
            }
            else
            {
                return
                    [[inline]]
                        WritePreamble();

                        _collection.Clear();
                    [[/inline]];
            }
        }

        public static string Collection_Insert(McgResource resource, string type, string index, string typedValue)
        {
            if (resource.CollectionType.IsFreezable)
            {
                string onInsert = String.Empty;

                if (resource.IsCollectionOfHandles)
                {
                    onInsert = [[inline]]OnInsert([[typedValue]]);[[/inline]];
                }

                return
                    [[inline]]
                        WritePreamble();

                        OnFreezablePropertyChanged(/* oldValue = */ null, /* newValue = */ [[typedValue]]);

                        _collection.Insert([[index]], [[typedValue]]);
                        [[onInsert]]
                    [[/inline]];
            }
            else
            {
                return
                    [[inline]]
                        WritePreamble();
                        _collection.Insert([[index]], [[typedValue]]);
                    [[/inline]];
            }
        }

        public static string Collection_Remove(McgResource resource, string type, string value)
        {
            if (resource.CollectionType.IsFreezable)
            {
                return
                    [[inline]]
                        WritePreamble();

                        // By design collections "succeed silently" if you attempt to remove an item
                        // not in the collection.  Therefore we need to first verify the old value exists
                        // before calling OnFreezablePropertyChanged.  Since we already need to locate
                        // the item in the collection we keep the index and use RemoveAt(...) to do
                        // the work.  (Windows OS #1016178)

                        // We use the public IndexOf to guard our UIContext since OnFreezablePropertyChanged
                        // is only called conditionally.  IList.IndexOf returns -1 if the value is not found.
                        int index = IndexOf([[value]]);

                        if (index >= 0)
                        {
                            [[type]] oldValue = _collection[index];

                            [[Collection_RemoveFreezableAt(resource, "oldValue", "index")]]

                            [[UpdateResource(resource)]]
                            ++_version;
                            WritePostscript();

                            return true;
                        }
                    [[/inline]];
            }
            else
            {
                return
                    [[inline]]
                        WritePreamble();
                        int index = IndexOf([[value]]);
                        if (index >= 0)
                        {
                            // we already have index from IndexOf so instead of using Remove,
                            // which will search the collection a second time, we'll use RemoveAt
                            _collection.RemoveAt(index);

                            ++_version;
                            WritePostscript();

                            return true;
                        }
                    [[/inline]];
            }
        }

        public static string Collection_RemoveAt(McgResource resource, string type, string index)
        {
            if (resource.CollectionType.IsFreezable)
            {
                return
                    [[inline]]
                        WritePreamble();

                        [[type]] oldValue = _collection[ [[index]] ];

                        [[Collection_RemoveFreezableAt(resource, "oldValue", index)]]
                    [[/inline]];
            }
            else
            {
                return
                    [[inline]]
                        WritePreamble();
                        _collection.RemoveAt([[index]]);
                    [[/inline]];
            }
        }

        // Helper used by Remove and RemoveAt for removing Freezables.
        private static string Collection_RemoveFreezableAt(McgResource resource, string oldValue, string index)
        {
            string onRemove = String.Empty;

            if (resource.IsCollectionOfHandles)
            {
                onRemove = [[inline]]OnRemove([[oldValue]]);[[/inline]];
            }

            return
                [[inline]]
                    OnFreezablePropertyChanged([[oldValue]], null);

                    _collection.RemoveAt([[index]]);

                    [[onRemove]]
                [[/inline]];
        }

        public static string Collection_CheckAllNotNullAndFirePropertyChanged(McgResource resource, string type, string collection)
        {
            if (resource.CollectionType.IsFreezable)
            {
                string onInsert = String.Empty;

                if (resource.IsCollectionOfHandles)
                {
                    onInsert = [[inline]]OnInsert(item);[[/inline]];
                }

                return
                    [[inline]]
                        foreach ([[type]] item in [[collection]])
                        {
                            if (item == null)
                            {
                                throw new System.ArgumentException(SR.Get(SRID.Collection_NoNull));
                            }
                            OnFreezablePropertyChanged(/* oldValue = */ null, item);
                            [[onInsert]]
                        }
                    [[/inline]];
            }
            else
                return String.Empty;
        }

        public static string Collection_Add(McgResource resource, string type, string value, string index)
        {
            if (index != String.Empty)
            {

                index = [[inline]][[index]] = [[/inline]];
            }

            if (resource.CollectionType.IsFreezable)
            {
                string onInsert = String.Empty;

                if (resource.IsCollectionOfHandles)
                {
                    onInsert = [[inline]]OnInsert(newValue);[[/inline]];
                }

                return
                    [[inline]]
                        [[type]] newValue = [[value]];
                        OnFreezablePropertyChanged(/* oldValue = */ null, newValue);
                        [[index]]_collection.Add(newValue);
                        [[onInsert]]
                    [[/inline]];
            }
            else
            {
                return
                    [[inline]]
                        [[index]]_collection.Add([[value]]);
                    [[/inline]];
            }
        }

        public static string Collection_CheckNullInsert(McgResource resource, string value)
        {
            if (resource.CollectionType.IsFreezable)
            {
                return
                    [[inline]]
                        if ([[value]] == null)
                        {
                            throw new System.ArgumentException(SR.Get(SRID.Collection_NoNull));
                        }
                    [[/inline]];
            }
            else
                return String.Empty;
        }

        public static string Collection_SetValue(McgResource resource, string type, string index, string typedValue)
        {
            if (resource.CollectionType.IsFreezable)
            {
                string onSet = String.Empty;

                if (resource.IsCollectionOfHandles)
                {
                    onSet = [[inline]]OnSet(oldValue, [[typedValue]]);[[/inline]];
                }

                return
                    [[inline]]
                        WritePreamble();

                        if (!Object.ReferenceEquals(_collection[ [[index]] ], [[typedValue]]))
                        {

                            [[type]] oldValue = _collection[ [[index]] ];
                            OnFreezablePropertyChanged(oldValue, [[typedValue]]);

                            _collection[ [[index]] ] = [[typedValue]];

                            [[onSet]]
                        }
                    [[/inline]];
            }
            else
            {
                return
                    [[inline]]
                        WritePreamble();
                        _collection[ [[index]] ] = [[typedValue]];
                    [[/inline]];
            }
        }

        public static string WriteCopyToTyped(McgResource resource)
        {
            String type = resource.CollectionType.ManagedName;

            return
                [[inline]]
                    /// <summary>
                    ///     Copies the elements of the collection into "array" starting at "index"
                    /// </summary>
                    public void CopyTo([[type]][] array, int index)
                    {
                        [[WriteCopyToCommon(resource)]]

                        _collection.CopyTo(array, index);
                    }
                [[/inline]];
        }

        public static string WriteCopyToArray(McgResource resource)
        {
            return
                [[inline]]
                    void ICollection.CopyTo(Array array, int index)
                    {
                        [[WriteCopyToCommon(resource)]]

                        if (array.Rank != 1)
                        {
                            throw new ArgumentException(SR.Get(SRID.Collection_BadRank));
                        }

                        // Elsewhere in the collection we throw an AE when the type is
                        // bad so we do it here as well to be consistent
                        try
                        {
                            int count = _collection.Count;
                            for (int i = 0; i < count; i++)
                            {
                                array.SetValue(_collection[i], index + i);
                            }
                        }
                        catch (InvalidCastException e)
                        {
                            throw new ArgumentException(SR.Get(SRID.Collection_BadDestArray, this.GetType().Name), e);
                        }
                    }
                [[/inline]];

        }

        public static string WriteCopyToCommon(McgResource resource)
        {
            return
                [[inline]]
                    ReadPreamble();

                    if (array == null)
                    {
                        throw new ArgumentNullException("array");
                    }

                    // This will not throw in the case that we are copying
                    // from an empty collection.  This is consistent with the
                    // BCL Collection implementations. (Windows 1587365)
                    if (index < 0  || (index + _collection.Count) > array.Length)
                    {
                        throw new ArgumentOutOfRangeException("index");
                    }
                [[/inline]];
        }

        public static string WriteCollectionSummary(McgResource resource)
        {
            if (!resource.IsCollection) return String.Empty;

            String summary = String.Empty;

            if (resource.CollectionType.IsValueType)
            {
                summary =
                    [[inline]]
                        A collection of [[resource.CollectionType.ManagedName]]s.
                    [[/inline]];
            }
            else
            {
                summary =
                    [[inline]]
                        A collection of [[resource.CollectionType.ManagedName]] objects.
                    [[/inline]];
            }

            return
                [[inline]]
                    /// <summary>
                    /// [[summary]]
                    /// </summary>
                [[/inline]];
        }

        public static string WriteCollectionConstructors(McgResource resource)
        {
            if (!resource.IsCollection) return String.Empty;

            String type = resource.CollectionType.ManagedName;

            return
                [[inline]]
                    /// <summary>
                    /// Initializes a new instance that is empty.
                    /// </summary>
                    public [[resource.Name]]()
                    {
                        _collection = new FrugalStructList<[[type]]>();
                    }

                    /// <summary>
                    /// Initializes a new instance that is empty and has the specified initial capacity.
                    /// </summary>
                    /// <param name="capacity"> int - The number of elements that the new list is initially capable of storing. </param>
                    public [[resource.Name]](int capacity)
                    {
                        _collection = new FrugalStructList<[[type]]>(capacity);
                    }

                    /// <summary>
                    /// Creates a [[resource.Name]] with all of the same elements as collection
                    /// </summary>
                    public [[resource.Name]](IEnumerable<[[type]]> collection)
                    {
                        // The WritePreamble and WritePostscript aren't technically necessary
                        // in the constructor as of 1/20/05 but they are put here in case
                        // their behavior changes at a later date

                        WritePreamble();
                
                        if (collection != null)
                        {
                            [[resource.CollectionType.IsFreezable ? "bool needsItemValidation = true;" : string.Empty]]
                            ICollection<[[type]]> icollectionOfT = collection as ICollection<[[type]]>;

                            if (icollectionOfT != null)
                            {
                                _collection = new FrugalStructList<[[type]]>(icollectionOfT);
                            }
                            else
                            {       
                                ICollection icollection = collection as ICollection;

                                if (icollection != null) // an IC but not and IC<T>
                                {
                                    _collection = new FrugalStructList<[[type]]>(icollection);
                                }
                                else // not a IC or IC<T> so fall back to the slower Add
                                {
                                    _collection = new FrugalStructList<[[type]]>();

                                    foreach ([[type]] item in collection)
                                    {
                                        [[Collection_CheckNullInsert(resource, "item")]]
                                        [[Collection_Add(resource, type, "item", String.Empty)]]
                                    }

                                    [[resource.CollectionType.IsFreezable ? "needsItemValidation = false;" : string.Empty]]
                                }
                            }

                            [[resource.CollectionType.IsFreezable ? "if (needsItemValidation)" : string.Empty]]
                            [[resource.CollectionType.IsFreezable ? "{" : string.Empty]]
                                [[Collection_CheckAllNotNullAndFirePropertyChanged(resource, type, "collection")]]
                            [[resource.CollectionType.IsFreezable ? "}" : string.Empty]]

                            [[UpdateResource(resource)]]
                            WritePostscript();
                        }
                        else
                        {
                            throw new ArgumentNullException("collection");
                        }
                    }
                [[/inline]];

        }

        public static string WriteCollectionEvents(McgResource resource)
        {
            if (!resource.CollectionType.IsFreezable) return String.Empty;

            if (resource.IsCollectionOfHandles)
            {
                // If the collection is nested in a resource, we expose these events so our
                // owner can addref/release the items on its channel.
                return
                    [[inline]]
                        internal event ItemInsertedHandler ItemInserted;
                        internal event ItemRemovedHandler ItemRemoved;

                        private void OnInsert(object item)
                        {
                            if (ItemInserted != null)
                            {
                                ItemInserted(this, item);
                            }
                        }

                        private void OnRemove(object oldValue)
                        {
                            if (ItemRemoved != null)
                            {
                                ItemRemoved(this, oldValue);
                            }
                        }

                        private void OnSet(object oldValue, object newValue)
                        {
                            OnInsert(newValue);
                            OnRemove(oldValue);
                        }
                    [[/inline]];
            }
            else
            {
                return String.Empty;
            }
        }

        public static string FreezeCore(McgResource resource)
        {
            if (!resource.IsCollection) return String.Empty;
            if (!resource.CollectionType.IsFreezable) return String.Empty;

            return
                [[inline]]
                    /// <summary>
                    /// Implementation of <see cref="System.Windows.Freezable.FreezeCore">Freezable.FreezeCore</see>.
                    /// </summary>
                    protected override bool FreezeCore(bool isChecking)
                    {
                        bool canFreeze = base.FreezeCore(isChecking);

                        int count = _collection.Count;
                        for (int i = 0; i < count && canFreeze; i++)
                        {
                            canFreeze &= Freezable.Freeze(_collection[i], isChecking);
                        }

                        return canFreeze;
                    }
                [[/inline]];
        }

        public static string CloneCollectionHelper(McgResource resource, string source, string value)
        {
            String type = resource.CollectionType.ManagedName;

            return
                [[inline]]
                    int count = [[source]]._collection.Count;

                    _collection = new FrugalStructList<[[type]]>(count);

                    for (int i = 0; i < count; i++)
                    {
                        [[Collection_Add(resource, type, value, "")]]
                    }
                    [[UpdateResource(resource)]]
                [[/inline]];
        }

        public static string CloneCollection(McgResource resource, string method, string argType, string source)
        {
            if (!resource.IsCollection) return String.Empty;

            String value;

            if (resource.CollectionType.IsFreezable)
            {
                value = [[inline]]([[resource.CollectionType.Name]]) [[source]]._collection[i].[[method]]()[[/inline]];
            }
            else
            {
                value = [[inline]][[source]]._collection[i][[/inline]];
            }

            return CloneCollectionHelper(resource, source, value);
        }

        public static string CallAggregateBooleanProperty(McgResource resource, string boolProperty)
        {
            if (!resource.IsCollection) return String.Empty;
            if (!resource.CollectionType.IsFreezable) return String.Empty;

            String type = resource.CollectionType.ManagedName;

            return
                [[inline]]
                    int count = _collection.Count;
                    for (int i = 0; i < count; i++)
                    {
                        if (_collection[i].[[boolProperty]])
                        {
                            return true;
                        }
                    }
                [[/inline]];
        }

        public static string CallAggregateMethod(McgResource resource, string method, string arguments)
        {
            if (!resource.IsCollection) return String.Empty;
            if (!resource.CollectionType.IsFreezable) return String.Empty;

            String type = resource.CollectionType.ManagedName;

            return
                [[inline]]
                    int count = _collection.Count;
                    for (int i = 0; i < count; i++)
                    {
                        _collection[i].[[method]]([[arguments]]);
                    }
                [[/inline]];
        }

        public static string WriteCollectionEnumerator(McgResource resource)
        {
            if (!resource.IsCollection) return String.Empty;

            String type = resource.CollectionType.ManagedName;

            return
                [[inline]]
                    #region Enumerator
                    /// <summary>
                    /// Enumerates the items in a [[type]]Collection
                    /// </summary>
                    public struct Enumerator : IEnumerator, IEnumerator<[[type]]>
                    {
                        #region Constructor

                        internal Enumerator([[resource.Name]] list)
                        {
                            Debug.Assert(list != null, "list may not be null.");

                            _list = list;
                            _version = list._version;
                            _index = -1;
                            _current = default([[type]]);
                        }

                        #endregion

                        #region Methods

                        void IDisposable.Dispose()
                        {

                        }

                        /// <summary>
                        /// Advances the enumerator to the next element of the collection.
                        /// </summary>
                        /// <returns>
                        /// true if the enumerator was successfully advanced to the next element,
                        /// false if the enumerator has passed the end of the collection.
                        /// </returns>
                        public bool MoveNext()
                        {
                            _list.ReadPreamble();

                            if (_version == _list._version)
                            {
                                if (_index > -2 && _index < _list._collection.Count - 1)
                                {
                                    _current = _list._collection[++_index];
                                    return true;
                                }
                                else
                                {
                                    _index = -2; // -2 indicates "past the end"
                                    return false;
                                }
                            }
                            else
                            {
                                throw new InvalidOperationException(SR.Get(SRID.Enumerator_CollectionChanged));
                            }
                        }

                        /// <summary>
                        /// Sets the enumerator to its initial position, which is before the
                        /// first element in the collection.
                        /// </summary>
                        public void Reset()
                        {
                            _list.ReadPreamble();

                            if (_version == _list._version)
                            {
                                _index = -1;
                            }
                            else
                            {
                                throw new InvalidOperationException(SR.Get(SRID.Enumerator_CollectionChanged));
                            }
                        }

                        #endregion

                        #region Properties

                        object IEnumerator.Current
                        {
                            get
                            {
                                return this.Current;
                            }
                        }

                        /// <summary>
                        /// Current element
                        ///
                        /// The behavior of IEnumerable&lt;T>.Current is undefined
                        /// before the first MoveNext and after we have walked
                        /// off the end of the list. However, the IEnumerable.Current
                        /// contract requires that we throw exceptions
                        /// </summary>
                        public [[type]] Current
                        {
                            get
                            {
                                if (_index > -1)
                                {
                                    return _current;
                                }
                                else if (_index == -1)
                                {
                                    throw new InvalidOperationException(SR.Get(SRID.Enumerator_NotStarted));
                                }
                                else
                                {
                                    Debug.Assert(_index == -2, "expected -2, got " + _index + "\n");
                                    throw new InvalidOperationException(SR.Get(SRID.Enumerator_ReachedEnd));
                                }
                            }
                        }

                        #endregion

                        #region Data
                        private [[type]] _current;
                        private [[resource.Name]] _list;
                        private uint _version;
                        private int _index;
                        #endregion
                    }
                    #endregion
                [[/inline]];
        }

        public static string WriteCollectionMethods(McgResource resource)
        {
            if (!resource.IsCollection) return String.Empty;

            String type = resource.CollectionType.ManagedName;

            String containsBody, indexOfBody, removeBody;
            if (!resource.CollectionType.IsValueType)
            {
                containsBody = [[inline]]return Contains(value as [[type]]);[[/inline]];
                indexOfBody = [[inline]]return IndexOf(value as [[type]]);[[/inline]];
                removeBody = [[inline]]Remove(value as [[type]]);[[/inline]];
            }
            else
            {
                containsBody =
                    [[inline]]
                        if (value is [[type]])
                        {
                            return Contains(([[type]])value);
                        }

                        return false;
                    [[/inline]];

                indexOfBody =
                    [[inline]]
                        if (value is [[type]])
                        {
                            return IndexOf(([[type]])value);
                        }

                        return -1;
                    [[/inline]];

                removeBody =
                    [[inline]]
                        if (value is [[type]])
                        {
                            Remove(([[type]])value);
                        }
                    [[/inline]];
            }

            String onInheritanceContextChangedCoreMethod = String.Empty;
            if (resource.CollectionType.IsFreezable)
            {
                onInheritanceContextChangedCoreMethod =
                    [[inline]]
                        /// <summary>
                        ///     Freezable collections need to notify their contained Freezables
                        ///     about the change in the InheritanceContext
                        /// </summary>
                        internal override void OnInheritanceContextChangedCore(EventArgs args)
                        {
                            base.OnInheritanceContextChangedCore(args);

                            for (int i=0; i<this.Count; i++)
                            {
                                DependencyObject inheritanceChild = _collection[i];
                                if (inheritanceChild!= null && inheritanceChild.InheritanceContext == this)
                                {
                                    inheritanceChild.OnInheritanceContextChanged(args);
                                }
                            }
                        }
                    [[/inline]];
            }

            return
                [[inline]]
                    #region IList<T>

                    /// <summary>
                    ///     Adds "value" to the list
                    /// </summary>
                    public void Add([[type]] value)
                    {
                        AddHelper(value);
                    }

                    /// <summary>
                    ///     Removes all elements from the list
                    /// </summary>
                    public void Clear()
                    {
                        [[Collection_Clear(resource,type)]]
                        [[UpdateResource(resource)]]
                        ++_version;
                        WritePostscript();
                    }

                    /// <summary>
                    ///     Determines if the list contains "value"
                    /// </summary>
                    public bool Contains([[type]] value)
                    {
                        ReadPreamble();

                        return _collection.Contains(value);
                    }

                    /// <summary>
                    ///     Returns the index of "value" in the list
                    /// </summary>
                    public int IndexOf([[type]] value)
                    {
                        ReadPreamble();

                        return _collection.IndexOf(value);
                    }

                    /// <summary>
                    ///     Inserts "value" into the list at the specified position
                    /// </summary>
                    public void Insert(int index, [[type]] value)
                    {
                        [[Collection_CheckNullInsert(resource, "value")]]

                        [[Collection_Insert(resource, type, "index", "value")]]
                        [[UpdateResource(resource)]]

                        ++_version;
                        WritePostscript();
                    }

                    /// <summary>
                    ///     Removes "value" from the list
                    /// </summary>
                    public bool Remove([[type]] value)
                    {
                        [[Collection_Remove(resource, type, "value")]]

                        // Collection_Remove returns true, calls WritePostscript,
                        // increments version, and does UpdateResource if it succeeds

                        return false;
                    }

                    /// <summary>
                    ///     Removes the element at the specified index
                    /// </summary>
                    public void RemoveAt(int index)
                    {
                        RemoveAtWithoutFiringPublicEvents(index);

                        // RemoveAtWithoutFiringPublicEvents incremented the version

                        WritePostscript();
                    }


                    /// <summary>
                    ///     Removes the element at the specified index without firing
                    ///     the public Changed event.
                    ///     The caller - typically a public method - is responsible for calling
                    ///     WritePostscript if appropriate.
                    /// </summary>
                    internal void RemoveAtWithoutFiringPublicEvents(int index)
                    {
                        [[Collection_RemoveAt(resource, type, "index")]]
                        [[UpdateResource(resource)]]

                        ++_version;

                        // No WritePostScript to avoid firing the Changed event.
                    }


                    /// <summary>
                    ///     Indexer for the collection
                    /// </summary>
                    public [[type]] this[int index]
                    {
                        get
                        {
                            ReadPreamble();

                            return _collection[index];
                        }
                        set
                        {
                            [[Collection_CheckNullInsert(resource, "value")]]

                            [[Collection_SetValue(resource, type, "index", "value")]]
                            [[UpdateResource(resource)]]

                            ++_version;
                            WritePostscript();
                        }
                    }

                    #endregion

                    #region ICollection<T>

                    /// <summary>
                    ///     The number of elements contained in the collection.
                    /// </summary>
                    public int Count
                    {
                        get
                        {
                            ReadPreamble();

                            return _collection.Count;
                        }
                    }

                    [[WriteCopyToTyped(resource)]]

                    bool ICollection<[[type]]>.IsReadOnly
                    {
                        get
                        {
                            ReadPreamble();

                            return IsFrozen;
                        }
                    }

                    #endregion

                    #region IEnumerable<T>

                    /// <summary>
                    /// Returns an enumerator for the collection
                    /// </summary>
                    public Enumerator GetEnumerator()
                    {
                        ReadPreamble();

                        return new Enumerator(this);
                    }

                    IEnumerator<[[type]]> IEnumerable<[[type]]>.GetEnumerator()
                    {
                        return this.GetEnumerator();
                    }

                    #endregion

                    #region IList

                    bool IList.IsReadOnly
                    {
                        get
                        {
                            return ((ICollection<[[type]]>)this).IsReadOnly;
                        }
                    }

                    bool IList.IsFixedSize
                    {
                        get
                        {
                            ReadPreamble();

                            return IsFrozen;
                        }
                    }

                    object IList.this[int index]
                    {
                        get
                        {
                            return this[index];
                        }
                        set
                        {
                            // Forwards to typed implementation
                            this[index] = Cast(value);
                        }
                    }

                    int IList.Add(object value)
                    {
                        // Forward to typed helper
                        return AddHelper(Cast(value));
                    }

                    bool IList.Contains(object value)
                    {
                        [[containsBody]]
                    }

                    int IList.IndexOf(object value)
                    {
                        [[indexOfBody]]
                    }

                    void IList.Insert(int index, object value)
                    {
                        // Forward to IList<T> Insert
                        Insert(index, Cast(value));
                    }

                    void IList.Remove(object value)
                    {
                        [[removeBody]]
                    }

                    #endregion

                    #region ICollection

                    [[WriteCopyToArray(resource)]]

                    bool ICollection.IsSynchronized
                    {
                        get
                        {
                            ReadPreamble();

                            return IsFrozen || Dispatcher != null;
                        }
                    }

                    object ICollection.SyncRoot
                    {
                        get
                        {
                            ReadPreamble();
                            return this;
                        }
                    }
                    #endregion

                    #region IEnumerable

                    IEnumerator IEnumerable.GetEnumerator()
                    {
                        return this.GetEnumerator();
                    }

                    #endregion

                    #region Internal Helpers

                    /// <summary>
                    /// A frozen empty [[resource.Name]].
                    /// </summary>
                    internal static [[resource.Name]] Empty
                    {
                        get
                        {
                            if (s_empty == null)
                            {
                                [[resource.Name]] collection = new [[resource.Name]]();
                                collection.Freeze();
                                s_empty = collection;
                            }

                            return s_empty;
                        }
                    }

                    /// <summary>
                    /// Helper to return read only access.
                    /// </summary>
                    internal [[type]] Internal_GetItem(int i)
                    {
                        return _collection[i];
                    }

                    [[onInheritanceContextChangedCoreMethod]]

                    #endregion

                    #region Private Helpers

                    private [[type]] Cast(object value)
                    {
                        if( value == null )
                        {
                            throw new System.ArgumentNullException("value");
                        }

                        if (!(value is [[type]]))
                        {
                            throw new System.ArgumentException(SR.Get(SRID.Collection_BadType, this.GetType().Name, value.GetType().Name, "[[type]]"));
                        }

                        return ([[type]]) value;
                    }

                    // IList.Add returns int and IList<T>.Add does not. This
                    // is called by both Adds and IList<T>'s just ignores the
                    // integer
                    private int AddHelper([[type]] value)
                    {
                        int index = AddWithoutFiringPublicEvents(value);

                        // AddAtWithoutFiringPublicEvents incremented the version

                        WritePostscript();

                        return index;
                    }

                    internal int AddWithoutFiringPublicEvents([[type]] value)
                    {
                        int index = -1;

                        [[Collection_CheckNullInsert(resource, "value")]]
                        WritePreamble();
                        [[Collection_Add(resource, type, "value", "index")]]
                        [[UpdateResource(resource)]]

                        ++_version;

                        // No WritePostScript to avoid firing the Changed event.

                        return index;
                    }

                    [[WriteCollectionEvents(resource)]]

                    #endregion Private Helpers

                    private static [[resource.Name]] s_empty;

                [[/inline]];
        }

        public static string WriteCollectionFields(McgResource resource)
        {
            if (!resource.IsCollection) return String.Empty;

            return
                [[inline]]
                    internal FrugalStructList<[[resource.CollectionType.ManagedName]]> _collection;
                    internal uint _version = 0;
                [[/inline]];
        }

    }
}



