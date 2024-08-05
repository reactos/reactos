using System;
using System.Collections.Generic;
using Lextm.SharpSnmpLib.Mib.Elements.Entities;
using Lextm.SharpSnmpLib.Mib.Elements.Types;

namespace Lextm.SharpSnmpLib.Mib
{
    [Flags]
    public enum MibTreeNodeType
    {
        Unknown             = 0,
        Container           = (1 << 0),
        Scalar              = (1 << 1),
        Table               = (1 << 2),
        TableRow            = (1 << 3),
        TableCell           = (1 << 4),
        NotificationRelated = (1 << 5),
        ConformanceRelated  = (1 << 6)
    }


    public class MibTreeNode
    {
        private MibTreeNode       _parent     = null;
        private List<MibTreeNode> _childNodes = new List<MibTreeNode>();
        private IEntity           _entity     = null;
        private MibTreeNodeType   _nodeType   = MibTreeNodeType.Unknown;

        public MibTreeNode(MibTreeNode parent, IEntity entity)
        {
            _parent = parent;
            _entity = entity;
        }

        public MibTreeNode Parent
        {
            get { return _parent; }
        }

        public IEntity Entity
        {
            get { return _entity; }
        }

        public MibTreeNodeType NodeType
        {
            get { return _nodeType; }
        }

        public List<MibTreeNode> ChildNodes
        {
            get { return _childNodes; }
        }

        public MibTreeNode AddChild(IEntity element)
        {
            MibTreeNode result = new MibTreeNode(this, element);
            this.ChildNodes.Add(result);

            return result;
        }

        public void UpdateNodeType()
        {
            _nodeType = MibTreeNodeType.Unknown;

            if (_entity != null)
            {
                if ((_entity is OidValueAssignment) || (_entity is ObjectIdentity))
                {
                    _nodeType |= MibTreeNodeType.Container;
                    return;
                }
                else if (_childNodes.Count > 0)
                {
                    _nodeType |= MibTreeNodeType.Container;
                }

                if (_entity is ObjectType)
                {
                    ObjectType ot = _entity as ObjectType;

                    if (ot.Syntax is SequenceOf)
                    {
                        _nodeType |= MibTreeNodeType.Table;
                    }
                    else if (ot.Syntax is Sequence)
                    {
                        _nodeType |= MibTreeNodeType.TableRow;
                    }
                    else if ((_parent != null) && ((_parent.NodeType & MibTreeNodeType.TableRow) != 0))
                    {
                        _nodeType |= MibTreeNodeType.TableCell;
                        _nodeType |= MibTreeNodeType.Scalar;
                    }
                    else
                    {
                        _nodeType |= MibTreeNodeType.Scalar;
                    }
                }
                else if ((_entity is ModuleCompliance) || (_entity is ObjectGroup) || (_entity is NotificationGroup))
                {
                    _nodeType |= MibTreeNodeType.ConformanceRelated;
                }
                else if ((_entity is NotificationGroup) || (_entity is NotificationType))
                {
                    _nodeType |= MibTreeNodeType.NotificationRelated;
                }
            }
        }
    }
}
