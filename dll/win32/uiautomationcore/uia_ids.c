/*
 * Copyright 2022 Connor McAdams for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "uia_private.h"
#include "ocidl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

static int __cdecl uia_property_guid_compare(const void *a, const void *b)
{
    const GUID *guid = a;
    const struct uia_prop_info *property = b;
    return memcmp(guid, property->guid, sizeof(*guid));
}

static int __cdecl uia_event_guid_compare(const void *a, const void *b)
{
    const GUID *guid = a;
    const struct uia_event_info *event = b;
    return memcmp(guid, event->guid, sizeof(*guid));
}

static int __cdecl uia_pattern_guid_compare(const void *a, const void *b)
{
    const GUID *guid = a;
    const struct uia_pattern_info *pattern = b;
    return memcmp(guid, pattern->guid, sizeof(*guid));
}

static int __cdecl uia_control_type_guid_compare(const void *a, const void *b)
{
    const GUID *guid = a;
    const struct uia_control_type_info *control = b;
    return memcmp(guid, control->guid, sizeof(*guid));
}

/* Sorted by GUID. */
static const struct uia_prop_info default_uia_properties[] = {
    { &AutomationId_Property_GUID,                       UIA_AutomationIdPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &FrameworkId_Property_GUID,                        UIA_FrameworkIdPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &IsTransformPatternAvailable_Property_GUID,        UIA_IsTransformPatternAvailablePropertyId, },
    { &IsScrollItemPatternAvailable_Property_GUID,       UIA_IsScrollItemPatternAvailablePropertyId, },
    { &IsExpandCollapsePatternAvailable_Property_GUID,   UIA_IsExpandCollapsePatternAvailablePropertyId, },
    { &CenterPoint_Property_GUID,                        UIA_CenterPointPropertyId, },
    { &IsTableItemPatternAvailable_Property_GUID,        UIA_IsTableItemPatternAvailablePropertyId, },
    { &Scroll_HorizontalScrollPercent_Property_GUID,     UIA_ScrollHorizontalScrollPercentPropertyId, },
    { &AccessKey_Property_GUID,                          UIA_AccessKeyPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &RangeValue_Maximum_Property_GUID,                 UIA_RangeValueMaximumPropertyId, },
    { &ClassName_Property_GUID,                          UIA_ClassNamePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &Transform2_ZoomMinimum_Property_GUID,             UIA_Transform2ZoomMinimumPropertyId, },
    { &LegacyIAccessible_Description_Property_GUID,      UIA_LegacyIAccessibleDescriptionPropertyId, },
    { &Transform2_ZoomLevel_Property_GUID,               UIA_Transform2ZoomLevelPropertyId, },
    { &Name_Property_GUID,                               UIA_NamePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &GridItem_RowSpan_Property_GUID,                   UIA_GridItemRowSpanPropertyId, },
    { &Size_Property_GUID,                               UIA_SizePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_DoubleArray, },
    { &IsTextPattern2Available_Property_GUID,            UIA_IsTextPattern2AvailablePropertyId, },
    { &Styles_FillPatternStyle_Property_GUID,            UIA_StylesFillPatternStylePropertyId, },
    { &FlowsTo_Property_GUID,                            UIA_FlowsToPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_ElementArray, },
    { &ItemStatus_Property_GUID,                         UIA_ItemStatusPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &Scroll_VerticalViewSize_Property_GUID,            UIA_ScrollVerticalViewSizePropertyId, },
    { &Selection_IsSelectionRequired_Property_GUID,      UIA_SelectionIsSelectionRequiredPropertyId, },
    { &IsGridItemPatternAvailable_Property_GUID,         UIA_IsGridItemPatternAvailablePropertyId, },
    { &Window_CanMinimize_Property_GUID,                 UIA_WindowCanMinimizePropertyId, },
    { &RangeValue_LargeChange_Property_GUID,             UIA_RangeValueLargeChangePropertyId, },
    { &Selection2_CurrentSelectedItem_Property_GUID,     UIA_Selection2CurrentSelectedItemPropertyId, },
    { &Culture_Property_GUID,                            UIA_CulturePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &LegacyIAccessible_DefaultAction_Property_GUID,    UIA_LegacyIAccessibleDefaultActionPropertyId, },
    { &Level_Property_GUID,                              UIA_LevelPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &IsKeyboardFocusable_Property_GUID,                UIA_IsKeyboardFocusablePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Bool, },
    { &GridItem_Row_Property_GUID,                       UIA_GridItemRowPropertyId, },
    { &IsSpreadsheetItemPatternAvailable_Property_GUID,  UIA_IsSpreadsheetItemPatternAvailablePropertyId, },
    { &Table_ColumnHeaders_Property_GUID,                UIA_TableColumnHeadersPropertyId, },
    { &Drag_GrabbedItems_Property_GUID,                  UIA_DragGrabbedItemsPropertyId, },
    { &Annotation_Target_Property_GUID,                  UIA_AnnotationTargetPropertyId, },
    { &IsSelectionItemPatternAvailable_Property_GUID,    UIA_IsSelectionItemPatternAvailablePropertyId, },
    { &IsDropTargetPatternAvailable_Property_GUID,       UIA_IsDropTargetPatternAvailablePropertyId, },
    { &Dock_DockPosition_Property_GUID,                  UIA_DockDockPositionPropertyId, },
    { &Styles_StyleId_Property_GUID,                     UIA_StylesStyleIdPropertyId, },
    { &Value_IsReadOnly_Property_GUID,                   UIA_ValueIsReadOnlyPropertyId,
      PROP_TYPE_PATTERN_PROP,                            UIAutomationType_Bool,
      UIA_ValuePatternId, },
    { &IsSpreadsheetPatternAvailable_Property_GUID,      UIA_IsSpreadsheetPatternAvailablePropertyId, },
    { &Styles_StyleName_Property_GUID,                   UIA_StylesStyleNamePropertyId, },
    { &IsAnnotationPatternAvailable_Property_GUID,       UIA_IsAnnotationPatternAvailablePropertyId, },
    { &SpreadsheetItem_AnnotationObjects_Property_GUID,  UIA_SpreadsheetItemAnnotationObjectsPropertyId, },
    { &IsInvokePatternAvailable_Property_GUID,           UIA_IsInvokePatternAvailablePropertyId, },
    { &HasKeyboardFocus_Property_GUID,                   UIA_HasKeyboardFocusPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Bool, },
    { &ClickablePoint_Property_GUID,                     UIA_ClickablePointPropertyId, },
    { &NewNativeWindowHandle_Property_GUID,              UIA_NativeWindowHandlePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &SizeOfSet_Property_GUID,                          UIA_SizeOfSetPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &LegacyIAccessible_Name_Property_GUID,             UIA_LegacyIAccessibleNamePropertyId, },
    { &Window_CanMaximize_Property_GUID,                 UIA_WindowCanMaximizePropertyId, },
    { &Scroll_HorizontallyScrollable_Property_GUID,      UIA_ScrollHorizontallyScrollablePropertyId, },
    { &ExpandCollapse_ExpandCollapseState_Property_GUID, UIA_ExpandCollapseExpandCollapseStatePropertyId, },
    { &Transform_CanRotate_Property_GUID,                UIA_TransformCanRotatePropertyId, },
    { &IsRangeValuePatternAvailable_Property_GUID,       UIA_IsRangeValuePatternAvailablePropertyId, },
    { &IsScrollPatternAvailable_Property_GUID,           UIA_IsScrollPatternAvailablePropertyId, },
    { &IsTransformPattern2Available_Property_GUID,       UIA_IsTransformPattern2AvailablePropertyId, },
    { &LabeledBy_Property_GUID,                          UIA_LabeledByPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Element, },
    { &ItemType_Property_GUID,                           UIA_ItemTypePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &Transform_CanMove_Property_GUID,                  UIA_TransformCanMovePropertyId, },
    { &LocalizedControlType_Property_GUID,               UIA_LocalizedControlTypePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &Annotation_AnnotationTypeId_Property_GUID,        UIA_AnnotationAnnotationTypeIdPropertyId, },
    { &FlowsFrom_Property_GUID,                          UIA_FlowsFromPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_ElementArray, },
    { &OptimizeForVisualContent_Property_GUID,           UIA_OptimizeForVisualContentPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Bool, },
    { &IsVirtualizedItemPatternAvailable_Property_GUID,  UIA_IsVirtualizedItemPatternAvailablePropertyId, },
    { &GridItem_Parent_Property_GUID,                    UIA_GridItemContainingGridPropertyId, },
    { &LegacyIAccessible_Help_Property_GUID,             UIA_LegacyIAccessibleHelpPropertyId, },
    { &Toggle_ToggleState_Property_GUID,                 UIA_ToggleToggleStatePropertyId, },
    { &IsTogglePatternAvailable_Property_GUID,           UIA_IsTogglePatternAvailablePropertyId, },
    { &LegacyIAccessible_State_Property_GUID,            UIA_LegacyIAccessibleStatePropertyId, },
    { &PositionInSet_Property_GUID,                      UIA_PositionInSetPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &RangeValue_IsReadOnly_Property_GUID,              UIA_RangeValueIsReadOnlyPropertyId, },
    { &Drag_DropEffects_Property_GUID,                   UIA_DragDropEffectsPropertyId, },
    { &RangeValue_SmallChange_Property_GUID,             UIA_RangeValueSmallChangePropertyId, },
    { &IsTextEditPatternAvailable_Property_GUID,         UIA_IsTextEditPatternAvailablePropertyId, },
    { &GridItem_Column_Property_GUID,                    UIA_GridItemColumnPropertyId, },
    { &LegacyIAccessible_ChildId_Property_GUID,          UIA_LegacyIAccessibleChildIdPropertyId,
      PROP_TYPE_PATTERN_PROP,                            UIAutomationType_Int,
      UIA_LegacyIAccessiblePatternId, },
    { &Annotation_DateTime_Property_GUID,                UIA_AnnotationDateTimePropertyId, },
    { &IsTablePatternAvailable_Property_GUID,            UIA_IsTablePatternAvailablePropertyId, },
    { &SelectionItem_IsSelected_Property_GUID,           UIA_SelectionItemIsSelectedPropertyId, },
    { &Window_WindowVisualState_Property_GUID,           UIA_WindowWindowVisualStatePropertyId, },
    { &IsOffscreen_Property_GUID,                        UIA_IsOffscreenPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Bool, },
    { &Annotation_Author_Property_GUID,                  UIA_AnnotationAuthorPropertyId, },
    { &Orientation_Property_GUID,                        UIA_OrientationPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &Value_Value_Property_GUID,                        UIA_ValueValuePropertyId, },
    { &VisualEffects_Property_GUID,                      UIA_VisualEffectsPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &Selection2_FirstSelectedItem_Property_GUID,       UIA_Selection2FirstSelectedItemPropertyId, },
    { &IsGridPatternAvailable_Property_GUID,             UIA_IsGridPatternAvailablePropertyId, },
    { &SelectionItem_SelectionContainer_Property_GUID,   UIA_SelectionItemSelectionContainerPropertyId, },
    { &HeadingLevel_Property_GUID,                       UIA_HeadingLevelPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &DropTarget_DropTargetEffect_Property_GUID,        UIA_DropTargetDropTargetEffectPropertyId, },
    { &Grid_ColumnCount_Property_GUID,                   UIA_GridColumnCountPropertyId, },
    { &AnnotationTypes_Property_GUID,                    UIA_AnnotationTypesPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_IntArray, },
    { &IsPeripheral_Property_GUID,                       UIA_IsPeripheralPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Bool, },
    { &Transform2_ZoomMaximum_Property_GUID,             UIA_Transform2ZoomMaximumPropertyId, },
    { &Drag_DropEffect_Property_GUID,                    UIA_DragDropEffectPropertyId, },
    { &MultipleView_CurrentView_Property_GUID,           UIA_MultipleViewCurrentViewPropertyId, },
    { &Styles_FillColor_Property_GUID,                   UIA_StylesFillColorPropertyId, },
    { &Rotation_Property_GUID,                           UIA_RotationPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Double, },
    { &SpreadsheetItem_Formula_Property_GUID,            UIA_SpreadsheetItemFormulaPropertyId, },
    { &IsEnabled_Property_GUID,                          UIA_IsEnabledPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Bool, },
    { &LocalizedLandmarkType_Property_GUID,              UIA_LocalizedLandmarkTypePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &IsDataValidForForm_Property_GUID,                 UIA_IsDataValidForFormPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Bool, },
    { &IsControlElement_Property_GUID,                   UIA_IsControlElementPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Bool, },
    { &HelpText_Property_GUID,                           UIA_HelpTextPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &Table_RowHeaders_Property_GUID,                   UIA_TableRowHeadersPropertyId, },
    { &ControllerFor_Property_GUID,                      UIA_ControllerForPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_ElementArray, },
    { &ProviderDescription_Property_GUID,                UIA_ProviderDescriptionPropertyId,
      PROP_TYPE_SPECIAL,                                 UIAutomationType_String, },
    { &AriaProperties_Property_GUID,                     UIA_AriaPropertiesPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &LiveSetting_Property_GUID,                        UIA_LiveSettingPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &Selection2_LastSelectedItem_Property_GUID,        UIA_Selection2LastSelectedItemPropertyId, },
    { &Transform2_CanZoom_Property_GUID,                 UIA_Transform2CanZoomPropertyId, },
    { &Window_IsModal_Property_GUID,                     UIA_WindowIsModalPropertyId, },
    { &Annotation_AnnotationTypeName_Property_GUID,      UIA_AnnotationAnnotationTypeNamePropertyId, },
    { &AriaRole_Property_GUID,                           UIA_AriaRolePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &Scroll_VerticallyScrollable_Property_GUID,        UIA_ScrollVerticallyScrollablePropertyId, },
    { &RangeValue_Value_Property_GUID,                   UIA_RangeValueValuePropertyId, },
    { &ProcessId_Property_GUID,                          UIA_ProcessIdPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &Scroll_VerticalScrollPercent_Property_GUID,       UIA_ScrollVerticalScrollPercentPropertyId, },
    { &IsObjectModelPatternAvailable_Property_GUID,      UIA_IsObjectModelPatternAvailablePropertyId, },
    { &IsDialog_Property_GUID,                           UIA_IsDialogPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Bool, },
    { &IsTextPatternAvailable_Property_GUID,             UIA_IsTextPatternAvailablePropertyId, },
    { &LegacyIAccessible_Role_Property_GUID,             UIA_LegacyIAccessibleRolePropertyId,
      PROP_TYPE_PATTERN_PROP,                            UIAutomationType_Int,
      UIA_LegacyIAccessiblePatternId, },
    { &Selection2_ItemCount_Property_GUID,               UIA_Selection2ItemCountPropertyId, },
    { &TableItem_RowHeaderItems_Property_GUID,           UIA_TableItemRowHeaderItemsPropertyId, },
    { &Styles_ExtendedProperties_Property_GUID,          UIA_StylesExtendedPropertiesPropertyId, },
    { &Selection_Selection_Property_GUID,                UIA_SelectionSelectionPropertyId, },
    { &TableItem_ColumnHeaderItems_Property_GUID,        UIA_TableItemColumnHeaderItemsPropertyId, },
    { &Window_WindowInteractionState_Property_GUID,      UIA_WindowWindowInteractionStatePropertyId, },
    { &Selection_CanSelectMultiple_Property_GUID,        UIA_SelectionCanSelectMultiplePropertyId, },
    { &Transform_CanResize_Property_GUID,                UIA_TransformCanResizePropertyId, },
    { &IsValuePatternAvailable_Property_GUID,            UIA_IsValuePatternAvailablePropertyId, },
    { &IsItemContainerPatternAvailable_Property_GUID,    UIA_IsItemContainerPatternAvailablePropertyId, },
    { &IsContentElement_Property_GUID,                   UIA_IsContentElementPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Bool, },
    { &LegacyIAccessible_KeyboardShortcut_Property_GUID, UIA_LegacyIAccessibleKeyboardShortcutPropertyId, },
    { &IsPassword_Property_GUID,                         UIA_IsPasswordPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Bool, },
    { &IsWindowPatternAvailable_Property_GUID,           UIA_IsWindowPatternAvailablePropertyId, },
    { &RangeValue_Minimum_Property_GUID,                 UIA_RangeValueMinimumPropertyId, },
    { &BoundingRectangle_Property_GUID,                  UIA_BoundingRectanglePropertyId,
      PROP_TYPE_SPECIAL,                                 UIAutomationType_Rect, },
    { &LegacyIAccessible_Value_Property_GUID,            UIA_LegacyIAccessibleValuePropertyId, },
    { &IsDragPatternAvailable_Property_GUID,             UIA_IsDragPatternAvailablePropertyId, },
    { &DescribedBy_Property_GUID,                        UIA_DescribedByPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_ElementArray, },
    { &IsSelectionPatternAvailable_Property_GUID,        UIA_IsSelectionPatternAvailablePropertyId, },
    { &Grid_RowCount_Property_GUID,                      UIA_GridRowCountPropertyId, },
    { &OutlineColor_Property_GUID,                       UIA_OutlineColorPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_IntArray, },
    { &Table_RowOrColumnMajor_Property_GUID,             UIA_TableRowOrColumnMajorPropertyId, },
    { &IsDockPatternAvailable_Property_GUID,             UIA_IsDockPatternAvailablePropertyId, },
    { &IsSynchronizedInputPatternAvailable_Property_GUID,UIA_IsSynchronizedInputPatternAvailablePropertyId, },
    { &OutlineThickness_Property_GUID,                   UIA_OutlineThicknessPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_DoubleArray, },
    { &IsLegacyIAccessiblePatternAvailable_Property_GUID,UIA_IsLegacyIAccessiblePatternAvailablePropertyId, },
    { &AnnotationObjects_Property_GUID,                  UIA_AnnotationObjectsPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_ElementArray, },
    { &IsRequiredForForm_Property_GUID,                  UIA_IsRequiredForFormPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Bool, },
    { &SpreadsheetItem_AnnotationTypes_Property_GUID,    UIA_SpreadsheetItemAnnotationTypesPropertyId, },
    { &FillColor_Property_GUID,                          UIA_FillColorPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &IsStylesPatternAvailable_Property_GUID,           UIA_IsStylesPatternAvailablePropertyId, },
    { &Window_IsTopmost_Property_GUID,                   UIA_WindowIsTopmostPropertyId, },
    { &IsCustomNavigationPatternAvailable_Property_GUID, UIA_IsCustomNavigationPatternAvailablePropertyId, },
    { &Scroll_HorizontalViewSize_Property_GUID,          UIA_ScrollHorizontalViewSizePropertyId, },
    { &AcceleratorKey_Property_GUID,                     UIA_AcceleratorKeyPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
    { &IsTextChildPatternAvailable_Property_GUID,        UIA_IsTextChildPatternAvailablePropertyId, },
    { &LegacyIAccessible_Selection_Property_GUID,        UIA_LegacyIAccessibleSelectionPropertyId, },
    { &FillType_Property_GUID,                           UIA_FillTypePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &ControlType_Property_GUID,                        UIA_ControlTypePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &IsMultipleViewPatternAvailable_Property_GUID,     UIA_IsMultipleViewPatternAvailablePropertyId, },
    { &DropTarget_DropTargetEffects_Property_GUID,       UIA_DropTargetDropTargetEffectsPropertyId, },
    { &LandmarkType_Property_GUID,                       UIA_LandmarkTypePropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_Int, },
    { &Drag_IsGrabbed_Property_GUID,                     UIA_DragIsGrabbedPropertyId, },
    { &GridItem_ColumnSpan_Property_GUID,                UIA_GridItemColumnSpanPropertyId, },
    { &Styles_Shape_Property_GUID,                       UIA_StylesShapePropertyId, },
    { &RuntimeId_Property_GUID,                          UIA_RuntimeIdPropertyId,
      PROP_TYPE_SPECIAL,                                 UIAutomationType_IntArray, },
    { &IsSelectionPattern2Available_Property_GUID,       UIA_IsSelectionPattern2AvailablePropertyId, },
    { &MultipleView_SupportedViews_Property_GUID,        UIA_MultipleViewSupportedViewsPropertyId, },
    { &Styles_FillPatternColor_Property_GUID,            UIA_StylesFillPatternColorPropertyId, },
    { &FullDescription_Property_GUID,                    UIA_FullDescriptionPropertyId,
      PROP_TYPE_ELEM_PROP,                               UIAutomationType_String, },
};

static const int prop_id_idx[] = {
    0xaa, 0x8b, 0x76, 0xa3, 0x3d, 0x0e, 0x9f, 0x08,
    0x2e, 0x1e, 0x65, 0x00, 0x0a, 0x69, 0x2f, 0x1b,
    0x68, 0x86, 0x3a, 0x88, 0x30, 0x3b, 0x52, 0x54,
    0x01, 0x98, 0x14, 0x93, 0x04, 0x17, 0x58, 0x2d,
    0xa4, 0x37, 0x38, 0x03, 0x24, 0x8f, 0x4f, 0x06,
    0x7a, 0x45, 0x02, 0x84, 0x89, 0x55, 0x28, 0x75,
    0x48, 0x8a, 0x09, 0x19, 0x4a, 0x07, 0x9e, 0x77,
    0x15, 0x34, 0x74, 0x7f, 0x82, 0x16, 0x90, 0x5c,
    0x1f, 0x4c, 0x0f, 0xa8, 0x42, 0x26, 0x35, 0x61,
    0xac, 0x33, 0x18, 0x51, 0x81, 0x71, 0x9c, 0x50,
    0x59, 0x6a, 0x21, 0x92, 0x7d, 0x80, 0x44, 0x3c,
    0x83, 0x36, 0x96, 0x4d, 0x32, 0x8c, 0x0c, 0x7b,
    0x46, 0x43, 0x87, 0xa1, 0x1c, 0x73, 0x6d, 0x67,
    0x6b, 0x8e, 0x13, 0x6c, 0x85, 0x41, 0x94, 0x40,
    0x78, 0x3e, 0x72, 0x53, 0x4e, 0x23, 0x2b, 0x11,
    0x27, 0x2a, 0x62, 0x12, 0xa9, 0xad, 0x7e, 0x9b,
    0x29, 0x64, 0x2c, 0x99, 0x20, 0x70, 0x39, 0x6e,
    0xa0, 0x8d, 0xa7, 0x60, 0x49, 0x25, 0x5b, 0xa5,
    0x22, 0x0d, 0x0b, 0x5f, 0x3f, 0x4b, 0x5e, 0x9d,
    0x47, 0x31, 0x1d, 0x5d, 0x97, 0xa6, 0x66, 0xae,
    0x9a, 0x91, 0xa2, 0x56, 0x95, 0x05, 0x63, 0x10,
    0xab, 0x57, 0x6f, 0x1a, 0x7c, 0x5a, 0x79,
};

#define PROP_ID_MIN 30000
#define PROP_ID_MAX (PROP_ID_MIN + ARRAY_SIZE(default_uia_properties))

static const struct uia_prop_info *uia_prop_info_from_guid(const GUID *guid)
{
    struct uia_prop_info *prop;

    if ((prop = bsearch(guid, default_uia_properties, ARRAY_SIZE(default_uia_properties), sizeof(*prop),
            uia_property_guid_compare)))
        return prop;

    return NULL;
}

const struct uia_prop_info *uia_prop_info_from_id(PROPERTYID prop_id)
{
    if ((prop_id < PROP_ID_MIN) || (prop_id >= PROP_ID_MAX))
        return NULL;

    return &default_uia_properties[prop_id_idx[prop_id - PROP_ID_MIN]];
}

/* Sorted by GUID. */
static const struct uia_event_info default_uia_events[] = {
    { &Selection_InvalidatedEvent_Event_GUID,                     UIA_Selection_InvalidatedEventId,
      EventArgsType_Simple, },
    { &Window_WindowOpened_Event_GUID,                            UIA_Window_WindowOpenedEventId,
      EventArgsType_Simple, },
    { &TextEdit_TextChanged_Event_GUID,                           UIA_TextEdit_TextChangedEventId,
      EventArgsType_TextEditTextChanged, },
    { &Drag_DragStart_Event_GUID,                                 UIA_Drag_DragStartEventId,
      EventArgsType_Simple, },
    { &Changes_Event_GUID,                                        UIA_ChangesEventId,
      EventArgsType_Changes, },
    { &DropTarget_DragLeave_Event_GUID,                           UIA_DropTarget_DragLeaveEventId,
      EventArgsType_Simple, },
    { &AutomationFocusChanged_Event_GUID,                         UIA_AutomationFocusChangedEventId,
      EventArgsType_Simple, },
    { &AsyncContentLoaded_Event_GUID,                             UIA_AsyncContentLoadedEventId,
      EventArgsType_AsyncContentLoaded, },
    { &MenuModeStart_Event_GUID,                                  UIA_MenuModeStartEventId,
      EventArgsType_Simple, },
    { &HostedFragmentRootsInvalidated_Event_GUID,                 UIA_HostedFragmentRootsInvalidatedEventId,
      EventArgsType_Simple, },
    { &LayoutInvalidated_Event_GUID,                              UIA_LayoutInvalidatedEventId,
      EventArgsType_Simple, },
    { &MenuOpened_Event_GUID,                                     UIA_MenuOpenedEventId,
      EventArgsType_Simple, },
    { &SystemAlert_Event_GUID,                                    UIA_SystemAlertEventId,
      EventArgsType_Simple, },
    { &StructureChanged_Event_GUID,                               UIA_StructureChangedEventId,
      EventArgsType_StructureChanged, },
    { &InputDiscarded_Event_GUID,                                 UIA_InputDiscardedEventId,
      EventArgsType_Simple, },
    { &MenuClosed_Event_GUID,                                     UIA_MenuClosedEventId,
      EventArgsType_Simple, },
    { &Text_TextChangedEvent_Event_GUID,                          UIA_Text_TextChangedEventId,
      EventArgsType_Simple, },
    { &TextEdit_ConversionTargetChanged_Event_GUID,               UIA_TextEdit_ConversionTargetChangedEventId,
      EventArgsType_Simple, },
    { &Drag_DragComplete_Event_GUID,                              UIA_Drag_DragCompleteEventId,
      EventArgsType_Simple, },
    { &InputReachedOtherElement_Event_GUID,                       UIA_InputReachedOtherElementEventId,
      EventArgsType_Simple, },
    { &LiveRegionChanged_Event_GUID,                              UIA_LiveRegionChangedEventId,
      EventArgsType_Simple, },
    { &InputReachedTarget_Event_GUID,                             UIA_InputReachedTargetEventId,
      EventArgsType_Simple, },
    { &DropTarget_DragEnter_Event_GUID,                           UIA_DropTarget_DragEnterEventId,
      EventArgsType_Simple, },
    { &MenuModeEnd_Event_GUID,                                    UIA_MenuModeEndEventId,
      EventArgsType_Simple, },
    { &Text_TextSelectionChangedEvent_Event_GUID,                 UIA_Text_TextSelectionChangedEventId,
      EventArgsType_Simple, },
    { &AutomationPropertyChanged_Event_GUID,                      UIA_AutomationPropertyChangedEventId,
      EventArgsType_PropertyChanged, },
    { &SelectionItem_ElementRemovedFromSelectionEvent_Event_GUID, UIA_SelectionItem_ElementRemovedFromSelectionEventId,
      EventArgsType_Simple, },
    { &SelectionItem_ElementAddedToSelectionEvent_Event_GUID,     UIA_SelectionItem_ElementAddedToSelectionEventId,
      EventArgsType_Simple, },
    { &DropTarget_Dropped_Event_GUID,                             UIA_DropTarget_DroppedEventId,
      EventArgsType_Simple, },
    { &ToolTipClosed_Event_GUID,                                  UIA_ToolTipClosedEventId,
      EventArgsType_Simple, },
    { &Invoke_Invoked_Event_GUID,                                 UIA_Invoke_InvokedEventId,
      EventArgsType_Simple, },
    { &Notification_Event_GUID,                                   UIA_NotificationEventId,
      EventArgsType_Notification, },
    { &Window_WindowClosed_Event_GUID,                            UIA_Window_WindowClosedEventId,
      EventArgsType_WindowClosed, },
    { &Drag_DragCancel_Event_GUID,                                UIA_Drag_DragCancelEventId,
      EventArgsType_Simple, },
    { &SelectionItem_ElementSelectedEvent_Event_GUID,             UIA_SelectionItem_ElementSelectedEventId,
      EventArgsType_Simple, },
    { &ToolTipOpened_Event_GUID,                                  UIA_ToolTipOpenedEventId,
      EventArgsType_Simple, },
};

static const int event_id_idx[] = {
    0x23, 0x1d, 0x0d, 0x0b, 0x19, 0x06, 0x07, 0x0f,
    0x0a, 0x1e, 0x1b, 0x1a, 0x22, 0x00, 0x18, 0x10,
    0x01, 0x20, 0x08, 0x17, 0x15, 0x13, 0x0e, 0x0c,
    0x14, 0x09, 0x03, 0x21, 0x12, 0x16, 0x05, 0x1c,
    0x02, 0x11, 0x04, 0x1f,
};

#define EVENT_ID_MIN 20000
#define EVENT_ID_MAX (EVENT_ID_MIN + ARRAY_SIZE(default_uia_events))

static const struct uia_event_info *uia_event_info_from_guid(const GUID *guid)
{
    struct uia_event_info *event;

    if ((event = bsearch(guid, default_uia_events, ARRAY_SIZE(default_uia_events), sizeof(*event),
            uia_event_guid_compare)))
        return event;

    return NULL;
}

const struct uia_event_info *uia_event_info_from_id(EVENTID event_id)
{
    if ((event_id < EVENT_ID_MIN) || (event_id >= EVENT_ID_MAX))
        return NULL;

    return &default_uia_events[event_id_idx[event_id - EVENT_ID_MIN]];
}

/* Sorted by GUID. */
static const struct uia_pattern_info default_uia_patterns[] = {
    { &ScrollItem_Pattern_GUID,         UIA_ScrollItemPatternId,
      &IID_IScrollItemProvider, },
    { &Tranform_Pattern2_GUID,          UIA_TransformPattern2Id,
      &IID_ITransformProvider2, },
    { &ItemContainer_Pattern_GUID,      UIA_ItemContainerPatternId,
      &IID_IItemContainerProvider, },
    { &Drag_Pattern_GUID,               UIA_DragPatternId,
      &IID_IDragProvider, },
    { &Window_Pattern_GUID,             UIA_WindowPatternId,
      &IID_IWindowProvider, },
    { &VirtualizedItem_Pattern_GUID,    UIA_VirtualizedItemPatternId,
      &IID_IVirtualizedItemProvider, },
    { &Dock_Pattern_GUID,               UIA_DockPatternId,
      &IID_IDockProvider, },
    { &Styles_Pattern_GUID,             UIA_StylesPatternId,
      &IID_IStylesProvider, },
    { &DropTarget_Pattern_GUID,         UIA_DropTargetPatternId,
      &IID_IDropTargetProvider, },
    { &Text_Pattern_GUID,               UIA_TextPatternId,
      &IID_ITextProvider, },
    { &Toggle_Pattern_GUID,             UIA_TogglePatternId,
      &IID_IToggleProvider, },
    { &GridItem_Pattern_GUID,           UIA_GridItemPatternId,
      &IID_IGridItemProvider, },
    { &RangeValue_Pattern_GUID,         UIA_RangeValuePatternId,
      &IID_IRangeValueProvider, },
    { &TextEdit_Pattern_GUID,           UIA_TextEditPatternId,
      &IID_ITextEditProvider, },
    { &CustomNavigation_Pattern_GUID,   UIA_CustomNavigationPatternId,
      &IID_ICustomNavigationProvider, },
    { &Table_Pattern_GUID,              UIA_TablePatternId,
      &IID_ITableProvider, },
    { &Value_Pattern_GUID,              UIA_ValuePatternId,
      &IID_IValueProvider, },
    { &LegacyIAccessible_Pattern_GUID,  UIA_LegacyIAccessiblePatternId,
      &IID_ILegacyIAccessibleProvider, },
    { &Text_Pattern2_GUID,              UIA_TextPattern2Id,
      &IID_ITextProvider2, },
    { &ExpandCollapse_Pattern_GUID,     UIA_ExpandCollapsePatternId,
      &IID_IExpandCollapseProvider, },
    { &SynchronizedInput_Pattern_GUID,  UIA_SynchronizedInputPatternId,
      &IID_ISynchronizedInputProvider, },
    { &Scroll_Pattern_GUID,             UIA_ScrollPatternId,
      &IID_IScrollProvider, },
    { &TextChild_Pattern_GUID,          UIA_TextChildPatternId,
      &IID_ITextChildProvider, },
    { &TableItem_Pattern_GUID,          UIA_TableItemPatternId,
      &IID_ITableItemProvider, },
    { &Spreadsheet_Pattern_GUID,        UIA_SpreadsheetPatternId,
      &IID_ISpreadsheetProvider, },
    { &Grid_Pattern_GUID,               UIA_GridPatternId,
      &IID_IGridProvider, },
    { &Annotation_Pattern_GUID,         UIA_AnnotationPatternId,
      &IID_IAnnotationProvider, },
    { &Transform_Pattern_GUID,          UIA_TransformPatternId,
      &IID_ITransformProvider, },
    { &MultipleView_Pattern_GUID,       UIA_MultipleViewPatternId,
      &IID_IMultipleViewProvider, },
    { &Selection_Pattern_GUID,          UIA_SelectionPatternId,
      &IID_ISelectionProvider, },
    { &SelectionItem_Pattern_GUID,      UIA_SelectionItemPatternId,
      &IID_ISelectionItemProvider, },
    { &Invoke_Pattern_GUID,             UIA_InvokePatternId,
      &IID_IInvokeProvider, },
    { &ObjectModel_Pattern_GUID,        UIA_ObjectModelPatternId,
      &IID_IObjectModelProvider, },
    { &SpreadsheetItem_Pattern_GUID,    UIA_SpreadsheetItemPatternId,
      &IID_ISpreadsheetItemProvider, },
};

static const int pattern_id_idx[] = {
    0x1f, 0x1d, 0x10, 0x0c, 0x15, 0x13, 0x19, 0x0b,
    0x1c, 0x04, 0x1e, 0x06, 0x0f, 0x17, 0x09, 0x0a,
    0x1b, 0x00, 0x11, 0x02, 0x05, 0x14, 0x20, 0x1a,
    0x12, 0x07, 0x18, 0x21, 0x01, 0x16, 0x03, 0x08,
    0x0d, 0x0e,
};

#define PATTERN_ID_MIN 10000
#define PATTERN_ID_MAX (PATTERN_ID_MIN + ARRAY_SIZE(default_uia_patterns))

static const struct uia_pattern_info *uia_pattern_info_from_guid(const GUID *guid)
{
    struct uia_pattern_info *pattern;

    if ((pattern = bsearch(guid, default_uia_patterns, ARRAY_SIZE(default_uia_patterns), sizeof(*pattern),
            uia_pattern_guid_compare)))
        return pattern;

    return NULL;
}

const struct uia_pattern_info *uia_pattern_info_from_id(PATTERNID pattern_id)
{
    if ((pattern_id < PATTERN_ID_MIN) || (pattern_id >= PATTERN_ID_MAX))
        return NULL;

    return &default_uia_patterns[pattern_id_idx[pattern_id - PATTERN_ID_MIN]];
}

/* Sorted by GUID. */
static const struct uia_control_type_info default_uia_control_types[] = {
    { &Table_Control_GUID,        UIA_TableControlTypeId },
    { &StatusBar_Control_GUID,    UIA_StatusBarControlTypeId },
    { &Group_Control_GUID,        UIA_GroupControlTypeId },
    { &SplitButton_Control_GUID,  UIA_SplitButtonControlTypeId },
    { &CheckBox_Control_GUID,     UIA_CheckBoxControlTypeId },
    { &Hyperlink_Control_GUID,    UIA_HyperlinkControlTypeId },
    { &Tab_Control_GUID,          UIA_TabControlTypeId },
    { &ScrollBar_Control_GUID,    UIA_ScrollBarControlTypeId },
    { &Spinner_Control_GUID,      UIA_SpinnerControlTypeId },
    { &Menu_Control_GUID,         UIA_MenuControlTypeId },
    { &Window_Control_GUID,       UIA_WindowControlTypeId },
    { &DataItem_Control_GUID,     UIA_DataItemControlTypeId },
    { &SemanticZoom_Control_GUID, UIA_SemanticZoomControlTypeId },
    { &Slider_Control_GUID,       UIA_SliderControlTypeId },
    { &TabItem_Control_GUID,      UIA_TabItemControlTypeId },
    { &MenuBar_Control_GUID,      UIA_MenuBarControlTypeId },
    { &ToolBar_Control_GUID,      UIA_ToolBarControlTypeId },
    { &Pane_Control_GUID,         UIA_PaneControlTypeId },
    { &Button_Control_GUID,       UIA_ButtonControlTypeId },
    { &ComboBox_Control_GUID,     UIA_ComboBoxControlTypeId },
    { &Document_Control_GUID,     UIA_DocumentControlTypeId },
    { &Thumb_Control_GUID,        UIA_ThumbControlTypeId },
    { &ProgressBar_Control_GUID,  UIA_ProgressBarControlTypeId },
    { &Calendar_Control_GUID,     UIA_CalendarControlTypeId },
    { &AppBar_Control_GUID,       UIA_AppBarControlTypeId },
    { &Tree_Control_GUID,         UIA_TreeControlTypeId },
    { &Separator_Control_GUID,    UIA_SeparatorControlTypeId },
    { &DataGrid_Control_GUID,     UIA_DataGridControlTypeId },
    { &TreeItem_Control_GUID,     UIA_TreeItemControlTypeId },
    { &TitleBar_Control_GUID,     UIA_TitleBarControlTypeId },
    { &Custom_Control_GUID,       UIA_CustomControlTypeId },
    { &Edit_Control_GUID,         UIA_EditControlTypeId },
    { &HeaderItem_Control_GUID,   UIA_HeaderItemControlTypeId },
    { &Header_Control_GUID,       UIA_HeaderControlTypeId },
    { &ToolTip_Control_GUID,      UIA_ToolTipControlTypeId },
    { &MenuItem_Control_GUID,     UIA_MenuItemControlTypeId },
    { &RadioButton_Control_GUID,  UIA_RadioButtonControlTypeId },
    { &Text_Control_GUID,         UIA_TextControlTypeId },
    { &List_Control_GUID,         UIA_ListControlTypeId },
    { &Image_Control_GUID,        UIA_ImageControlTypeId },
    { &ListItem_Control_GUID,     UIA_ListItemControlTypeId },
};

static const int control_type_id_idx[] = {
    0x12, 0x17, 0x04, 0x13, 0x1f, 0x05, 0x27, 0x28,
    0x26, 0x09, 0x0f, 0x23, 0x16, 0x24, 0x07, 0x0d,
    0x08, 0x01, 0x06, 0x0e, 0x25, 0x10, 0x22, 0x19,
    0x1c, 0x1e, 0x02, 0x15, 0x1b, 0x0b, 0x14, 0x03,
    0x0a, 0x11, 0x21, 0x20, 0x00, 0x1d, 0x1a, 0x0c,
    0x18,
};

#define CONTROL_TYPE_ID_MIN 50000
#define CONTROL_TYPE_ID_MAX (CONTROL_TYPE_ID_MIN + ARRAY_SIZE(default_uia_control_types))

static const struct uia_control_type_info *uia_control_type_info_from_guid(const GUID *guid)
{
    struct uia_control_type_info *control_type;

    if ((control_type = bsearch(guid, default_uia_control_types, ARRAY_SIZE(default_uia_control_types),
                    sizeof(*control_type), uia_control_type_guid_compare)))
        return control_type;

    return NULL;
}

const struct uia_control_type_info *uia_control_type_info_from_id(CONTROLTYPEID control_type_id)
{
    if ((control_type_id < CONTROL_TYPE_ID_MIN) || (control_type_id >= CONTROL_TYPE_ID_MAX))
        return NULL;

    return &default_uia_control_types[control_type_id_idx[control_type_id - CONTROL_TYPE_ID_MIN]];
}

/***********************************************************************
 *          UiaLookupId (uiautomationcore.@)
 */
int WINAPI UiaLookupId(enum AutomationIdentifierType type, const GUID *guid)
{
    int ret_id = 0;

    TRACE("(%d, %s)\n", type, debugstr_guid(guid));

    switch (type)
    {
    case AutomationIdentifierType_Property:
    {
        const struct uia_prop_info *prop = uia_prop_info_from_guid(guid);

        if (prop)
            ret_id = prop->prop_id;
        else
            FIXME("Failed to find propertyId for GUID %s\n", debugstr_guid(guid));

        break;
    }

    case AutomationIdentifierType_Event:
    {
        const struct uia_event_info *event = uia_event_info_from_guid(guid);

        if (event)
            ret_id = event->event_id;
        else
            FIXME("Failed to find eventId for GUID %s\n", debugstr_guid(guid));

        break;
    }

    case AutomationIdentifierType_Pattern:
    {
        const struct uia_pattern_info *pattern = uia_pattern_info_from_guid(guid);

        if (pattern)
            ret_id = pattern->pattern_id;
        else
            FIXME("Failed to find patternId for GUID %s\n", debugstr_guid(guid));

        break;
    }

    case AutomationIdentifierType_ControlType:
    {
        const struct uia_control_type_info *control_type = uia_control_type_info_from_guid(guid);

        if (control_type)
            ret_id = control_type->control_type_id;
        else
            FIXME("Failed to find control type Id for GUID %s\n", debugstr_guid(guid));

        break;
    }

    case AutomationIdentifierType_TextAttribute:
    case AutomationIdentifierType_LandmarkType:
    case AutomationIdentifierType_Annotation:
    case AutomationIdentifierType_Changes:
    case AutomationIdentifierType_Style:
        FIXME("Unimplemented AutomationIdentifierType %d\n", type);
        break;

    default:
        FIXME("Invalid AutomationIdentifierType %d\n", type);
        break;
    }

    return ret_id;
}
