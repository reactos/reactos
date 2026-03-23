/*
 * UI Automation tests
 *
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#define COBJMACROS

#include <oleauto.h>
#include <assert.h>
#include "windows.h"
#include "initguid.h"
#include "uiautomation.h"
#include "ocidl.h"
#include "wine/iaccessible2.h"

#include "wine/test.h"

static HRESULT (WINAPI *pUiaProviderFromIAccessible)(IAccessible *, LONG, DWORD, IRawElementProviderSimple **);
static HRESULT (WINAPI *pUiaDisconnectProvider)(IRawElementProviderSimple *);

struct str_id_pair {
    int id;
    const char *str;
};

static const struct str_id_pair uia_prop_id_strs[] = {
    { UIA_RuntimeIdPropertyId,                           "UIA_RuntimeIdPropertyId", },
    { UIA_BoundingRectanglePropertyId,                   "UIA_BoundingRectanglePropertyId", },
    { UIA_ProcessIdPropertyId,                           "UIA_ProcessIdPropertyId", },
    { UIA_ControlTypePropertyId,                         "UIA_ControlTypePropertyId", },
    { UIA_LocalizedControlTypePropertyId,                "UIA_LocalizedControlTypePropertyId", },
    { UIA_NamePropertyId,                                "UIA_NamePropertyId", },
    { UIA_AcceleratorKeyPropertyId,                      "UIA_AcceleratorKeyPropertyId", },
    { UIA_AccessKeyPropertyId,                           "UIA_AccessKeyPropertyId", },
    { UIA_HasKeyboardFocusPropertyId,                    "UIA_HasKeyboardFocusPropertyId", },
    { UIA_IsKeyboardFocusablePropertyId,                 "UIA_IsKeyboardFocusablePropertyId", },
    { UIA_IsEnabledPropertyId,                           "UIA_IsEnabledPropertyId", },
    { UIA_AutomationIdPropertyId,                        "UIA_AutomationIdPropertyId", },
    { UIA_ClassNamePropertyId,                           "UIA_ClassNamePropertyId", },
    { UIA_HelpTextPropertyId,                            "UIA_HelpTextPropertyId", },
    { UIA_ClickablePointPropertyId,                      "UIA_ClickablePointPropertyId", },
    { UIA_CulturePropertyId,                             "UIA_CulturePropertyId", },
    { UIA_IsControlElementPropertyId,                    "UIA_IsControlElementPropertyId", },
    { UIA_IsContentElementPropertyId,                    "UIA_IsContentElementPropertyId", },
    { UIA_LabeledByPropertyId,                           "UIA_LabeledByPropertyId", },
    { UIA_IsPasswordPropertyId,                          "UIA_IsPasswordPropertyId", },
    { UIA_NativeWindowHandlePropertyId,                  "UIA_NativeWindowHandlePropertyId", },
    { UIA_ItemTypePropertyId,                            "UIA_ItemTypePropertyId", },
    { UIA_IsOffscreenPropertyId,                         "UIA_IsOffscreenPropertyId", },
    { UIA_OrientationPropertyId,                         "UIA_OrientationPropertyId", },
    { UIA_FrameworkIdPropertyId,                         "UIA_FrameworkIdPropertyId", },
    { UIA_IsRequiredForFormPropertyId,                   "UIA_IsRequiredForFormPropertyId", },
    { UIA_ItemStatusPropertyId,                          "UIA_ItemStatusPropertyId", },
    { UIA_IsDockPatternAvailablePropertyId,              "UIA_IsDockPatternAvailablePropertyId", },
    { UIA_IsExpandCollapsePatternAvailablePropertyId,    "UIA_IsExpandCollapsePatternAvailablePropertyId", },
    { UIA_IsGridItemPatternAvailablePropertyId,          "UIA_IsGridItemPatternAvailablePropertyId", },
    { UIA_IsGridPatternAvailablePropertyId,              "UIA_IsGridPatternAvailablePropertyId", },
    { UIA_IsInvokePatternAvailablePropertyId,            "UIA_IsInvokePatternAvailablePropertyId", },
    { UIA_IsMultipleViewPatternAvailablePropertyId,      "UIA_IsMultipleViewPatternAvailablePropertyId", },
    { UIA_IsRangeValuePatternAvailablePropertyId,        "UIA_IsRangeValuePatternAvailablePropertyId", },
    { UIA_IsScrollPatternAvailablePropertyId,            "UIA_IsScrollPatternAvailablePropertyId", },
    { UIA_IsScrollItemPatternAvailablePropertyId,        "UIA_IsScrollItemPatternAvailablePropertyId", },
    { UIA_IsSelectionItemPatternAvailablePropertyId,     "UIA_IsSelectionItemPatternAvailablePropertyId", },
    { UIA_IsSelectionPatternAvailablePropertyId,         "UIA_IsSelectionPatternAvailablePropertyId", },
    { UIA_IsTablePatternAvailablePropertyId,             "UIA_IsTablePatternAvailablePropertyId", },
    { UIA_IsTableItemPatternAvailablePropertyId,         "UIA_IsTableItemPatternAvailablePropertyId", },
    { UIA_IsTextPatternAvailablePropertyId,              "UIA_IsTextPatternAvailablePropertyId", },
    { UIA_IsTogglePatternAvailablePropertyId,            "UIA_IsTogglePatternAvailablePropertyId", },
    { UIA_IsTransformPatternAvailablePropertyId,         "UIA_IsTransformPatternAvailablePropertyId", },
    { UIA_IsValuePatternAvailablePropertyId,             "UIA_IsValuePatternAvailablePropertyId", },
    { UIA_IsWindowPatternAvailablePropertyId,            "UIA_IsWindowPatternAvailablePropertyId", },
    { UIA_ValueValuePropertyId,                          "UIA_ValueValuePropertyId", },
    { UIA_ValueIsReadOnlyPropertyId,                     "UIA_ValueIsReadOnlyPropertyId", },
    { UIA_RangeValueValuePropertyId,                     "UIA_RangeValueValuePropertyId", },
    { UIA_RangeValueIsReadOnlyPropertyId,                "UIA_RangeValueIsReadOnlyPropertyId", },
    { UIA_RangeValueMinimumPropertyId,                   "UIA_RangeValueMinimumPropertyId", },
    { UIA_RangeValueMaximumPropertyId,                   "UIA_RangeValueMaximumPropertyId", },
    { UIA_RangeValueLargeChangePropertyId,               "UIA_RangeValueLargeChangePropertyId", },
    { UIA_RangeValueSmallChangePropertyId,               "UIA_RangeValueSmallChangePropertyId", },
    { UIA_ScrollHorizontalScrollPercentPropertyId,       "UIA_ScrollHorizontalScrollPercentPropertyId", },
    { UIA_ScrollHorizontalViewSizePropertyId,            "UIA_ScrollHorizontalViewSizePropertyId", },
    { UIA_ScrollVerticalScrollPercentPropertyId,         "UIA_ScrollVerticalScrollPercentPropertyId", },
    { UIA_ScrollVerticalViewSizePropertyId,              "UIA_ScrollVerticalViewSizePropertyId", },
    { UIA_ScrollHorizontallyScrollablePropertyId,        "UIA_ScrollHorizontallyScrollablePropertyId", },
    { UIA_ScrollVerticallyScrollablePropertyId,          "UIA_ScrollVerticallyScrollablePropertyId", },
    { UIA_SelectionSelectionPropertyId,                  "UIA_SelectionSelectionPropertyId", },
    { UIA_SelectionCanSelectMultiplePropertyId,          "UIA_SelectionCanSelectMultiplePropertyId", },
    { UIA_SelectionIsSelectionRequiredPropertyId,        "UIA_SelectionIsSelectionRequiredPropertyId", },
    { UIA_GridRowCountPropertyId,                        "UIA_GridRowCountPropertyId", },
    { UIA_GridColumnCountPropertyId,                     "UIA_GridColumnCountPropertyId", },
    { UIA_GridItemRowPropertyId,                         "UIA_GridItemRowPropertyId", },
    { UIA_GridItemColumnPropertyId,                      "UIA_GridItemColumnPropertyId", },
    { UIA_GridItemRowSpanPropertyId,                     "UIA_GridItemRowSpanPropertyId", },
    { UIA_GridItemColumnSpanPropertyId,                  "UIA_GridItemColumnSpanPropertyId", },
    { UIA_GridItemContainingGridPropertyId,              "UIA_GridItemContainingGridPropertyId", },
    { UIA_DockDockPositionPropertyId,                    "UIA_DockDockPositionPropertyId", },
    { UIA_ExpandCollapseExpandCollapseStatePropertyId,   "UIA_ExpandCollapseExpandCollapseStatePropertyId", },
    { UIA_MultipleViewCurrentViewPropertyId,             "UIA_MultipleViewCurrentViewPropertyId", },
    { UIA_MultipleViewSupportedViewsPropertyId,          "UIA_MultipleViewSupportedViewsPropertyId", },
    { UIA_WindowCanMaximizePropertyId,                   "UIA_WindowCanMaximizePropertyId", },
    { UIA_WindowCanMinimizePropertyId,                   "UIA_WindowCanMinimizePropertyId", },
    { UIA_WindowWindowVisualStatePropertyId,             "UIA_WindowWindowVisualStatePropertyId", },
    { UIA_WindowWindowInteractionStatePropertyId,        "UIA_WindowWindowInteractionStatePropertyId", },
    { UIA_WindowIsModalPropertyId,                       "UIA_WindowIsModalPropertyId", },
    { UIA_WindowIsTopmostPropertyId,                     "UIA_WindowIsTopmostPropertyId", },
    { UIA_SelectionItemIsSelectedPropertyId,             "UIA_SelectionItemIsSelectedPropertyId", },
    { UIA_SelectionItemSelectionContainerPropertyId,     "UIA_SelectionItemSelectionContainerPropertyId", },
    { UIA_TableRowHeadersPropertyId,                     "UIA_TableRowHeadersPropertyId", },
    { UIA_TableColumnHeadersPropertyId,                  "UIA_TableColumnHeadersPropertyId", },
    { UIA_TableRowOrColumnMajorPropertyId,               "UIA_TableRowOrColumnMajorPropertyId", },
    { UIA_TableItemRowHeaderItemsPropertyId,             "UIA_TableItemRowHeaderItemsPropertyId", },
    { UIA_TableItemColumnHeaderItemsPropertyId,          "UIA_TableItemColumnHeaderItemsPropertyId", },
    { UIA_ToggleToggleStatePropertyId,                   "UIA_ToggleToggleStatePropertyId", },
    { UIA_TransformCanMovePropertyId,                    "UIA_TransformCanMovePropertyId", },
    { UIA_TransformCanResizePropertyId,                  "UIA_TransformCanResizePropertyId", },
    { UIA_TransformCanRotatePropertyId,                  "UIA_TransformCanRotatePropertyId", },
    { UIA_IsLegacyIAccessiblePatternAvailablePropertyId, "UIA_IsLegacyIAccessiblePatternAvailablePropertyId", },
    { UIA_LegacyIAccessibleChildIdPropertyId,            "UIA_LegacyIAccessibleChildIdPropertyId", },
    { UIA_LegacyIAccessibleNamePropertyId,               "UIA_LegacyIAccessibleNamePropertyId", },
    { UIA_LegacyIAccessibleValuePropertyId,              "UIA_LegacyIAccessibleValuePropertyId", },
    { UIA_LegacyIAccessibleDescriptionPropertyId,        "UIA_LegacyIAccessibleDescriptionPropertyId", },
    { UIA_LegacyIAccessibleRolePropertyId,               "UIA_LegacyIAccessibleRolePropertyId", },
    { UIA_LegacyIAccessibleStatePropertyId,              "UIA_LegacyIAccessibleStatePropertyId", },
    { UIA_LegacyIAccessibleHelpPropertyId,               "UIA_LegacyIAccessibleHelpPropertyId", },
    { UIA_LegacyIAccessibleKeyboardShortcutPropertyId,   "UIA_LegacyIAccessibleKeyboardShortcutPropertyId", },
    { UIA_LegacyIAccessibleSelectionPropertyId,          "UIA_LegacyIAccessibleSelectionPropertyId", },
    { UIA_LegacyIAccessibleDefaultActionPropertyId,      "UIA_LegacyIAccessibleDefaultActionPropertyId", },
    { UIA_AriaRolePropertyId,                            "UIA_AriaRolePropertyId", },
    { UIA_AriaPropertiesPropertyId,                      "UIA_AriaPropertiesPropertyId", },
    { UIA_IsDataValidForFormPropertyId,                  "UIA_IsDataValidForFormPropertyId", },
    { UIA_ControllerForPropertyId,                       "UIA_ControllerForPropertyId", },
    { UIA_DescribedByPropertyId,                         "UIA_DescribedByPropertyId", },
    { UIA_FlowsToPropertyId,                             "UIA_FlowsToPropertyId", },
    { UIA_ProviderDescriptionPropertyId,                 "UIA_ProviderDescriptionPropertyId", },
    { UIA_IsItemContainerPatternAvailablePropertyId,     "UIA_IsItemContainerPatternAvailablePropertyId", },
    { UIA_IsVirtualizedItemPatternAvailablePropertyId,   "UIA_IsVirtualizedItemPatternAvailablePropertyId", },
    { UIA_IsSynchronizedInputPatternAvailablePropertyId, "UIA_IsSynchronizedInputPatternAvailablePropertyId", },
    { UIA_OptimizeForVisualContentPropertyId,            "UIA_OptimizeForVisualContentPropertyId", },
    { UIA_IsObjectModelPatternAvailablePropertyId,       "UIA_IsObjectModelPatternAvailablePropertyId", },
    { UIA_AnnotationAnnotationTypeIdPropertyId,          "UIA_AnnotationAnnotationTypeIdPropertyId", },
    { UIA_AnnotationAnnotationTypeNamePropertyId,        "UIA_AnnotationAnnotationTypeNamePropertyId", },
    { UIA_AnnotationAuthorPropertyId,                    "UIA_AnnotationAuthorPropertyId", },
    { UIA_AnnotationDateTimePropertyId,                  "UIA_AnnotationDateTimePropertyId", },
    { UIA_AnnotationTargetPropertyId,                    "UIA_AnnotationTargetPropertyId", },
    { UIA_IsAnnotationPatternAvailablePropertyId,        "UIA_IsAnnotationPatternAvailablePropertyId", },
    { UIA_IsTextPattern2AvailablePropertyId,             "UIA_IsTextPattern2AvailablePropertyId", },
    { UIA_StylesStyleIdPropertyId,                       "UIA_StylesStyleIdPropertyId", },
    { UIA_StylesStyleNamePropertyId,                     "UIA_StylesStyleNamePropertyId", },
    { UIA_StylesFillColorPropertyId,                     "UIA_StylesFillColorPropertyId", },
    { UIA_StylesFillPatternStylePropertyId,              "UIA_StylesFillPatternStylePropertyId", },
    { UIA_StylesShapePropertyId,                         "UIA_StylesShapePropertyId", },
    { UIA_StylesFillPatternColorPropertyId,              "UIA_StylesFillPatternColorPropertyId", },
    { UIA_StylesExtendedPropertiesPropertyId,            "UIA_StylesExtendedPropertiesPropertyId", },
    { UIA_IsStylesPatternAvailablePropertyId,            "UIA_IsStylesPatternAvailablePropertyId", },
    { UIA_IsSpreadsheetPatternAvailablePropertyId,       "UIA_IsSpreadsheetPatternAvailablePropertyId", },
    { UIA_SpreadsheetItemFormulaPropertyId,              "UIA_SpreadsheetItemFormulaPropertyId", },
    { UIA_SpreadsheetItemAnnotationObjectsPropertyId,    "UIA_SpreadsheetItemAnnotationObjectsPropertyId", },
    { UIA_SpreadsheetItemAnnotationTypesPropertyId,      "UIA_SpreadsheetItemAnnotationTypesPropertyId", },
    { UIA_IsSpreadsheetItemPatternAvailablePropertyId,   "UIA_IsSpreadsheetItemPatternAvailablePropertyId", },
    { UIA_Transform2CanZoomPropertyId,                   "UIA_Transform2CanZoomPropertyId", },
    { UIA_IsTransformPattern2AvailablePropertyId,        "UIA_IsTransformPattern2AvailablePropertyId", },
    { UIA_LiveSettingPropertyId,                         "UIA_LiveSettingPropertyId", },
    { UIA_IsTextChildPatternAvailablePropertyId,         "UIA_IsTextChildPatternAvailablePropertyId", },
    { UIA_IsDragPatternAvailablePropertyId,              "UIA_IsDragPatternAvailablePropertyId", },
    { UIA_DragIsGrabbedPropertyId,                       "UIA_DragIsGrabbedPropertyId", },
    { UIA_DragDropEffectPropertyId,                      "UIA_DragDropEffectPropertyId", },
    { UIA_DragDropEffectsPropertyId,                     "UIA_DragDropEffectsPropertyId", },
    { UIA_IsDropTargetPatternAvailablePropertyId,        "UIA_IsDropTargetPatternAvailablePropertyId", },
    { UIA_DropTargetDropTargetEffectPropertyId,          "UIA_DropTargetDropTargetEffectPropertyId", },
    { UIA_DropTargetDropTargetEffectsPropertyId,         "UIA_DropTargetDropTargetEffectsPropertyId", },
    { UIA_DragGrabbedItemsPropertyId,                    "UIA_DragGrabbedItemsPropertyId", },
    { UIA_Transform2ZoomLevelPropertyId,                 "UIA_Transform2ZoomLevelPropertyId", },
    { UIA_Transform2ZoomMinimumPropertyId,               "UIA_Transform2ZoomMinimumPropertyId", },
    { UIA_Transform2ZoomMaximumPropertyId,               "UIA_Transform2ZoomMaximumPropertyId", },
    { UIA_FlowsFromPropertyId,                           "UIA_FlowsFromPropertyId", },
    { UIA_IsTextEditPatternAvailablePropertyId,          "UIA_IsTextEditPatternAvailablePropertyId", },
    { UIA_IsPeripheralPropertyId,                        "UIA_IsPeripheralPropertyId", },
    { UIA_IsCustomNavigationPatternAvailablePropertyId,  "UIA_IsCustomNavigationPatternAvailablePropertyId", },
    { UIA_PositionInSetPropertyId,                       "UIA_PositionInSetPropertyId", },
    { UIA_SizeOfSetPropertyId,                           "UIA_SizeOfSetPropertyId", },
    { UIA_LevelPropertyId,                               "UIA_LevelPropertyId", },
    { UIA_AnnotationTypesPropertyId,                     "UIA_AnnotationTypesPropertyId", },
    { UIA_AnnotationObjectsPropertyId,                   "UIA_AnnotationObjectsPropertyId", },
    { UIA_LandmarkTypePropertyId,                        "UIA_LandmarkTypePropertyId", },
    { UIA_LocalizedLandmarkTypePropertyId,               "UIA_LocalizedLandmarkTypePropertyId", },
    { UIA_FullDescriptionPropertyId,                     "UIA_FullDescriptionPropertyId", },
    { UIA_FillColorPropertyId,                           "UIA_FillColorPropertyId", },
    { UIA_OutlineColorPropertyId,                        "UIA_OutlineColorPropertyId", },
    { UIA_FillTypePropertyId,                            "UIA_FillTypePropertyId", },
    { UIA_VisualEffectsPropertyId,                       "UIA_VisualEffectsPropertyId", },
    { UIA_OutlineThicknessPropertyId,                    "UIA_OutlineThicknessPropertyId", },
    { UIA_CenterPointPropertyId,                         "UIA_CenterPointPropertyId", },
    { UIA_RotationPropertyId,                            "UIA_RotationPropertyId", },
    { UIA_SizePropertyId,                                "UIA_SizePropertyId", },
    { UIA_IsSelectionPattern2AvailablePropertyId,        "UIA_IsSelectionPattern2AvailablePropertyId", },
    { UIA_Selection2FirstSelectedItemPropertyId,         "UIA_Selection2FirstSelectedItemPropertyId", },
    { UIA_Selection2LastSelectedItemPropertyId,          "UIA_Selection2LastSelectedItemPropertyId", },
    { UIA_Selection2CurrentSelectedItemPropertyId,       "UIA_Selection2CurrentSelectedItemPropertyId", },
    { UIA_Selection2ItemCountPropertyId,                 "UIA_Selection2ItemCountPropertyId", },
    { UIA_HeadingLevelPropertyId,                        "UIA_HeadingLevelPropertyId", },
    { UIA_IsDialogPropertyId,                            "UIA_IsDialogPropertyId", },
};

static const struct str_id_pair uia_pattern_id_strs[] = {
    { UIA_InvokePatternId,            "UIA_InvokePatternId", },
    { UIA_SelectionPatternId,         "UIA_SelectionPatternId", },
    { UIA_ValuePatternId,             "UIA_ValuePatternId", },
    { UIA_RangeValuePatternId,        "UIA_RangeValuePatternId", },
    { UIA_ScrollPatternId,            "UIA_ScrollPatternId", },
    { UIA_ExpandCollapsePatternId,    "UIA_ExpandCollapsePatternId", },
    { UIA_GridPatternId,              "UIA_GridPatternId", },
    { UIA_GridItemPatternId,          "UIA_GridItemPatternId", },
    { UIA_MultipleViewPatternId,      "UIA_MultipleViewPatternId", },
    { UIA_WindowPatternId,            "UIA_WindowPatternId", },
    { UIA_SelectionItemPatternId,     "UIA_SelectionItemPatternId", },
    { UIA_DockPatternId,              "UIA_DockPatternId", },
    { UIA_TablePatternId,             "UIA_TablePatternId", },
    { UIA_TableItemPatternId,         "UIA_TableItemPatternId", },
    { UIA_TextPatternId,              "UIA_TextPatternId", },
    { UIA_TogglePatternId,            "UIA_TogglePatternId", },
    { UIA_TransformPatternId,         "UIA_TransformPatternId", },
    { UIA_ScrollItemPatternId,        "UIA_ScrollItemPatternId", },
    { UIA_LegacyIAccessiblePatternId, "UIA_LegacyIAccessiblePatternId", },
    { UIA_ItemContainerPatternId,     "UIA_ItemContainerPatternId", },
    { UIA_VirtualizedItemPatternId,   "UIA_VirtualizedItemPatternId", },
    { UIA_SynchronizedInputPatternId, "UIA_SynchronizedInputPatternId", },
    { UIA_ObjectModelPatternId,       "UIA_ObjectModelPatternId", },
    { UIA_AnnotationPatternId,        "UIA_AnnotationPatternId", },
    { UIA_TextPattern2Id,             "UIA_TextPattern2Id", },
    { UIA_StylesPatternId,            "UIA_StylesPatternId", },
    { UIA_SpreadsheetPatternId,       "UIA_SpreadsheetPatternId", },
    { UIA_SpreadsheetItemPatternId,   "UIA_SpreadsheetItemPatternId", },
    { UIA_TransformPattern2Id,        "UIA_TransformPattern2Id", },
    { UIA_TextChildPatternId,         "UIA_TextChildPatternId", },
    { UIA_DragPatternId,              "UIA_DragPatternId", },
    { UIA_DropTargetPatternId,        "UIA_DropTargetPatternId", },
    { UIA_TextEditPatternId,          "UIA_TextEditPatternId", },
    { UIA_CustomNavigationPatternId,  "UIA_CustomNavigationPatternId", },
};

static const struct str_id_pair uia_nav_dir_strs[] = {
    { NavigateDirection_Parent,          "NavigateDirection_Parent" },
    { NavigateDirection_NextSibling,     "NavigateDirection_NextSibling" },
    { NavigateDirection_PreviousSibling, "NavigateDirection_PreviousSibling" },
    { NavigateDirection_FirstChild,      "NavigateDirection_FirstChild" },
    { NavigateDirection_LastChild,       "NavigateDirection_LastChild" },
};

static const struct str_id_pair uia_event_id_strs[] = {
    { UIA_ToolTipOpenedEventId,                             "UIA_ToolTipOpenedEventId" },
    { UIA_ToolTipClosedEventId,                             "UIA_ToolTipClosedEventId", },
    { UIA_StructureChangedEventId,                          "UIA_StructureChangedEventId", },
    { UIA_MenuOpenedEventId,                                "UIA_MenuOpenedEventId", },
    { UIA_AutomationPropertyChangedEventId,                 "UIA_AutomationPropertyChangedEventId", },
    { UIA_AutomationFocusChangedEventId,                    "UIA_AutomationFocusChangedEventId", },
    { UIA_AsyncContentLoadedEventId,                        "UIA_AsyncContentLoadedEventId", },
    { UIA_MenuClosedEventId,                                "UIA_MenuClosedEventId", },
    { UIA_LayoutInvalidatedEventId,                         "UIA_LayoutInvalidatedEventId", },
    { UIA_Invoke_InvokedEventId,                            "UIA_Invoke_InvokedEventId", },
    { UIA_SelectionItem_ElementAddedToSelectionEventId,     "UIA_SelectionItem_ElementAddedToSelectionEventId", },
    { UIA_SelectionItem_ElementRemovedFromSelectionEventId, "UIA_SelectionItem_ElementRemovedFromSelectionEventId", },
    { UIA_SelectionItem_ElementSelectedEventId,             "UIA_SelectionItem_ElementSelectedEventId", },
    { UIA_Selection_InvalidatedEventId,                     "UIA_Selection_InvalidatedEventId", },
    { UIA_Text_TextSelectionChangedEventId,                 "UIA_Text_TextSelectionChangedEventId", },
    { UIA_Text_TextChangedEventId,                          "UIA_Text_TextChangedEventId", },
    { UIA_Window_WindowOpenedEventId,                       "UIA_Window_WindowOpenedEventId", },
    { UIA_Window_WindowClosedEventId,                       "UIA_Window_WindowClosedEventId", },
    { UIA_MenuModeStartEventId,                             "UIA_MenuModeStartEventId", },
    { UIA_MenuModeEndEventId,                               "UIA_MenuModeEndEventId", },
    { UIA_InputReachedTargetEventId,                        "UIA_InputReachedTargetEventId", },
    { UIA_InputReachedOtherElementEventId,                  "UIA_InputReachedOtherElementEventId", },
    { UIA_InputDiscardedEventId,                            "UIA_InputDiscardedEventId", },
    { UIA_SystemAlertEventId,                               "UIA_SystemAlertEventId", },
    { UIA_LiveRegionChangedEventId,                         "UIA_LiveRegionChangedEventId", },
    { UIA_HostedFragmentRootsInvalidatedEventId,            "UIA_HostedFragmentRootsInvalidatedEventId", },
    { UIA_Drag_DragStartEventId,                            "UIA_Drag_DragStartEventId", },
    { UIA_Drag_DragCancelEventId,                           "UIA_Drag_DragCancelEventId", },
    { UIA_Drag_DragCompleteEventId,                         "UIA_Drag_DragCompleteEventId", },
    { UIA_DropTarget_DragEnterEventId,                      "UIA_DropTarget_DragEnterEventId", },
    { UIA_DropTarget_DragLeaveEventId,                      "UIA_DropTarget_DragLeaveEventId", },
    { UIA_DropTarget_DroppedEventId,                        "UIA_DropTarget_DroppedEventId", },
    { UIA_TextEdit_TextChangedEventId,                      "UIA_TextEdit_TextChangedEventId", },
    { UIA_TextEdit_ConversionTargetChangedEventId,          "UIA_TextEdit_ConversionTargetChangedEventId", },
    { UIA_ChangesEventId,                                   "UIA_ChangesEventId", },
    { UIA_NotificationEventId,                              "UIA_NotificationEventId", },
};

static int __cdecl str_id_pair_compare(const void *a, const void *b)
{
    const int *id = a;
    const struct str_id_pair *pair = b;

    return ((*id) > pair->id) - ((*id) < pair->id);
}

#define get_str_for_id(id, id_pair) \
    get_str_from_id_pair( (id), (id_pair), (ARRAY_SIZE(id_pair)) )
static const char *get_str_from_id_pair(int id, const struct str_id_pair *id_pair, int id_pair_size)
{
    const struct str_id_pair *pair;

    if (!(pair = bsearch(&id, id_pair, id_pair_size, sizeof(*pair), str_id_pair_compare)))
        return "";
    else
        return pair->str;
}


#define PROV_METHOD_TRACE(prov, method) \
    if(winetest_debug > 1) printf("%#lx:%#lx: %s_" #method "\n", GetCurrentProcessId(), GetCurrentThreadId(), (prov)->prov_name);

#define PROV_METHOD_TRACE2(prov, method, arg, str_table) \
    if(winetest_debug > 1) printf("%#lx:%#lx: %s_" #method ": %d (%s)\n", GetCurrentProcessId(), GetCurrentThreadId(), (prov)->prov_name, \
            arg, get_str_for_id(arg, str_table)); \

#define ACC_METHOD_TRACE(acc, method) \
    if(winetest_debug > 1) printf("%#lx:%#lx: %s_" #method "\n", GetCurrentProcessId(), GetCurrentThreadId(), (acc)->interface_name);

#define ACC_METHOD_TRACE2(acc, cid, method) \
    if(winetest_debug > 1) printf("%#lx:%#lx: %s_" #method ": %s\n", GetCurrentProcessId(), GetCurrentThreadId(), \
            (acc)->interface_name, debugstr_variant((cid)));

#define DEFINE_EXPECT(func) \
    static int expect_ ## func = 0, called_ ## func = 0

#define SET_EXPECT(func) \
    do { called_ ## func = 0; expect_ ## func = 1; } while(0)

#define SET_EXPECT_MULTI(func, num) \
    do { called_ ## func = 0; expect_ ## func = num; } while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func++; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func--; \
    }while(0)

#define CALLED_COUNT(func) \
    called_ ## func

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = 0; \
    }while(0)

#define CHECK_CALLED_MULTI(func, num) \
    do { \
        ok(called_ ## func == num, "expected " #func " %d times (got %d)\n", num, called_ ## func); \
        expect_ ## func = called_ ## func = 0; \
    }while(0)

#define CHECK_CALLED_AT_LEAST(func, num) \
    do { \
        ok(called_ ## func >= num, "expected " #func " at least %d time(s) (got %d)\n", num, called_ ## func); \
        expect_ ## func = called_ ## func = 0; \
    }while(0)

#define CHECK_CALLED_AT_MOST(func, num) \
    do { \
        ok(called_ ## func <= num, "expected " #func " at most %d time(s) (got %d)\n", num, called_ ## func); \
        expect_ ## func = called_ ## func = 0; \
    }while(0)

#define NAVDIR_INTERNAL_HWND 10
#define UIA_RUNTIME_ID_PREFIX 42

DEFINE_EXPECT(winproc_GETOBJECT_CLIENT);
DEFINE_EXPECT(prov_callback_base_hwnd);
DEFINE_EXPECT(prov_callback_nonclient);
DEFINE_EXPECT(prov_callback_proxy);
DEFINE_EXPECT(prov_callback_parent_proxy);
DEFINE_EXPECT(uia_event_callback);
DEFINE_EXPECT(uia_event_callback2);
DEFINE_EXPECT(uia_com_event_callback);
DEFINE_EXPECT(winproc_GETOBJECT_UiaRoot);
DEFINE_EXPECT(child_winproc_GETOBJECT_UiaRoot);
DEFINE_EXPECT(ProxyEventSink_AddAutomationPropertyChangedEvent);
DEFINE_EXPECT(ProxyEventSink_AddAutomationEvent);
DEFINE_EXPECT(ProxyEventSink_AddStructureChangedEvent);

static BOOL check_variant_i4(VARIANT *v, int val)
{
    if (V_VT(v) == VT_I4 && V_I4(v) == val)
        return TRUE;

    return FALSE;
}

static BOOL check_variant_bool(VARIANT *v, BOOL val)
{
    if (V_VT(v) == VT_BOOL && V_BOOL(v) == (val ? VARIANT_TRUE : VARIANT_FALSE))
        return TRUE;

    return FALSE;
}

static void variant_init_bool(VARIANT *v, BOOL val)
{
    V_VT(v) = VT_BOOL;
    V_BOOL(v) = val ? VARIANT_TRUE : VARIANT_FALSE;
}

static BOOL iface_cmp(IUnknown *iface1, IUnknown *iface2)
{
    IUnknown *unk1, *unk2;
    BOOL cmp;

    IUnknown_QueryInterface(iface1, &IID_IUnknown, (void**)&unk1);
    IUnknown_QueryInterface(iface2, &IID_IUnknown, (void**)&unk2);
    cmp = unk1 == unk2;

    IUnknown_Release(unk1);
    IUnknown_Release(unk2);
    return cmp;
}

#define test_implements_interface( unk, iid, exp_implemented ) \
        test_implements_interface_( ((IUnknown *)(unk)), (iid), (exp_implemented), __FILE__, __LINE__)
static void test_implements_interface_(IUnknown *unk, const GUID *iid, BOOL exp_implemented, const char *file, int line)
{
    IUnknown *unk2 = NULL;
    HRESULT hr;

    hr = IUnknown_QueryInterface(unk, iid, (void **)&unk2);
    ok_(file, line)(hr == (exp_implemented ? S_OK : E_NOINTERFACE), "Unexpected hr %#lx\n", hr);
    ok_(file, line)(!!unk2 == exp_implemented, "Unexpected iface %p\n", unk2);
    if (unk2)
        IUnknown_Release(unk2);
}

#define check_interface_marshal_proxy_creation( iface, iid, expect_proxy ) \
        check_interface_marshal_proxy_creation_( (iface), (iid), (expect_proxy), __FILE__, __LINE__)
static void check_interface_marshal_proxy_creation_(IUnknown *iface, REFIID iid, BOOL expect_proxy, const char *file,
        int line);

#define DEFINE_ACC_METHOD_EXPECT(method) \
    int expect_ ## method , called_ ## method

#define DEFINE_ACC_METHOD_EXPECTS \
    DEFINE_ACC_METHOD_EXPECT(QI_IAccIdentity); \
    DEFINE_ACC_METHOD_EXPECT(get_accParent); \
    DEFINE_ACC_METHOD_EXPECT(get_accChildCount); \
    DEFINE_ACC_METHOD_EXPECT(get_accChild); \
    DEFINE_ACC_METHOD_EXPECT(get_accName); \
    DEFINE_ACC_METHOD_EXPECT(get_accValue); \
    DEFINE_ACC_METHOD_EXPECT(get_accDescription); \
    DEFINE_ACC_METHOD_EXPECT(get_accRole); \
    DEFINE_ACC_METHOD_EXPECT(get_accState); \
    DEFINE_ACC_METHOD_EXPECT(get_accHelp); \
    DEFINE_ACC_METHOD_EXPECT(get_accHelpTopic); \
    DEFINE_ACC_METHOD_EXPECT(get_accKeyboardShortcut); \
    DEFINE_ACC_METHOD_EXPECT(get_accFocus); \
    DEFINE_ACC_METHOD_EXPECT(get_accSelection); \
    DEFINE_ACC_METHOD_EXPECT(get_accDefaultAction); \
    DEFINE_ACC_METHOD_EXPECT(accSelect); \
    DEFINE_ACC_METHOD_EXPECT(accLocation); \
    DEFINE_ACC_METHOD_EXPECT(accNavigate); \
    DEFINE_ACC_METHOD_EXPECT(accHitTest); \
    DEFINE_ACC_METHOD_EXPECT(accDoDefaultAction); \
    DEFINE_ACC_METHOD_EXPECT(put_accName); \
    DEFINE_ACC_METHOD_EXPECT(put_accValue); \
    DEFINE_ACC_METHOD_EXPECT(get_nRelations); \
    DEFINE_ACC_METHOD_EXPECT(get_relation); \
    DEFINE_ACC_METHOD_EXPECT(get_relations); \
    DEFINE_ACC_METHOD_EXPECT(role); \
    DEFINE_ACC_METHOD_EXPECT(scrollTo); \
    DEFINE_ACC_METHOD_EXPECT(scrollToPoint); \
    DEFINE_ACC_METHOD_EXPECT(get_groupPosition); \
    DEFINE_ACC_METHOD_EXPECT(get_states); \
    DEFINE_ACC_METHOD_EXPECT(get_extendedRole); \
    DEFINE_ACC_METHOD_EXPECT(get_localizedExtendedRole); \
    DEFINE_ACC_METHOD_EXPECT(get_nExtendedStates); \
    DEFINE_ACC_METHOD_EXPECT(get_extendedStates); \
    DEFINE_ACC_METHOD_EXPECT(get_localizedExtendedStates); \
    DEFINE_ACC_METHOD_EXPECT(get_uniqueID); \
    DEFINE_ACC_METHOD_EXPECT(get_windowHandle); \
    DEFINE_ACC_METHOD_EXPECT(get_indexInParent); \
    DEFINE_ACC_METHOD_EXPECT(get_locale); \
    DEFINE_ACC_METHOD_EXPECT(get_attributes) \

static DWORD msg_wait_for_all_events(HANDLE *event_handles, int event_handle_count, DWORD timeout_val)
{
    int events_handled = 0;
    DWORD wait_res;

    while ((wait_res = MsgWaitForMultipleObjects(event_handle_count, (const HANDLE *)event_handles, FALSE, timeout_val,
                    QS_ALLINPUT)) <= (WAIT_OBJECT_0 + event_handle_count))
    {
        if (wait_res == (WAIT_OBJECT_0 + event_handle_count))
        {
            MSG msg;

            while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
        else
            events_handled++;

        if (events_handled == event_handle_count)
            break;
    }

    return wait_res;
}

static struct Accessible
{
    IAccessible IAccessible_iface;
    IAccessible2 IAccessible2_iface;
    IOleWindow IOleWindow_iface;
    IServiceProvider IServiceProvider_iface;
    LONG ref;

    const char *interface_name;

    IAccessible *parent;
    HWND acc_hwnd;
    HWND ow_hwnd;
    INT role;
    INT state;
    LONG child_count;
    LPCWSTR name;
    LONG left, top, width, height;
    BOOL enable_ia2;
    LONG unique_id;
    INT focus_child_id;
    IAccessible *focus_acc;
    DEFINE_ACC_METHOD_EXPECTS;
} Accessible, Accessible2, Accessible_child, Accessible_child2;

#define SET_ACC_METHOD_EXPECT(acc, method) \
    do { (acc)->called_ ## method = 0; (acc)->expect_ ## method = 1; } while(0)

#define SET_ACC_METHOD_EXPECT_MULTI(acc, method, num) \
    do { (acc)->called_ ## method = 0; (acc)->expect_ ## method = num; } while(0)

#define CHECK_ACC_METHOD_EXPECT2(acc, method) \
    do { \
        ok((acc)->expect_ ##method, "unexpected call %s_" #method "\n", (acc)->interface_name); \
        (acc)->called_ ## method++; \
    }while(0)

#define CHECK_ACC_METHOD_EXPECT(acc, method) \
    do { \
        CHECK_ACC_METHOD_EXPECT2(acc, method); \
        (acc)->expect_ ## method--; \
    }while(0)

#define CHECK_ACC_METHOD_CALLED(acc, method) \
    do { \
        ok((acc)->called_ ## method, "expected %s_" #method "\n", (acc)->interface_name); \
        (acc)->expect_ ## method = (acc)->called_ ## method = 0; \
    }while(0)

#define CHECK_ACC_METHOD_CALLED_MULTI(acc, method, num) \
    do { \
        ok((acc)->called_ ## method == num, "expected %s_" #method " %d times (got %d)\n", (acc)->interface_name, num, (acc)->called_ ## method); \
        (acc)->expect_ ## method = (acc)->called_ ## method = 0; \
    }while(0)

#define CHECK_ACC_METHOD_CALLED_AT_LEAST(acc, method, num) \
    do { \
        ok((acc)->called_ ## method >= num, "expected %s_" #method " at least %d time(s) (got %d)\n", (acc)->interface_name, num, (acc)->called_ ## method); \
        (acc)->expect_ ## method = (acc)->called_ ## method = 0; \
    }while(0)

#define CHECK_ACC_METHOD_CALLED_AT_MOST(acc, method, num) \
    do { \
        ok((acc)->called_ ## method <= num, "expected %s_" #method " at most %d time(s) (got %d)\n", (acc)->interface_name, num, (acc)->called_ ## method); \
        (acc)->expect_ ## method = (acc)->called_ ## method = 0; \
    }while(0)

static inline struct Accessible* impl_from_Accessible(IAccessible *iface)
{
    return CONTAINING_RECORD(iface, struct Accessible, IAccessible_iface);
}

static HRESULT WINAPI Accessible_QueryInterface(IAccessible *iface, REFIID riid, void **obj)
{
    struct Accessible *This = impl_from_Accessible(iface);

    *obj = NULL;
    if (IsEqualIID(riid, &IID_IAccIdentity))
    {
        CHECK_ACC_METHOD_EXPECT(This, QI_IAccIdentity);
        ACC_METHOD_TRACE(This, QI_IAccIdentity);
        return E_NOINTERFACE;
    }

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDispatch) ||
            IsEqualIID(riid, &IID_IAccessible))
        *obj = iface;
    else if (IsEqualIID(riid, &IID_IOleWindow))
        *obj = &This->IOleWindow_iface;
    else if (IsEqualIID(riid, &IID_IServiceProvider))
        *obj = &This->IServiceProvider_iface;
    else if (IsEqualIID(riid, &IID_IAccessible2) && This->enable_ia2)
        *obj = &This->IAccessible2_iface;
    else
        return E_NOINTERFACE;

    IAccessible_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI Accessible_AddRef(IAccessible *iface)
{
    struct Accessible *This = impl_from_Accessible(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI Accessible_Release(IAccessible *iface)
{
    struct Accessible *This = impl_from_Accessible(iface);
    return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI Accessible_GetTypeInfoCount(IAccessible *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_GetTypeInfo(IAccessible *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **out_tinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_GetIDsOfNames(IAccessible *iface, REFIID riid,
        LPOLESTR *rg_names, UINT name_count, LCID lcid, DISPID *rg_disp_id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_Invoke(IAccessible *iface, DISPID disp_id_member,
        REFIID riid, LCID lcid, WORD flags, DISPPARAMS *disp_params,
        VARIANT *var_result, EXCEPINFO *excep_info, UINT *arg_err)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accParent(IAccessible *iface, IDispatch **out_parent)
{
    struct Accessible *This = impl_from_Accessible(iface);

    CHECK_ACC_METHOD_EXPECT(This, get_accParent);
    ACC_METHOD_TRACE(This, get_accParent);
    if (This->parent)
        return IAccessible_QueryInterface(This->parent, &IID_IDispatch, (void **)out_parent);

    *out_parent = NULL;
    return S_FALSE;
}

static HRESULT WINAPI Accessible_get_accChildCount(IAccessible *iface, LONG *out_count)
{
    struct Accessible *This = impl_from_Accessible(iface);

    CHECK_ACC_METHOD_EXPECT(This, get_accChildCount);
    ACC_METHOD_TRACE(This, get_accChildCount);
    if (This->child_count)
    {
        *out_count = This->child_count;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accChild(IAccessible *iface, VARIANT child_id,
        IDispatch **out_child)
{
    struct Accessible *This = impl_from_Accessible(iface);

    CHECK_ACC_METHOD_EXPECT(This, get_accChild);
    ACC_METHOD_TRACE2(This, &child_id, get_accChild);

    *out_child = NULL;
    if (V_VT(&child_id) != VT_I4)
        return E_INVALIDARG;

    if (This == &Accessible)
    {
        switch (V_I4(&child_id))
        {
        case CHILDID_SELF:
            return IAccessible_QueryInterface(&This->IAccessible_iface, &IID_IDispatch, (void **)out_child);

        /* Simple element children. */
        case 1:
        case 3:
            return S_FALSE;

        case 2:
            return IAccessible_QueryInterface(&Accessible_child.IAccessible_iface, &IID_IDispatch, (void **)out_child);

        case 4:
            return IAccessible_QueryInterface(&Accessible_child2.IAccessible_iface, &IID_IDispatch, (void **)out_child);

        case 7:
            return IAccessible_QueryInterface(&Accessible.IAccessible_iface, &IID_IDispatch, (void **)out_child);

        default:
            break;

        }
    }
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accName(IAccessible *iface, VARIANT child_id,
        BSTR *out_name)
{
    struct Accessible *This = impl_from_Accessible(iface);

    CHECK_ACC_METHOD_EXPECT(This, get_accName);
    ACC_METHOD_TRACE2(This, &child_id, get_accName);

    *out_name = NULL;
    if (This->name)
    {
        *out_name = SysAllocString(This->name);
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accValue(IAccessible *iface, VARIANT child_id,
        BSTR *out_value)
{
    struct Accessible *This = impl_from_Accessible(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_accValue);
    ACC_METHOD_TRACE2(This, &child_id, get_accValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accDescription(IAccessible *iface, VARIANT child_id,
        BSTR *out_description)
{
    struct Accessible *This = impl_from_Accessible(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_accDescription);
    ACC_METHOD_TRACE2(This, &child_id, get_accDescription);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accRole(IAccessible *iface, VARIANT child_id,
        VARIANT *out_role)
{
    struct Accessible *This = impl_from_Accessible(iface);

    CHECK_ACC_METHOD_EXPECT(This, get_accRole);
    ACC_METHOD_TRACE2(This, &child_id, get_accRole);

    if (This->role)
    {
        V_VT(out_role) = VT_I4;
        V_I4(out_role) = This->role;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accState(IAccessible *iface, VARIANT child_id,
        VARIANT *out_state)
{
    struct Accessible *This = impl_from_Accessible(iface);

    CHECK_ACC_METHOD_EXPECT(This, get_accState);
    ACC_METHOD_TRACE2(This, &child_id, get_accState);

    if (V_VT(&child_id) != VT_I4)
        return E_INVALIDARG;

    if (This == &Accessible && V_I4(&child_id) != CHILDID_SELF)
    {
        switch (V_I4(&child_id))
        {
        case 1:
            V_VT(out_state) = VT_I4;
            V_I4(out_state) = STATE_SYSTEM_INVISIBLE;
            break;

        case 3:
            V_VT(out_state) = VT_I4;
            V_I4(out_state) = STATE_SYSTEM_FOCUSABLE;
            break;

        default:
            return E_INVALIDARG;
        }

        return S_OK;
    }

    if (This->state)
    {
        V_VT(out_state) = VT_I4;
        V_I4(out_state) = This->state;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accHelp(IAccessible *iface, VARIANT child_id,
        BSTR *out_help)
{
    struct Accessible *This = impl_from_Accessible(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_accHelp);
    ACC_METHOD_TRACE2(This, &child_id, get_accHelp);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accHelpTopic(IAccessible *iface,
        BSTR *out_help_file, VARIANT child_id, LONG *out_topic_id)
{
    struct Accessible *This = impl_from_Accessible(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_accHelpTopic);
    ACC_METHOD_TRACE2(This, &child_id, get_accHelpTopic);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accKeyboardShortcut(IAccessible *iface, VARIANT child_id,
        BSTR *out_kbd_shortcut)
{
    struct Accessible *This = impl_from_Accessible(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_accKeyboardShortcut);
    ACC_METHOD_TRACE2(This, &child_id, get_accKeyboardShortcut);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accFocus(IAccessible *iface, VARIANT *pchild_id)
{
    struct Accessible *This = impl_from_Accessible(iface);

    CHECK_ACC_METHOD_EXPECT(This, get_accFocus);
    ACC_METHOD_TRACE(This, get_accFocus);

    VariantInit(pchild_id);
    if (This->focus_acc)
    {
        HRESULT hr;

        hr = IAccessible_QueryInterface(This->focus_acc, &IID_IDispatch, (void **)&V_DISPATCH(pchild_id));
        if (SUCCEEDED(hr))
            V_VT(pchild_id) = VT_DISPATCH;

        return hr;
    }
    else if (This->focus_child_id >= 0)
    {
        V_VT(pchild_id) = VT_I4;
        V_I4(pchild_id) = This->focus_child_id;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accSelection(IAccessible *iface, VARIANT *out_selection)
{
    struct Accessible *This = impl_from_Accessible(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_accSelection);
    ACC_METHOD_TRACE(This, get_accSelection);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accDefaultAction(IAccessible *iface, VARIANT child_id,
        BSTR *out_default_action)
{
    struct Accessible *This = impl_from_Accessible(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_accDefaultAction);
    ACC_METHOD_TRACE2(This, &child_id, get_accDefaultAction);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accSelect(IAccessible *iface, LONG select_flags,
        VARIANT child_id)
{
    struct Accessible *This = impl_from_Accessible(iface);
    CHECK_ACC_METHOD_EXPECT(This, accSelect);
    ACC_METHOD_TRACE2(This, &child_id, accSelect);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accLocation(IAccessible *iface, LONG *out_left,
        LONG *out_top, LONG *out_width, LONG *out_height, VARIANT child_id)
{
    struct Accessible *This = impl_from_Accessible(iface);

    CHECK_ACC_METHOD_EXPECT(This, accLocation);
    ACC_METHOD_TRACE2(This, &child_id, accLocation);

    if (This->width && This->height)
    {
        *out_left = This->left;
        *out_top = This->top;
        *out_width = This->width;
        *out_height = This->height;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accNavigate(IAccessible *iface, LONG nav_direction,
        VARIANT child_id_start, VARIANT *out_var)
{
    struct Accessible *This = impl_from_Accessible(iface);

    CHECK_ACC_METHOD_EXPECT(This, accNavigate);
    ACC_METHOD_TRACE2(This, &child_id_start, accNavigate);

    VariantInit(out_var);

    /*
     * This is an undocumented way for UI Automation to get an HWND for
     * IAccessible's contained in a Direct Annotation wrapper object.
     */
    if ((nav_direction == NAVDIR_INTERNAL_HWND) && check_variant_i4(&child_id_start, CHILDID_SELF) &&
            This->acc_hwnd)
    {
        V_VT(out_var) = VT_I4;
        V_I4(out_var) = HandleToUlong(This->acc_hwnd);
        return S_OK;
    }
    return S_FALSE;
}

static HRESULT WINAPI Accessible_accHitTest(IAccessible *iface, LONG left, LONG top,
        VARIANT *out_child_id)
{
    struct Accessible *This = impl_from_Accessible(iface);
    CHECK_ACC_METHOD_EXPECT(This, accHitTest);
    ACC_METHOD_TRACE(This, accHitTest);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accDoDefaultAction(IAccessible *iface, VARIANT child_id)
{
    struct Accessible *This = impl_from_Accessible(iface);
    CHECK_ACC_METHOD_EXPECT(This, accDoDefaultAction);
    ACC_METHOD_TRACE2(This, &child_id, accDoDefaultAction);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_put_accName(IAccessible *iface, VARIANT child_id,
        BSTR name)
{
    struct Accessible *This = impl_from_Accessible(iface);
    CHECK_ACC_METHOD_EXPECT(This, put_accName);
    ACC_METHOD_TRACE2(This, &child_id, put_accName);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_put_accValue(IAccessible *iface, VARIANT child_id,
        BSTR value)
{
    struct Accessible *This = impl_from_Accessible(iface);
    CHECK_ACC_METHOD_EXPECT(This, put_accValue);
    ACC_METHOD_TRACE2(This, &child_id, put_accValue);
    return E_NOTIMPL;
}

static IAccessibleVtbl AccessibleVtbl = {
    Accessible_QueryInterface,
    Accessible_AddRef,
    Accessible_Release,
    Accessible_GetTypeInfoCount,
    Accessible_GetTypeInfo,
    Accessible_GetIDsOfNames,
    Accessible_Invoke,
    Accessible_get_accParent,
    Accessible_get_accChildCount,
    Accessible_get_accChild,
    Accessible_get_accName,
    Accessible_get_accValue,
    Accessible_get_accDescription,
    Accessible_get_accRole,
    Accessible_get_accState,
    Accessible_get_accHelp,
    Accessible_get_accHelpTopic,
    Accessible_get_accKeyboardShortcut,
    Accessible_get_accFocus,
    Accessible_get_accSelection,
    Accessible_get_accDefaultAction,
    Accessible_accSelect,
    Accessible_accLocation,
    Accessible_accNavigate,
    Accessible_accHitTest,
    Accessible_accDoDefaultAction,
    Accessible_put_accName,
    Accessible_put_accValue
};

static inline struct Accessible* impl_from_Accessible2(IAccessible2 *iface)
{
    return CONTAINING_RECORD(iface, struct Accessible, IAccessible2_iface);
}

static HRESULT WINAPI Accessible2_QueryInterface(IAccessible2 *iface, REFIID riid, void **obj)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_QueryInterface(&This->IAccessible_iface, riid, obj);
}

static ULONG WINAPI Accessible2_AddRef(IAccessible2 *iface)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_AddRef(&This->IAccessible_iface);
}

static ULONG WINAPI Accessible2_Release(IAccessible2 *iface)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_Release(&This->IAccessible_iface);
}

static HRESULT WINAPI Accessible2_GetTypeInfoCount(IAccessible2 *iface, UINT *pctinfo)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_GetTypeInfoCount(&This->IAccessible_iface, pctinfo);
}

static HRESULT WINAPI Accessible2_GetTypeInfo(IAccessible2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **out_tinfo)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_GetTypeInfo(&This->IAccessible_iface, iTInfo, lcid, out_tinfo);
}

static HRESULT WINAPI Accessible2_GetIDsOfNames(IAccessible2 *iface, REFIID riid,
        LPOLESTR *rg_names, UINT name_count, LCID lcid, DISPID *rg_disp_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_GetIDsOfNames(&This->IAccessible_iface, riid, rg_names, name_count,
            lcid, rg_disp_id);
}

static HRESULT WINAPI Accessible2_Invoke(IAccessible2 *iface, DISPID disp_id_member,
        REFIID riid, LCID lcid, WORD flags, DISPPARAMS *disp_params,
        VARIANT *var_result, EXCEPINFO *excep_info, UINT *arg_err)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_Invoke(&This->IAccessible_iface, disp_id_member, riid, lcid, flags,
            disp_params, var_result, excep_info, arg_err);
}

static HRESULT WINAPI Accessible2_get_accParent(IAccessible2 *iface, IDispatch **out_parent)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accParent(&This->IAccessible_iface, out_parent);
}

static HRESULT WINAPI Accessible2_get_accChildCount(IAccessible2 *iface, LONG *out_count)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accChildCount(&This->IAccessible_iface, out_count);
}

static HRESULT WINAPI Accessible2_get_accChild(IAccessible2 *iface, VARIANT child_id,
        IDispatch **out_child)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accChild(&This->IAccessible_iface, child_id, out_child);
}

static HRESULT WINAPI Accessible2_get_accName(IAccessible2 *iface, VARIANT child_id,
        BSTR *out_name)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accName(&This->IAccessible_iface, child_id, out_name);
}

static HRESULT WINAPI Accessible2_get_accValue(IAccessible2 *iface, VARIANT child_id,
        BSTR *out_value)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accValue(&This->IAccessible_iface, child_id, out_value);
}

static HRESULT WINAPI Accessible2_get_accDescription(IAccessible2 *iface, VARIANT child_id,
        BSTR *out_description)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accDescription(&This->IAccessible_iface, child_id, out_description);
}

static HRESULT WINAPI Accessible2_get_accRole(IAccessible2 *iface, VARIANT child_id,
        VARIANT *out_role)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accRole(&This->IAccessible_iface, child_id, out_role);
}

static HRESULT WINAPI Accessible2_get_accState(IAccessible2 *iface, VARIANT child_id,
        VARIANT *out_state)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accState(&This->IAccessible_iface, child_id, out_state);
}

static HRESULT WINAPI Accessible2_get_accHelp(IAccessible2 *iface, VARIANT child_id,
        BSTR *out_help)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accHelp(&This->IAccessible_iface, child_id, out_help);
}

static HRESULT WINAPI Accessible2_get_accHelpTopic(IAccessible2 *iface,
        BSTR *out_help_file, VARIANT child_id, LONG *out_topic_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accHelpTopic(&This->IAccessible_iface, out_help_file, child_id,
            out_topic_id);
}

static HRESULT WINAPI Accessible2_get_accKeyboardShortcut(IAccessible2 *iface, VARIANT child_id,
        BSTR *out_kbd_shortcut)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accKeyboardShortcut(&This->IAccessible_iface, child_id,
            out_kbd_shortcut);
}

static HRESULT WINAPI Accessible2_get_accFocus(IAccessible2 *iface, VARIANT *pchild_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accFocus(&This->IAccessible_iface, pchild_id);
}

static HRESULT WINAPI Accessible2_get_accSelection(IAccessible2 *iface, VARIANT *out_selection)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accSelection(&This->IAccessible_iface, out_selection);
}

static HRESULT WINAPI Accessible2_get_accDefaultAction(IAccessible2 *iface, VARIANT child_id,
        BSTR *out_default_action)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_get_accDefaultAction(&This->IAccessible_iface, child_id,
            out_default_action);
}

static HRESULT WINAPI Accessible2_accSelect(IAccessible2 *iface, LONG select_flags,
        VARIANT child_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_accSelect(&This->IAccessible_iface, select_flags, child_id);
}

static HRESULT WINAPI Accessible2_accLocation(IAccessible2 *iface, LONG *out_left,
        LONG *out_top, LONG *out_width, LONG *out_height, VARIANT child_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_accLocation(&This->IAccessible_iface, out_left, out_top, out_width,
            out_height, child_id);
}

static HRESULT WINAPI Accessible2_accNavigate(IAccessible2 *iface, LONG nav_direction,
        VARIANT child_id_start, VARIANT *out_var)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_accNavigate(&This->IAccessible_iface, nav_direction, child_id_start,
            out_var);
}

static HRESULT WINAPI Accessible2_accHitTest(IAccessible2 *iface, LONG left, LONG top,
        VARIANT *out_child_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_accHitTest(&This->IAccessible_iface, left, top, out_child_id);
}

static HRESULT WINAPI Accessible2_accDoDefaultAction(IAccessible2 *iface, VARIANT child_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_accDoDefaultAction(&This->IAccessible_iface, child_id);
}

static HRESULT WINAPI Accessible2_put_accName(IAccessible2 *iface, VARIANT child_id,
        BSTR name)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_put_accName(&This->IAccessible_iface, child_id, name);
}

static HRESULT WINAPI Accessible2_put_accValue(IAccessible2 *iface, VARIANT child_id,
        BSTR value)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    return IAccessible_put_accValue(&This->IAccessible_iface, child_id, value);
}

static HRESULT WINAPI Accessible2_get_nRelations(IAccessible2 *iface, LONG *out_nRelations)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_nRelations);
    ACC_METHOD_TRACE(This, get_nRelations);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_relation(IAccessible2 *iface, LONG relation_idx,
        IAccessibleRelation **out_relation)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_relation);
    ACC_METHOD_TRACE(This, get_relation);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_relations(IAccessible2 *iface, LONG count,
        IAccessibleRelation **out_relations, LONG *out_relation_count)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_relations);
    ACC_METHOD_TRACE(This, get_relations);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_role(IAccessible2 *iface, LONG *out_role)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, role);
    ACC_METHOD_TRACE(This, role);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_scrollTo(IAccessible2 *iface, enum IA2ScrollType scroll_type)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, scrollTo);
    ACC_METHOD_TRACE(This, scrollTo);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_scrollToPoint(IAccessible2 *iface,
        enum IA2CoordinateType coordinate_type, LONG x, LONG y)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, scrollToPoint);
    ACC_METHOD_TRACE(This, scrollToPoint);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_groupPosition(IAccessible2 *iface, LONG *out_group_level,
        LONG *out_similar_items_in_group, LONG *out_position_in_group)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_groupPosition);
    ACC_METHOD_TRACE(This, get_groupPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_states(IAccessible2 *iface, AccessibleStates *out_states)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_states);
    ACC_METHOD_TRACE(This, get_states);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_extendedRole(IAccessible2 *iface, BSTR *out_extended_role)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_extendedRole);
    ACC_METHOD_TRACE(This, get_extendedRole);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_localizedExtendedRole(IAccessible2 *iface,
        BSTR *out_localized_extended_role)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_localizedExtendedRole);
    ACC_METHOD_TRACE(This, get_localizedExtendedRole);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_nExtendedStates(IAccessible2 *iface, LONG *out_nExtendedStates)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_nExtendedStates);
    ACC_METHOD_TRACE(This, get_nExtendedStates);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_extendedStates(IAccessible2 *iface, LONG count,
        BSTR **out_extended_states, LONG *out_extended_states_count)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_extendedStates);
    ACC_METHOD_TRACE(This, get_extendedStates);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_localizedExtendedStates(IAccessible2 *iface, LONG count,
        BSTR **out_localized_extended_states, LONG *out_localized_extended_states_count)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_localizedExtendedStates);
    ACC_METHOD_TRACE(This, get_localizedExtendedStates);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_uniqueID(IAccessible2 *iface, LONG *out_unique_id)
{
    struct Accessible *This = impl_from_Accessible2(iface);

    CHECK_ACC_METHOD_EXPECT(This, get_uniqueID);
    ACC_METHOD_TRACE(This, get_uniqueID);

    *out_unique_id = 0;
    if (This->unique_id)
    {
        *out_unique_id = This->unique_id;
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_windowHandle(IAccessible2 *iface, HWND *out_hwnd)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_windowHandle);
    ACC_METHOD_TRACE(This, get_windowHandle);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_indexInParent(IAccessible2 *iface, LONG *out_idx_in_parent)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_indexInParent);
    ACC_METHOD_TRACE(This, get_indexInParent);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_locale(IAccessible2 *iface, IA2Locale *out_locale)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_locale);
    ACC_METHOD_TRACE(This, get_locale);
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible2_get_attributes(IAccessible2 *iface, BSTR *out_attributes)
{
    struct Accessible *This = impl_from_Accessible2(iface);
    CHECK_ACC_METHOD_EXPECT(This, get_attributes);
    ACC_METHOD_TRACE(This, get_attributes);
    return E_NOTIMPL;
}

static const IAccessible2Vtbl Accessible2Vtbl = {
    Accessible2_QueryInterface,
    Accessible2_AddRef,
    Accessible2_Release,
    Accessible2_GetTypeInfoCount,
    Accessible2_GetTypeInfo,
    Accessible2_GetIDsOfNames,
    Accessible2_Invoke,
    Accessible2_get_accParent,
    Accessible2_get_accChildCount,
    Accessible2_get_accChild,
    Accessible2_get_accName,
    Accessible2_get_accValue,
    Accessible2_get_accDescription,
    Accessible2_get_accRole,
    Accessible2_get_accState,
    Accessible2_get_accHelp,
    Accessible2_get_accHelpTopic,
    Accessible2_get_accKeyboardShortcut,
    Accessible2_get_accFocus,
    Accessible2_get_accSelection,
    Accessible2_get_accDefaultAction,
    Accessible2_accSelect,
    Accessible2_accLocation,
    Accessible2_accNavigate,
    Accessible2_accHitTest,
    Accessible2_accDoDefaultAction,
    Accessible2_put_accName,
    Accessible2_put_accValue,
    Accessible2_get_nRelations,
    Accessible2_get_relation,
    Accessible2_get_relations,
    Accessible2_role,
    Accessible2_scrollTo,
    Accessible2_scrollToPoint,
    Accessible2_get_groupPosition,
    Accessible2_get_states,
    Accessible2_get_extendedRole,
    Accessible2_get_localizedExtendedRole,
    Accessible2_get_nExtendedStates,
    Accessible2_get_extendedStates,
    Accessible2_get_localizedExtendedStates,
    Accessible2_get_uniqueID,
    Accessible2_get_windowHandle,
    Accessible2_get_indexInParent,
    Accessible2_get_locale,
    Accessible2_get_attributes,
};

static inline struct Accessible* impl_from_OleWindow(IOleWindow *iface)
{
    return CONTAINING_RECORD(iface, struct Accessible, IOleWindow_iface);
}

static HRESULT WINAPI OleWindow_QueryInterface(IOleWindow *iface, REFIID riid, void **obj)
{
    struct Accessible *This = impl_from_OleWindow(iface);
    return IAccessible_QueryInterface(&This->IAccessible_iface, riid, obj);
}

static ULONG WINAPI OleWindow_AddRef(IOleWindow *iface)
{
    struct Accessible *This = impl_from_OleWindow(iface);
    return IAccessible_AddRef(&This->IAccessible_iface);
}

static ULONG WINAPI OleWindow_Release(IOleWindow *iface)
{
    struct Accessible *This = impl_from_OleWindow(iface);
    return IAccessible_Release(&This->IAccessible_iface);
}

static HRESULT WINAPI OleWindow_GetWindow(IOleWindow *iface, HWND *hwnd)
{
    struct Accessible *This = impl_from_OleWindow(iface);

    *hwnd = This->ow_hwnd;
    return *hwnd ? S_OK : E_FAIL;
}

static HRESULT WINAPI OleWindow_ContextSensitiveHelp(IOleWindow *iface, BOOL f_enter_mode)
{
    return E_NOTIMPL;
}

static const IOleWindowVtbl OleWindowVtbl = {
    OleWindow_QueryInterface,
    OleWindow_AddRef,
    OleWindow_Release,
    OleWindow_GetWindow,
    OleWindow_ContextSensitiveHelp
};

static inline struct Accessible* impl_from_ServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, struct Accessible, IServiceProvider_iface);
}

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **obj)
{
    struct Accessible *This = impl_from_ServiceProvider(iface);
    return IAccessible_QueryInterface(&This->IAccessible_iface, riid, obj);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    struct Accessible *This = impl_from_ServiceProvider(iface);
    return IAccessible_AddRef(&This->IAccessible_iface);
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    struct Accessible *This = impl_from_ServiceProvider(iface);
    return IAccessible_Release(&This->IAccessible_iface);
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID service_guid,
        REFIID riid, void **obj)
{
    struct Accessible *This = impl_from_ServiceProvider(iface);

    if (IsEqualIID(riid, &IID_IAccessible2) && IsEqualIID(service_guid, &IID_IAccessible2) &&
            This->enable_ia2)
        return IAccessible_QueryInterface(&This->IAccessible_iface, riid, obj);

    return E_NOTIMPL;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService,
};

static struct Accessible Accessible =
{
    { &AccessibleVtbl },
    { &Accessible2Vtbl },
    { &OleWindowVtbl },
    { &ServiceProviderVtbl },
    1,
    "Accessible",
    NULL,
    0, 0,
    0, 0, 0, NULL,
    0, 0, 0, 0,
    FALSE, 0,
    CHILDID_SELF, NULL,
};

static struct Accessible Accessible2 =
{
    { &AccessibleVtbl },
    { &Accessible2Vtbl },
    { &OleWindowVtbl },
    { &ServiceProviderVtbl },
    1,
    "Accessible2",
    NULL,
    0, 0,
    0, 0, 0, NULL,
    0, 0, 0, 0,
    FALSE, 0,
    CHILDID_SELF, NULL,
};

static struct Accessible Accessible_child =
{
    { &AccessibleVtbl },
    { &Accessible2Vtbl },
    { &OleWindowVtbl },
    { &ServiceProviderVtbl },
    1,
    "Accessible_child",
    &Accessible.IAccessible_iface,
    0, 0,
    0, 0, 0, NULL,
    0, 0, 0, 0,
    FALSE, 0,
    CHILDID_SELF, NULL,
};

static struct Accessible Accessible_child2 =
{
    { &AccessibleVtbl },
    { &Accessible2Vtbl },
    { &OleWindowVtbl },
    { &ServiceProviderVtbl },
    1,
    "Accessible_child2",
    &Accessible.IAccessible_iface,
    0, 0,
    0, 0, 0, NULL,
    0, 0, 0, 0,
    FALSE, 0,
    CHILDID_SELF, NULL,
};

struct Provider_prop_override
{
    int prop_id;
    VARIANT val;
};

struct Provider_value_pattern_data
{
    BOOL is_supported;
    BOOL is_read_only;
};

struct Provider_legacy_accessible_pattern_data
{
    BOOL is_supported;
    int child_id;
    DWORD role;
};

struct Provider_win_event_handler_data
{
    BOOL is_supported;

    DWORD exp_win_event_id;
    HWND exp_win_event_hwnd;
    LONG exp_win_event_obj_id;
    LONG exp_win_event_child_id;

    IRawElementProviderSimple *responder_prov;
    int responder_event;
};

static struct Provider
{
    IRawElementProviderSimple IRawElementProviderSimple_iface;
    IRawElementProviderFragment IRawElementProviderFragment_iface;
    IRawElementProviderFragmentRoot IRawElementProviderFragmentRoot_iface;
    IRawElementProviderHwndOverride IRawElementProviderHwndOverride_iface;
    IRawElementProviderAdviseEvents IRawElementProviderAdviseEvents_iface;
    IProxyProviderWinEventHandler IProxyProviderWinEventHandler_iface;
    IValueProvider IValueProvider_iface;
    ILegacyIAccessibleProvider ILegacyIAccessibleProvider_iface;
    LONG ref;

    const char *prov_name;
    IRawElementProviderFragment *parent;
    IRawElementProviderFragmentRoot *frag_root;
    IRawElementProviderFragment *prev_sibling;
    IRawElementProviderFragment *next_sibling;
    IRawElementProviderFragment *first_child;
    IRawElementProviderFragment *last_child;
    enum ProviderOptions prov_opts;
    HWND hwnd;
    BOOL ret_invalid_prop_type;
    DWORD expected_tid;
    int runtime_id[2];
    DWORD last_call_tid;
    BOOL ignore_hwnd_prop;
    HWND override_hwnd;
    struct Provider_prop_override *prop_override;
    int prop_override_count;
    struct UiaRect bounds_rect;
    struct Provider_value_pattern_data value_pattern_data;
    struct Provider_legacy_accessible_pattern_data legacy_acc_pattern_data;
    IRawElementProviderFragment *focus_prov;
    IRawElementProviderFragmentRoot **embedded_frag_roots;
    int embedded_frag_roots_count;
    int advise_events_added_event_id;
    int advise_events_removed_event_id;
    struct Provider_win_event_handler_data win_event_handler_data;
    HANDLE method_call_event_handle;
    int method_call_event_method_id;
} Provider, Provider2, Provider_child, Provider_child2;
static struct Provider Provider_hwnd, Provider_nc, Provider_proxy, Provider_proxy2, Provider_override;
static void initialize_provider(struct Provider *prov, int prov_opts, HWND hwnd, BOOL initialize_nav_links);
static void set_provider_prop_override(struct Provider *prov, struct Provider_prop_override *override, int count);
static void set_property_override(struct Provider_prop_override *override, int prop_id, VARIANT *val);
static void initialize_provider_tree(BOOL initialize_nav_links);
static void provider_add_child(struct Provider *prov, struct Provider *child);

static const WCHAR *uia_bstr_prop_str = L"uia-string";
static const ULONG uia_i4_prop_val = 0xdeadbeef;
static const ULONG uia_i4_arr_prop_val[] = { 0xfeedbeef, 0xdeadcafe, 0xfefedede };
static const double uia_r8_prop_val = 128.256f;
static const double uia_r8_arr_prop_val[] = { 2.4, 8.16, 32.64 };
static const IRawElementProviderSimple *uia_unk_arr_prop_val[] = { &Provider_child.IRawElementProviderSimple_iface,
                                                                   &Provider_child2.IRawElementProviderSimple_iface };
static SAFEARRAY *create_i4_safearray(void)
{
    SAFEARRAY *sa;
    LONG idx;

    if (!(sa = SafeArrayCreateVector(VT_I4, 0, ARRAY_SIZE(uia_i4_arr_prop_val))))
        return NULL;

    for (idx = 0; idx < ARRAY_SIZE(uia_i4_arr_prop_val); idx++)
        SafeArrayPutElement(sa, &idx, (void *)&uia_i4_arr_prop_val[idx]);

    return sa;
}

static SAFEARRAY *create_r8_safearray(void)
{
    SAFEARRAY *sa;
    LONG idx;

    if (!(sa = SafeArrayCreateVector(VT_R8, 0, ARRAY_SIZE(uia_r8_arr_prop_val))))
        return NULL;

    for (idx = 0; idx < ARRAY_SIZE(uia_r8_arr_prop_val); idx++)
        SafeArrayPutElement(sa, &idx, (void *)&uia_r8_arr_prop_val[idx]);

    return sa;
}

static SAFEARRAY *create_unk_safearray(void)
{
    SAFEARRAY *sa;
    LONG idx;

    if (!(sa = SafeArrayCreateVector(VT_UNKNOWN, 0, ARRAY_SIZE(uia_unk_arr_prop_val))))
        return NULL;

    for (idx = 0; idx < ARRAY_SIZE(uia_unk_arr_prop_val); idx++)
        SafeArrayPutElement(sa, &idx, (void *)uia_unk_arr_prop_val[idx]);

    return sa;
}

enum {
    PROV_GET_PROVIDER_OPTIONS,
    PROV_GET_PATTERN_PROV,
    PROV_GET_PROPERTY_VALUE,
    PROV_GET_HOST_RAW_ELEMENT_PROVIDER,
    FRAG_NAVIGATE,
    FRAG_GET_RUNTIME_ID,
    FRAG_GET_FRAGMENT_ROOT,
    FRAG_GET_BOUNDING_RECT,
    FRAG_GET_EMBEDDED_FRAGMENT_ROOTS,
    FRAG_ROOT_GET_FOCUS,
    HWND_OVERRIDE_GET_OVERRIDE_PROVIDER,
    ADVISE_EVENTS_EVENT_ADDED,
    ADVISE_EVENTS_EVENT_REMOVED,
    WINEVENT_HANDLER_RESPOND_TO_WINEVENT,
};

static const char *prov_method_str[] = {
    "get_ProviderOptions",
    "GetPatternProvider",
    "GetPropertyValue",
    "get_HostRawElementProvider",
    "Navigate",
    "GetRuntimeId",
    "get_FragmentRoot",
    "get_BoundingRectangle",
    "GetEmbeddedFragmentRoots",
    "GetFocus",
    "GetOverrideProviderForHwnd",
    "AdviseEventAdded",
    "AdviseEventRemoved",
    "RespondToWinEvent",
};

static const char *get_prov_method_str(int method)
{
    if (method >= ARRAY_SIZE(prov_method_str))
        return NULL;
    else
        return prov_method_str[method];
}

enum {
    METHOD_OPTIONAL = 0x01,
    METHOD_TODO = 0x02,
};

struct prov_method_sequence {
    struct Provider *prov;
    int method;
    int flags;
};

static int sequence_cnt, sequence_size;
static struct prov_method_sequence *sequence;

/*
 * This sequence of method calls is always used when creating an HUIANODE from
 * an IRawElementProviderSimple.
 */
#define NODE_CREATE_SEQ(prov) \
    { prov , PROV_GET_PROVIDER_OPTIONS }, \
    /* Win10v1507 and below call this. */ \
    { prov , PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */ \
    { prov , PROV_GET_HOST_RAW_ELEMENT_PROVIDER }, \
    { prov , PROV_GET_PROPERTY_VALUE }, \
    { prov , FRAG_NAVIGATE }, /* NavigateDirection_Parent */ \
    { prov , PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL } \

#define NODE_CREATE_SEQ_OPTIONAL(prov) \
    { prov , PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, \
    /* Win10v1507 and below call this. */ \
    { prov , PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */ \
    { prov , PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL }, \
    { prov , PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, \
    { prov , FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */ \
    { prov , PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL } \

/*
 * This sequence of method calls is always used when creating an HUIANODE from
 * an IRawElementProviderSimple that returns an HWND from get_HostRawElementProvider.
 */
#define NODE_CREATE_SEQ2(prov) \
    { prov , PROV_GET_PROVIDER_OPTIONS }, \
    /* Win10v1507 and below call this. */ \
    { prov , PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */ \
    { prov , PROV_GET_HOST_RAW_ELEMENT_PROVIDER }, \
    { prov , FRAG_NAVIGATE }, /* NavigateDirection_Parent */ \
    { prov , PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL } \

#define NODE_CREATE_SEQ2_OPTIONAL(prov) \
    { prov , PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, \
    { prov , PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL }, \
    { prov , FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */ \
    { prov , PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL } \

/*
 * Node creation sequence with the first get_ProviderOptions call being optional.
 * Used for event tests since each Windows version has different amounts of
 * calls to get_ProviderOptions before node creation.
 */
#define NODE_CREATE_SEQ3(prov) \
    { prov , PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, \
    /* Win10v1507 and below call this. */ \
    { prov , PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */ \
    { prov , PROV_GET_HOST_RAW_ELEMENT_PROVIDER }, \
    { prov , PROV_GET_PROPERTY_VALUE }, \
    { prov , FRAG_NAVIGATE }, /* NavigateDirection_Parent */ \
    { prov , PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL } \

static void flush_method_sequence(void)
{
    free(sequence);
    sequence = NULL;
    sequence_cnt = sequence_size = 0;
}

static BOOL method_sequences_enabled = TRUE;
static void add_method_call(struct Provider *prov, int method)
{
    struct prov_method_sequence prov_method = {0};

    if (!method_sequences_enabled)
        return;

    if (sequence_cnt == sequence_size)
    {
        sequence_size = sequence_size ? sequence_size * 2 : 10;
        sequence = realloc(sequence, sequence_size * sizeof(*sequence));
    }

    prov_method.prov = prov;
    prov_method.method = method;
    prov_method.flags = 0;
    sequence[sequence_cnt++] = prov_method;
}

#define ok_method_sequence( exp, context ) \
        ok_method_sequence_( (exp), (context), __FILE__, __LINE__)
static void ok_method_sequence_(const struct prov_method_sequence *expected_list, const char *context,
        const char *file, int line)
{
    const struct prov_method_sequence *expected = expected_list;
    const struct prov_method_sequence *actual;
    unsigned int count = 0;

    assert(method_sequences_enabled);
    add_method_call(NULL, 0);
    actual = sequence;

    if (context)
        winetest_push_context("%s", context);

    while (expected->prov && actual->prov)
    {
	if (expected->prov == actual->prov && expected->method == actual->method)
	{
            if (expected->flags & METHOD_TODO)
                todo_wine ok_(file, line)(1, "%d: expected %s_%s, got %s_%s\n", count, expected->prov->prov_name,
                        get_prov_method_str(expected->method), actual->prov->prov_name, get_prov_method_str(actual->method));
            expected++;
            actual++;
        }
        else if (expected->flags & METHOD_TODO)
        {
            todo_wine ok_(file, line)(0, "%d: expected %s_%s, got %s_%s\n", count, expected->prov->prov_name,
                    get_prov_method_str(expected->method), actual->prov->prov_name, get_prov_method_str(actual->method));
	    expected++;
        }
        else if (expected->flags & METHOD_OPTIONAL)
	    expected++;
        else
        {
            ok_(file, line)(0, "%d: expected %s_%s, got %s_%s\n", count, expected->prov->prov_name,
                    get_prov_method_str(expected->method), actual->prov->prov_name, get_prov_method_str(actual->method));
            expected++;
            actual++;
        }
        count++;
    }

    /* Handle trailing optional/todo_wine methods. */
    while (expected->prov && ((expected->flags & METHOD_OPTIONAL) ||
                ((expected->flags & METHOD_TODO) && !strcmp(winetest_platform, "wine"))))
    {
        if (expected->flags & METHOD_TODO)
            todo_wine ok_(file, line)(0, "%d: expected %s_%s\n", count, expected->prov->prov_name,
                    get_prov_method_str(expected->method));
        count++;
	expected++;
    }

    if (expected->prov || actual->prov)
    {
        if (expected->prov)
            ok_( file, line)(0, "incomplete sequence: expected %s_%s, got nothing\n", expected->prov->prov_name,
                    get_prov_method_str(expected->method));
        else
            ok_( file, line)(0, "incomplete sequence: expected nothing, got %s_%s\n", actual->prov->prov_name,
                    get_prov_method_str(actual->method));
    }

    if (context)
        winetest_pop_context();

    flush_method_sequence();
}

static void check_for_method_call_event(struct Provider *prov, int method)
{
    if (prov->method_call_event_handle && (prov->method_call_event_method_id == method))
        SetEvent(prov->method_call_event_handle);
}

/*
 * Parsing the string returned by UIA_ProviderDescriptionPropertyId is
 * the only way to know what an HUIANODE represents internally. It
 * returns a formatted string which always starts with:
 * "[pid:<process-id>,providerId:0x<hwnd-ptr> "
 * On Windows versions 10v1507 and below, "providerId:" is "hwnd:"
 *
 * This is followed by strings for each provider it represents. These are
 * formatted as:
 * "<prov-type>:<prov-desc> (<origin>)"
 * and are terminated with ";", the final provider has no ";" terminator,
 * instead it has "]".
 *
 * If the given provider is the one used for navigation towards a parent, it has
 * "(parent link)" as a suffix on "<prov-type>".
 *
 * <prov-type> is one of "Annotation", "Main", "Override", "Hwnd", or
 * "Nonclient".
 *
 * <prov-desc> is the string returned from calling GetPropertyValue on the
 * IRawElementProviderSimple being represented with a property ID of
 * UIA_ProviderDescriptionPropertyId.
 *
 * <origin> is the name of the module that the
 * IRawElementProviderSimple comes from. For unmanaged code, it's:
 * "unmanaged:<executable>"
 * and for managed code, it's:
 * "managed:<assembly-qualified-name>"
 *
 * An example:
 * [pid:1500,providerId:0x2F054C Main:Provider (unmanaged:uiautomation_test.exe); Hwnd(parent link):HWND Proxy (unmanaged:uiautomationcore.dll)]
 */
static BOOL get_provider_desc(BSTR prov_desc, const WCHAR *prov_type, WCHAR *out_name)
{
    const WCHAR *str, *str2;

    str = wcsstr(prov_desc, prov_type);
    if (!str)
        return FALSE;

    if (!out_name)
        return TRUE;

    str += wcslen(prov_type);
    str2 = wcschr(str, L'(');
    lstrcpynW(out_name, str, ((str2 - str)));

    return TRUE;
}

#define check_node_provider_desc_todo( prov_desc, prov_type, prov_name, parent_link ) \
        check_node_provider_desc_( (prov_desc), (prov_type), (prov_name), (parent_link), TRUE, __FILE__, __LINE__)
#define check_node_provider_desc( prov_desc, prov_type, prov_name, parent_link ) \
        check_node_provider_desc_( (prov_desc), (prov_type), (prov_name), (parent_link), FALSE, __FILE__, __LINE__)
static void check_node_provider_desc_(BSTR prov_desc, const WCHAR *prov_type, const WCHAR *prov_name,
        BOOL parent_link, BOOL todo, const char *file, int line)
{
    WCHAR buf[2048];

    if (parent_link)
        wsprintfW(buf, L"%s(parent link):", prov_type);
    else
        wsprintfW(buf, L"%s:", prov_type);

    if (!get_provider_desc(prov_desc, buf, buf))
    {
        if (parent_link)
            wsprintfW(buf, L"%s:", prov_type);
        else
            wsprintfW(buf, L"%s(parent link):", prov_type);

        if (!get_provider_desc(prov_desc, buf, buf))
            todo_wine_if(todo) ok_(file, line)(0, "failed to get provider string for %s\n", debugstr_w(prov_type));
        else
        {
            if (parent_link)
                todo_wine_if(todo) ok_(file, line)(0, "expected parent link provider %s\n", debugstr_w(prov_type));
            else
                todo_wine_if(todo) ok_(file, line)(0, "unexpected parent link provider %s\n", debugstr_w(prov_type));
        }

        return;
    }

    if (prov_name)
        ok_(file, line)(!wcscmp(prov_name, buf), "unexpected provider name %s\n", debugstr_w(buf));
}

#define check_node_provider_desc_prefix( prov_desc, pid, prov_id ) \
        check_node_provider_desc_prefix_( (prov_desc), (pid), (prov_id), __FILE__, __LINE__)
static void check_node_provider_desc_prefix_(BSTR prov_desc, DWORD pid, HWND prov_id, const char *file, int line)
{
    const WCHAR *str, *str2;
    WCHAR buf[128];
    DWORD prov_pid;
    HWND prov_hwnd;
    WCHAR *end;

    str = wcsstr(prov_desc, L"pid:");
    str += wcslen(L"pid:");
    str2 = wcschr(str, L',');
    lstrcpynW(buf, str, (str2 - str) + 1);
    prov_pid = wcstoul(buf, &end, 10);
    ok_(file, line)(prov_pid == pid, "Unexpected pid %lu\n", prov_pid);

    str = wcsstr(prov_desc, L"providerId:");
    if (str)
        str += wcslen(L"providerId:");
    else
    {
        str = wcsstr(prov_desc, L"hwnd:");
        str += wcslen(L"hwnd:");
    }
    str2 = wcschr(str, L' ');
    lstrcpynW(buf, str, (str2 - str) + 1);
    prov_hwnd = ULongToHandle(wcstoul(buf, &end, 16));
    ok_(file, line)(prov_hwnd == prov_id, "Unexpected hwnd %p\n", prov_hwnd);
}

/*
 * For node providers that come from an HWND belonging to another process
 * or another thread, the provider is considered 'nested', a node in a node.
 */
static BOOL get_nested_provider_desc(BSTR prov_desc, const WCHAR *prov_type, BOOL parent_link, WCHAR *out_desc)
{
    const WCHAR *str, *str2;
    WCHAR buf[1024];

    if (!parent_link)
        wsprintfW(buf, L"%s:Nested ", prov_type);
    else
        wsprintfW(buf, L"%s(parent link):Nested ", prov_type);
    str = wcsstr(prov_desc, buf);
    /* Check with and without parent-link. */
    if (!str)
        return FALSE;

    if (!out_desc)
        return TRUE;

    str += wcslen(buf);
    str2 = wcschr(str, L']');
    /* We want to include the ']' character, so + 2. */
    lstrcpynW(out_desc, str, ((str2 - str) + 2));

    return TRUE;
}

#define check_runtime_id( exp_runtime_id, exp_size, runtime_id ) \
        check_runtime_id_( (exp_runtime_id), (exp_size), (runtime_id), __FILE__, __LINE__)
static void check_runtime_id_(int *exp_runtime_id, int exp_size, SAFEARRAY *runtime_id, const char *file, int line)
{
    LONG i, idx, lbound, ubound, elems;
    HRESULT hr;
    UINT dims;
    int val;

    dims = SafeArrayGetDim(runtime_id);
    ok_(file, line)(dims == 1, "Unexpected array dims %d\n", dims);

    hr = SafeArrayGetLBound(runtime_id, 1, &lbound);
    ok_(file, line)(hr == S_OK, "Failed to get LBound with hr %#lx\n", hr);

    hr = SafeArrayGetUBound(runtime_id, 1, &ubound);
    ok_(file, line)(hr == S_OK, "Failed to get UBound with hr %#lx\n", hr);

    elems = (ubound - lbound) + 1;
    ok_(file, line)(exp_size == elems, "Unexpected runtime_id array size %#lx\n", elems);

    for (i = 0; i < elems; i++)
    {
        idx = lbound + i;
        hr = SafeArrayGetElement(runtime_id, &idx, &val);
        ok_(file, line)(hr == S_OK, "Failed to get element with hr %#lx\n", hr);
        ok_(file, line)(val == exp_runtime_id[i], "Unexpected runtime_id[%ld] %#x\n", i, val);
    }
}

static inline struct Provider *impl_from_ProviderSimple(IRawElementProviderSimple *iface)
{
    return CONTAINING_RECORD(iface, struct Provider, IRawElementProviderSimple_iface);
}

HRESULT WINAPI ProviderSimple_QueryInterface(IRawElementProviderSimple *iface, REFIID riid, void **ppv)
{
    struct Provider *This = impl_from_ProviderSimple(iface);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IRawElementProviderSimple) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IRawElementProviderFragment))
        *ppv = &This->IRawElementProviderFragment_iface;
    else if (IsEqualIID(riid, &IID_IRawElementProviderFragmentRoot))
        *ppv = &This->IRawElementProviderFragmentRoot_iface;
    else if (IsEqualIID(riid, &IID_IRawElementProviderHwndOverride))
        *ppv = &This->IRawElementProviderHwndOverride_iface;
    else if (IsEqualIID(riid, &IID_IRawElementProviderAdviseEvents))
        *ppv = &This->IRawElementProviderAdviseEvents_iface;
    else if (IsEqualIID(riid, &IID_IValueProvider))
        *ppv = &This->IValueProvider_iface;
    else if (IsEqualIID(riid, &IID_ILegacyIAccessibleProvider))
        *ppv = &This->ILegacyIAccessibleProvider_iface;
    else if (This->win_event_handler_data.is_supported && IsEqualIID(riid, &IID_IProxyProviderWinEventHandler))
        *ppv = &This->IProxyProviderWinEventHandler_iface;
    else
        return E_NOINTERFACE;

    IRawElementProviderSimple_AddRef(iface);
    return S_OK;
}

ULONG WINAPI ProviderSimple_AddRef(IRawElementProviderSimple *iface)
{
    struct Provider *This = impl_from_ProviderSimple(iface);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI ProviderSimple_Release(IRawElementProviderSimple *iface)
{
    struct Provider *This = impl_from_ProviderSimple(iface);
    return InterlockedDecrement(&This->ref);
}

HRESULT WINAPI ProviderSimple_get_ProviderOptions(IRawElementProviderSimple *iface,
        enum ProviderOptions *ret_val)
{
    struct Provider *This = impl_from_ProviderSimple(iface);

    add_method_call(This, PROV_GET_PROVIDER_OPTIONS);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();
    check_for_method_call_event(This, PROV_GET_PROVIDER_OPTIONS);
    PROV_METHOD_TRACE(This, get_ProviderOptions);

    *ret_val = 0;
    if (This->prov_opts)
    {
        *ret_val = This->prov_opts;
        return S_OK;
    }

    return E_NOTIMPL;
}

HRESULT WINAPI ProviderSimple_GetPatternProvider(IRawElementProviderSimple *iface,
        PATTERNID pattern_id, IUnknown **ret_val)
{
    struct Provider *This = impl_from_ProviderSimple(iface);

    add_method_call(This, PROV_GET_PATTERN_PROV);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();
    PROV_METHOD_TRACE2(This, GetPatternProvider, pattern_id, uia_pattern_id_strs);

    *ret_val = NULL;
    switch (pattern_id)
    {
    case UIA_ValuePatternId:
        if (This->value_pattern_data.is_supported)
            *ret_val = (IUnknown *)iface;
        break;

    case UIA_LegacyIAccessiblePatternId:
        if (This->legacy_acc_pattern_data.is_supported)
            *ret_val = (IUnknown *)iface;
        break;

    default:
        break;
    }

    if (*ret_val)
        IUnknown_AddRef(*ret_val);

    check_for_method_call_event(This, PROV_GET_PATTERN_PROV);
    return S_OK;
}

HRESULT WINAPI ProviderSimple_GetPropertyValue(IRawElementProviderSimple *iface,
        PROPERTYID prop_id, VARIANT *ret_val)
{
    struct Provider *This = impl_from_ProviderSimple(iface);

    add_method_call(This, PROV_GET_PROPERTY_VALUE);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();
    check_for_method_call_event(This, PROV_GET_PROPERTY_VALUE);
    PROV_METHOD_TRACE2(This, GetPropertyValue, prop_id, uia_prop_id_strs);

    if (This->prop_override && This->prop_override_count)
    {
        int i;

        for (i = 0; i < This->prop_override_count; i++)
        {
            if (This->prop_override[i].prop_id == prop_id)
            {
                *ret_val = This->prop_override[i].val;
                if (V_VT(ret_val) == VT_UNKNOWN)
                    IUnknown_AddRef(V_UNKNOWN(ret_val));

                return S_OK;
            }
        }
    }

    VariantInit(ret_val);
    switch (prop_id)
    {
    case UIA_NativeWindowHandlePropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_R8;
            V_R8(ret_val) = uia_r8_prop_val;
        }
        else if (!This->ignore_hwnd_prop)
        {
            V_VT(ret_val) = VT_I4;
            V_I4(ret_val) = HandleToULong(This->hwnd);
        }
        break;

    case UIA_ProcessIdPropertyId:
    case UIA_ControlTypePropertyId:
    case UIA_CulturePropertyId:
    case UIA_OrientationPropertyId:
    case UIA_LiveSettingPropertyId:
    case UIA_PositionInSetPropertyId:
    case UIA_SizeOfSetPropertyId:
    case UIA_LevelPropertyId:
    case UIA_LandmarkTypePropertyId:
    case UIA_FillColorPropertyId:
    case UIA_FillTypePropertyId:
    case UIA_VisualEffectsPropertyId:
    case UIA_HeadingLevelPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_R8;
            V_R8(ret_val) = uia_r8_prop_val;
        }
        else
        {
            V_VT(ret_val) = VT_I4;
            V_I4(ret_val) = uia_i4_prop_val;
        }
        break;

    case UIA_RotationPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_I4;
            V_I4(ret_val) = uia_i4_prop_val;
        }
        else
        {
            V_VT(ret_val) = VT_R8;
            V_R8(ret_val) = uia_r8_prop_val;
        }
        break;

    case UIA_LocalizedControlTypePropertyId:
    case UIA_NamePropertyId:
    case UIA_AcceleratorKeyPropertyId:
    case UIA_AccessKeyPropertyId:
    case UIA_AutomationIdPropertyId:
    case UIA_ClassNamePropertyId:
    case UIA_HelpTextPropertyId:
    case UIA_ItemTypePropertyId:
    case UIA_FrameworkIdPropertyId:
    case UIA_ItemStatusPropertyId:
    case UIA_AriaRolePropertyId:
    case UIA_AriaPropertiesPropertyId:
    case UIA_LocalizedLandmarkTypePropertyId:
    case UIA_FullDescriptionPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_I4;
            V_I4(ret_val) = uia_i4_prop_val;
        }
        else
        {
            V_VT(ret_val) = VT_BSTR;
            V_BSTR(ret_val) = SysAllocString(uia_bstr_prop_str);
        }
        break;

    case UIA_HasKeyboardFocusPropertyId:
    case UIA_IsKeyboardFocusablePropertyId:
    case UIA_IsEnabledPropertyId:
    case UIA_IsControlElementPropertyId:
    case UIA_IsContentElementPropertyId:
    case UIA_IsPasswordPropertyId:
    case UIA_IsOffscreenPropertyId:
    case UIA_IsRequiredForFormPropertyId:
    case UIA_IsDataValidForFormPropertyId:
    case UIA_OptimizeForVisualContentPropertyId:
    case UIA_IsPeripheralPropertyId:
    case UIA_IsDialogPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_R8;
            V_R8(ret_val) = uia_r8_prop_val;
        }
        else
        {
            V_VT(ret_val) = VT_BOOL;
            V_BOOL(ret_val) = VARIANT_TRUE;
        }
        break;

    case UIA_AnnotationTypesPropertyId:
    case UIA_OutlineColorPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_ARRAY | VT_R8;
            V_ARRAY(ret_val) = create_r8_safearray();
        }
        else
        {
            V_VT(ret_val) = VT_ARRAY | VT_I4;
            V_ARRAY(ret_val) = create_i4_safearray();
        }
        break;

    case UIA_OutlineThicknessPropertyId:
    case UIA_SizePropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_ARRAY | VT_I4;
            V_ARRAY(ret_val) = create_i4_safearray();
        }
        else
        {
            V_VT(ret_val) = VT_ARRAY | VT_R8;
            V_ARRAY(ret_val) = create_r8_safearray();
        }
        break;

    case UIA_LabeledByPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_I4;
            V_I4(ret_val) = uia_i4_prop_val;
        }
        else
        {
            V_VT(ret_val) = VT_UNKNOWN;
            V_UNKNOWN(ret_val) = (IUnknown *)&Provider_child.IRawElementProviderSimple_iface;
            IUnknown_AddRef(V_UNKNOWN(ret_val));
        }
        break;

    case UIA_AnnotationObjectsPropertyId:
    case UIA_DescribedByPropertyId:
    case UIA_FlowsFromPropertyId:
    case UIA_FlowsToPropertyId:
    case UIA_ControllerForPropertyId:
        if (This->ret_invalid_prop_type)
        {
            V_VT(ret_val) = VT_ARRAY | VT_I4;
            V_ARRAY(ret_val) = create_i4_safearray();
        }
        else
        {
            V_VT(ret_val) = VT_UNKNOWN | VT_ARRAY;
            V_ARRAY(ret_val) = create_unk_safearray();
        }
        break;

    case UIA_ProviderDescriptionPropertyId:
    {
#ifdef __REACTOS__
        WCHAR buf[1024];
#else
        WCHAR buf[1024] = {};
        memset(&buf, 0, sizeof(buf));
#endif

        mbstowcs(buf, This->prov_name, strlen(This->prov_name));
        V_VT(ret_val) = VT_BSTR;
        V_BSTR(ret_val) = SysAllocString(buf);
        break;
    }

    default:
        break;
    }

    return S_OK;
}

HRESULT WINAPI ProviderSimple_get_HostRawElementProvider(IRawElementProviderSimple *iface,
        IRawElementProviderSimple **ret_val)
{
    struct Provider *This = impl_from_ProviderSimple(iface);

    add_method_call(This, PROV_GET_HOST_RAW_ELEMENT_PROVIDER);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();
    check_for_method_call_event(This, PROV_GET_HOST_RAW_ELEMENT_PROVIDER);
    PROV_METHOD_TRACE(This, get_HostRawElementProvider);

    *ret_val = NULL;
    if (This->hwnd)
        return UiaHostProviderFromHwnd(This->hwnd, ret_val);

    return S_OK;
}

IRawElementProviderSimpleVtbl ProviderSimpleVtbl = {
    ProviderSimple_QueryInterface,
    ProviderSimple_AddRef,
    ProviderSimple_Release,
    ProviderSimple_get_ProviderOptions,
    ProviderSimple_GetPatternProvider,
    ProviderSimple_GetPropertyValue,
    ProviderSimple_get_HostRawElementProvider,
};

static inline struct Provider *impl_from_ProviderFragment(IRawElementProviderFragment *iface)
{
    return CONTAINING_RECORD(iface, struct Provider, IRawElementProviderFragment_iface);
}

static HRESULT WINAPI ProviderFragment_QueryInterface(IRawElementProviderFragment *iface, REFIID riid,
        void **ppv)
{
    struct Provider *Provider = impl_from_ProviderFragment(iface);
    return IRawElementProviderSimple_QueryInterface(&Provider->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI ProviderFragment_AddRef(IRawElementProviderFragment *iface)
{
    struct Provider *Provider = impl_from_ProviderFragment(iface);
    return IRawElementProviderSimple_AddRef(&Provider->IRawElementProviderSimple_iface);
}

static ULONG WINAPI ProviderFragment_Release(IRawElementProviderFragment *iface)
{
    struct Provider *Provider = impl_from_ProviderFragment(iface);
    return IRawElementProviderSimple_Release(&Provider->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI ProviderFragment_Navigate(IRawElementProviderFragment *iface,
        enum NavigateDirection direction, IRawElementProviderFragment **ret_val)
{
    struct Provider *This = impl_from_ProviderFragment(iface);

    add_method_call(This, FRAG_NAVIGATE);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();
    PROV_METHOD_TRACE2(This, Navigate, direction, uia_nav_dir_strs);

    *ret_val = NULL;
    switch (direction)
    {
    case NavigateDirection_Parent:
        *ret_val = This->parent;
        break;

    case NavigateDirection_NextSibling:
        *ret_val = This->next_sibling;
        break;

    case NavigateDirection_PreviousSibling:
        *ret_val = This->prev_sibling;
        break;

    case NavigateDirection_FirstChild:
        *ret_val = This->first_child;
        break;

    case NavigateDirection_LastChild:
        *ret_val = This->last_child;
        break;

    default:
        trace("Invalid navigate direction %d\n", direction);
        break;
    }

    if (*ret_val)
        IRawElementProviderFragment_AddRef(*ret_val);

    check_for_method_call_event(This, FRAG_NAVIGATE);
    return S_OK;
}

static HRESULT WINAPI ProviderFragment_GetRuntimeId(IRawElementProviderFragment *iface,
        SAFEARRAY **ret_val)
{
    struct Provider *This = impl_from_ProviderFragment(iface);

    add_method_call(This, FRAG_GET_RUNTIME_ID);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();
    check_for_method_call_event(This, FRAG_GET_RUNTIME_ID);
    PROV_METHOD_TRACE(This, GetRuntimeId);

    *ret_val = NULL;
    if (This->runtime_id[0] || This->runtime_id[1])
    {
        SAFEARRAY *sa;
        LONG idx;

        if (!(sa = SafeArrayCreateVector(VT_I4, 0, ARRAY_SIZE(This->runtime_id))))
            return E_FAIL;

        for (idx = 0; idx < ARRAY_SIZE(This->runtime_id); idx++)
            SafeArrayPutElement(sa, &idx, (void *)&This->runtime_id[idx]);

        *ret_val = sa;
    }

    return S_OK;
}

static HRESULT WINAPI ProviderFragment_get_BoundingRectangle(IRawElementProviderFragment *iface,
        struct UiaRect *ret_val)
{
    struct Provider *This = impl_from_ProviderFragment(iface);

    add_method_call(This, FRAG_GET_BOUNDING_RECT);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();
    check_for_method_call_event(This, FRAG_GET_BOUNDING_RECT);
    PROV_METHOD_TRACE(This, get_BoundingRectangle);

    *ret_val = This->bounds_rect;
    return S_OK;
}

static HRESULT WINAPI ProviderFragment_GetEmbeddedFragmentRoots(IRawElementProviderFragment *iface,
        SAFEARRAY **ret_val)
{
    struct Provider *This = impl_from_ProviderFragment(iface);

    add_method_call(This, FRAG_GET_EMBEDDED_FRAGMENT_ROOTS);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();
    check_for_method_call_event(This, FRAG_GET_EMBEDDED_FRAGMENT_ROOTS);
    PROV_METHOD_TRACE(This, GetEmbeddedFragmentRoots);

    *ret_val = NULL;
    if (This->embedded_frag_roots && This->embedded_frag_roots_count)
    {
        SAFEARRAY *sa;
        LONG idx;

        if (!(sa = SafeArrayCreateVector(VT_UNKNOWN, 0, This->embedded_frag_roots_count)))
            return E_FAIL;

        for (idx = 0; idx < This->embedded_frag_roots_count; idx++)
            SafeArrayPutElement(sa, &idx, (IUnknown *)This->embedded_frag_roots[idx]);

        *ret_val = sa;
    }

    return S_OK;
}

static HRESULT WINAPI ProviderFragment_SetFocus(IRawElementProviderFragment *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderFragment_get_FragmentRoot(IRawElementProviderFragment *iface,
        IRawElementProviderFragmentRoot **ret_val)
{
    struct Provider *This = impl_from_ProviderFragment(iface);

    add_method_call(This, FRAG_GET_FRAGMENT_ROOT);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();
    PROV_METHOD_TRACE(This, get_FragmentRoot);

    *ret_val = NULL;
    if (This->frag_root)
    {
        *ret_val = This->frag_root;
        IRawElementProviderFragmentRoot_AddRef(This->frag_root);
    }

    check_for_method_call_event(This, FRAG_GET_FRAGMENT_ROOT);
    return S_OK;
}

static const IRawElementProviderFragmentVtbl ProviderFragmentVtbl = {
    ProviderFragment_QueryInterface,
    ProviderFragment_AddRef,
    ProviderFragment_Release,
    ProviderFragment_Navigate,
    ProviderFragment_GetRuntimeId,
    ProviderFragment_get_BoundingRectangle,
    ProviderFragment_GetEmbeddedFragmentRoots,
    ProviderFragment_SetFocus,
    ProviderFragment_get_FragmentRoot,
};

static inline struct Provider *impl_from_ProviderFragmentRoot(IRawElementProviderFragmentRoot *iface)
{
    return CONTAINING_RECORD(iface, struct Provider, IRawElementProviderFragmentRoot_iface);
}

static HRESULT WINAPI ProviderFragmentRoot_QueryInterface(IRawElementProviderFragmentRoot *iface, REFIID riid,
        void **ppv)
{
    struct Provider *Provider = impl_from_ProviderFragmentRoot(iface);
    return IRawElementProviderSimple_QueryInterface(&Provider->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI ProviderFragmentRoot_AddRef(IRawElementProviderFragmentRoot *iface)
{
    struct Provider *Provider = impl_from_ProviderFragmentRoot(iface);
    return IRawElementProviderSimple_AddRef(&Provider->IRawElementProviderSimple_iface);
}

static ULONG WINAPI ProviderFragmentRoot_Release(IRawElementProviderFragmentRoot *iface)
{
    struct Provider *Provider = impl_from_ProviderFragmentRoot(iface);
    return IRawElementProviderSimple_Release(&Provider->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI ProviderFragmentRoot_ElementProviderFromPoint(IRawElementProviderFragmentRoot *iface,
        double x, double y, IRawElementProviderFragment **ret_val)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderFragmentRoot_GetFocus(IRawElementProviderFragmentRoot *iface,
        IRawElementProviderFragment **ret_val)
{
    struct Provider *Provider = impl_from_ProviderFragmentRoot(iface);

    add_method_call(Provider, FRAG_ROOT_GET_FOCUS);
    if (Provider->expected_tid)
        ok(Provider->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    Provider->last_call_tid = GetCurrentThreadId();
    PROV_METHOD_TRACE(Provider, GetFocus);

    *ret_val = NULL;
    if (Provider->focus_prov)
    {
        *ret_val = Provider->focus_prov;
        IRawElementProviderFragment_AddRef(*ret_val);
    }

    check_for_method_call_event(Provider, FRAG_ROOT_GET_FOCUS);
    return S_OK;
}

static const IRawElementProviderFragmentRootVtbl ProviderFragmentRootVtbl = {
    ProviderFragmentRoot_QueryInterface,
    ProviderFragmentRoot_AddRef,
    ProviderFragmentRoot_Release,
    ProviderFragmentRoot_ElementProviderFromPoint,
    ProviderFragmentRoot_GetFocus,
};

static inline struct Provider *impl_from_ProviderHwndOverride(IRawElementProviderHwndOverride *iface)
{
    return CONTAINING_RECORD(iface, struct Provider, IRawElementProviderHwndOverride_iface);
}

static HRESULT WINAPI ProviderHwndOverride_QueryInterface(IRawElementProviderHwndOverride *iface, REFIID riid,
        void **ppv)
{
    struct Provider *Provider = impl_from_ProviderHwndOverride(iface);
    return IRawElementProviderSimple_QueryInterface(&Provider->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI ProviderHwndOverride_AddRef(IRawElementProviderHwndOverride *iface)
{
    struct Provider *Provider = impl_from_ProviderHwndOverride(iface);
    return IRawElementProviderSimple_AddRef(&Provider->IRawElementProviderSimple_iface);
}

static ULONG WINAPI ProviderHwndOverride_Release(IRawElementProviderHwndOverride *iface)
{
    struct Provider *Provider = impl_from_ProviderHwndOverride(iface);
    return IRawElementProviderSimple_Release(&Provider->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI ProviderHwndOverride_GetOverrideProviderForHwnd(IRawElementProviderHwndOverride *iface,
        HWND hwnd, IRawElementProviderSimple **ret_val)
{
    struct Provider *This = impl_from_ProviderHwndOverride(iface);

    add_method_call(This, HWND_OVERRIDE_GET_OVERRIDE_PROVIDER);
    check_for_method_call_event(This, HWND_OVERRIDE_GET_OVERRIDE_PROVIDER);
    PROV_METHOD_TRACE(This, GetOverrideProviderForHwnd);

    *ret_val = NULL;
    if (This->override_hwnd == hwnd)
    {
        return IRawElementProviderSimple_QueryInterface(&Provider_override.IRawElementProviderSimple_iface,
                &IID_IRawElementProviderSimple, (void **)ret_val);
    }

    return S_OK;
}

static const IRawElementProviderHwndOverrideVtbl ProviderHwndOverrideVtbl = {
    ProviderHwndOverride_QueryInterface,
    ProviderHwndOverride_AddRef,
    ProviderHwndOverride_Release,
    ProviderHwndOverride_GetOverrideProviderForHwnd,
};

static inline struct Provider *impl_from_ProviderAdviseEvents(IRawElementProviderAdviseEvents *iface)
{
    return CONTAINING_RECORD(iface, struct Provider, IRawElementProviderAdviseEvents_iface);
}

static HRESULT WINAPI ProviderAdviseEvents_QueryInterface(IRawElementProviderAdviseEvents *iface, REFIID riid,
        void **ppv)
{
    struct Provider *Provider = impl_from_ProviderAdviseEvents(iface);
    return IRawElementProviderSimple_QueryInterface(&Provider->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI ProviderAdviseEvents_AddRef(IRawElementProviderAdviseEvents *iface)
{
    struct Provider *Provider = impl_from_ProviderAdviseEvents(iface);
    return IRawElementProviderSimple_AddRef(&Provider->IRawElementProviderSimple_iface);
}

static ULONG WINAPI ProviderAdviseEvents_Release(IRawElementProviderAdviseEvents *iface)
{
    struct Provider *Provider = impl_from_ProviderAdviseEvents(iface);
    return IRawElementProviderSimple_Release(&Provider->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI ProviderAdviseEvents_AdviseEventAdded(IRawElementProviderAdviseEvents *iface,
        EVENTID event_id, SAFEARRAY *prop_ids)
{
    struct Provider *This = impl_from_ProviderAdviseEvents(iface);

    add_method_call(This, ADVISE_EVENTS_EVENT_ADDED);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();
    This->advise_events_added_event_id = event_id;
    PROV_METHOD_TRACE2(This, AdviseEventAdded, event_id, uia_event_id_strs);

    check_for_method_call_event(This, ADVISE_EVENTS_EVENT_ADDED);
    return S_OK;
}

static HRESULT WINAPI ProviderAdviseEvents_AdviseEventRemoved(IRawElementProviderAdviseEvents *iface,
        EVENTID event_id, SAFEARRAY *prop_ids)
{
    struct Provider *This = impl_from_ProviderAdviseEvents(iface);

    add_method_call(This, ADVISE_EVENTS_EVENT_REMOVED);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();
    This->advise_events_removed_event_id = event_id;
    PROV_METHOD_TRACE2(This, AdviseEventRemoved, event_id, uia_event_id_strs);

    check_for_method_call_event(This, ADVISE_EVENTS_EVENT_REMOVED);
    return S_OK;
}

static const IRawElementProviderAdviseEventsVtbl ProviderAdviseEventsVtbl = {
    ProviderAdviseEvents_QueryInterface,
    ProviderAdviseEvents_AddRef,
    ProviderAdviseEvents_Release,
    ProviderAdviseEvents_AdviseEventAdded,
    ProviderAdviseEvents_AdviseEventRemoved,
};

static inline struct Provider *impl_from_ProviderWinEventHandler(IProxyProviderWinEventHandler *iface)
{
    return CONTAINING_RECORD(iface, struct Provider, IProxyProviderWinEventHandler_iface);
}

static HRESULT WINAPI ProviderWinEventHandler_QueryInterface(IProxyProviderWinEventHandler *iface, REFIID riid,
        void **ppv)
{
    struct Provider *Provider = impl_from_ProviderWinEventHandler(iface);
    return IRawElementProviderSimple_QueryInterface(&Provider->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI ProviderWinEventHandler_AddRef(IProxyProviderWinEventHandler *iface)
{
    struct Provider *Provider = impl_from_ProviderWinEventHandler(iface);
    return IRawElementProviderSimple_AddRef(&Provider->IRawElementProviderSimple_iface);
}

static ULONG WINAPI ProviderWinEventHandler_Release(IProxyProviderWinEventHandler *iface)
{
    struct Provider *Provider = impl_from_ProviderWinEventHandler(iface);
    return IRawElementProviderSimple_Release(&Provider->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI ProviderWinEventHandler_RespondToWinEvent(IProxyProviderWinEventHandler *iface,
        DWORD event_id, HWND hwnd, LONG obj_id, LONG child_id, IProxyProviderWinEventSink *event_sink)
{
    struct Provider *This = impl_from_ProviderWinEventHandler(iface);
    struct Provider_win_event_handler_data *data;
    HRESULT hr;

    PROV_METHOD_TRACE(This, RespondToWinEvent);
    data = &This->win_event_handler_data;
    if ((data->exp_win_event_id != event_id) || (data->exp_win_event_hwnd != hwnd) || (data->exp_win_event_obj_id != obj_id) ||
            (data->exp_win_event_child_id != child_id))
        return S_OK;

    add_method_call(This, WINEVENT_HANDLER_RESPOND_TO_WINEVENT);
    if (This->expected_tid)
        ok(This->expected_tid == GetCurrentThreadId(), "Unexpected tid %ld\n", GetCurrentThreadId());
    This->last_call_tid = GetCurrentThreadId();

    if (data->responder_prov)
    {
        /*
         * The IProxyProviderWinEventSink interface uses the free threaded
         * marshaler, so no proxy will be created in-process.
         */
        check_interface_marshal_proxy_creation((IUnknown *)event_sink, &IID_IProxyProviderWinEventSink, FALSE);
        hr = IProxyProviderWinEventSink_AddAutomationEvent(event_sink, data->responder_prov, data->responder_event);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }

    check_for_method_call_event(This, WINEVENT_HANDLER_RESPOND_TO_WINEVENT);
    return S_OK;
}

static const IProxyProviderWinEventHandlerVtbl ProviderWinEventHandlerVtbl = {
    ProviderWinEventHandler_QueryInterface,
    ProviderWinEventHandler_AddRef,
    ProviderWinEventHandler_Release,
    ProviderWinEventHandler_RespondToWinEvent,
};

static inline struct Provider *impl_from_ProviderValuePattern(IValueProvider *iface)
{
    return CONTAINING_RECORD(iface, struct Provider, IValueProvider_iface);
}

static HRESULT WINAPI ProviderValuePattern_QueryInterface(IValueProvider *iface, REFIID riid,
        void **ppv)
{
    struct Provider *Provider = impl_from_ProviderValuePattern(iface);
    return IRawElementProviderSimple_QueryInterface(&Provider->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI ProviderValuePattern_AddRef(IValueProvider *iface)
{
    struct Provider *Provider = impl_from_ProviderValuePattern(iface);
    return IRawElementProviderSimple_AddRef(&Provider->IRawElementProviderSimple_iface);
}

static ULONG WINAPI ProviderValuePattern_Release(IValueProvider *iface)
{
    struct Provider *Provider = impl_from_ProviderValuePattern(iface);
    return IRawElementProviderSimple_Release(&Provider->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI ProviderValuePattern_SetValue(IValueProvider *iface, LPCWSTR val)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderValuePattern_get_Value(IValueProvider *iface, BSTR *ret_val)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderValuePattern_get_IsReadOnly(IValueProvider *iface, BOOL *ret_val)
{
    struct Provider *Provider = impl_from_ProviderValuePattern(iface);

    *ret_val = Provider->value_pattern_data.is_read_only;

    return S_OK;
}

static const IValueProviderVtbl ProviderValuePatternVtbl = {
    ProviderValuePattern_QueryInterface,
    ProviderValuePattern_AddRef,
    ProviderValuePattern_Release,
    ProviderValuePattern_SetValue,
    ProviderValuePattern_get_Value,
    ProviderValuePattern_get_IsReadOnly,
};

static inline struct Provider *impl_from_ProviderLegacyIAccessiblePattern(ILegacyIAccessibleProvider *iface)
{
    return CONTAINING_RECORD(iface, struct Provider, ILegacyIAccessibleProvider_iface);
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_QueryInterface(ILegacyIAccessibleProvider *iface, REFIID riid,
        void **ppv)
{
    struct Provider *Provider = impl_from_ProviderLegacyIAccessiblePattern(iface);
    return IRawElementProviderSimple_QueryInterface(&Provider->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI ProviderLegacyIAccessiblePattern_AddRef(ILegacyIAccessibleProvider *iface)
{
    struct Provider *Provider = impl_from_ProviderLegacyIAccessiblePattern(iface);
    return IRawElementProviderSimple_AddRef(&Provider->IRawElementProviderSimple_iface);
}

static ULONG WINAPI ProviderLegacyIAccessiblePattern_Release(ILegacyIAccessibleProvider *iface)
{
    struct Provider *Provider = impl_from_ProviderLegacyIAccessiblePattern(iface);
    return IRawElementProviderSimple_Release(&Provider->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_Select(ILegacyIAccessibleProvider *iface, LONG select_flags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_DoDefaultAction(ILegacyIAccessibleProvider *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_SetValue(ILegacyIAccessibleProvider *iface, LPCWSTR val)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_GetIAccessible(ILegacyIAccessibleProvider *iface,
        IAccessible **out_acc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_get_ChildId(ILegacyIAccessibleProvider *iface, int *out_cid)
{
    struct Provider *Provider = impl_from_ProviderLegacyIAccessiblePattern(iface);

    *out_cid = Provider->legacy_acc_pattern_data.child_id;
    return S_OK;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_get_Name(ILegacyIAccessibleProvider *iface, BSTR *out_name)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_get_Value(ILegacyIAccessibleProvider *iface, BSTR *out_value)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_get_Description(ILegacyIAccessibleProvider *iface,
        BSTR *out_description)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_get_Role(ILegacyIAccessibleProvider *iface, DWORD *out_role)
{
    struct Provider *Provider = impl_from_ProviderLegacyIAccessiblePattern(iface);

    *out_role = Provider->legacy_acc_pattern_data.role;
    return S_OK;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_get_State(ILegacyIAccessibleProvider *iface, DWORD *out_state)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_get_Help(ILegacyIAccessibleProvider *iface, BSTR *out_help)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_get_KeyboardShortcut(ILegacyIAccessibleProvider *iface,
        BSTR *out_kbd_shortcut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_GetSelection(ILegacyIAccessibleProvider *iface,
        SAFEARRAY **out_selected)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProviderLegacyIAccessiblePattern_get_DefaultAction(ILegacyIAccessibleProvider *iface,
        BSTR *out_default_action)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const ILegacyIAccessibleProviderVtbl ProviderLegacyIAccessiblePatternVtbl = {
    ProviderLegacyIAccessiblePattern_QueryInterface,
    ProviderLegacyIAccessiblePattern_AddRef,
    ProviderLegacyIAccessiblePattern_Release,
    ProviderLegacyIAccessiblePattern_Select,
    ProviderLegacyIAccessiblePattern_DoDefaultAction,
    ProviderLegacyIAccessiblePattern_SetValue,
    ProviderLegacyIAccessiblePattern_GetIAccessible,
    ProviderLegacyIAccessiblePattern_get_ChildId,
    ProviderLegacyIAccessiblePattern_get_Name,
    ProviderLegacyIAccessiblePattern_get_Value,
    ProviderLegacyIAccessiblePattern_get_Description,
    ProviderLegacyIAccessiblePattern_get_Role,
    ProviderLegacyIAccessiblePattern_get_State,
    ProviderLegacyIAccessiblePattern_get_Help,
    ProviderLegacyIAccessiblePattern_get_KeyboardShortcut,
    ProviderLegacyIAccessiblePattern_GetSelection,
    ProviderLegacyIAccessiblePattern_get_DefaultAction,
};

static struct Provider Provider =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    { &ProviderHwndOverrideVtbl },
    { &ProviderAdviseEventsVtbl },
    { &ProviderWinEventHandlerVtbl },
    { &ProviderValuePatternVtbl },
    { &ProviderLegacyIAccessiblePatternVtbl },
    1,
    "Provider",
    NULL, NULL,
    NULL, NULL,
    &Provider_child.IRawElementProviderFragment_iface, &Provider_child2.IRawElementProviderFragment_iface,
    0, 0, 0,
};

static struct Provider Provider2 =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    { &ProviderHwndOverrideVtbl },
    { &ProviderAdviseEventsVtbl },
    { &ProviderWinEventHandlerVtbl },
    { &ProviderValuePatternVtbl },
    { &ProviderLegacyIAccessiblePatternVtbl },
    1,
    "Provider2",
    NULL, NULL,
    NULL, NULL,
    NULL, NULL,
    0, 0, 0,
};

static struct Provider Provider_child =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    { &ProviderHwndOverrideVtbl },
    { &ProviderAdviseEventsVtbl },
    { &ProviderWinEventHandlerVtbl },
    { &ProviderValuePatternVtbl },
    { &ProviderLegacyIAccessiblePatternVtbl },
    1,
    "Provider_child",
    &Provider.IRawElementProviderFragment_iface, &Provider.IRawElementProviderFragmentRoot_iface,
    NULL, &Provider_child2.IRawElementProviderFragment_iface,
    NULL, NULL,
    ProviderOptions_ServerSideProvider, 0, 0,
};

static struct Provider Provider_child2 =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    { &ProviderHwndOverrideVtbl },
    { &ProviderAdviseEventsVtbl },
    { &ProviderWinEventHandlerVtbl },
    { &ProviderValuePatternVtbl },
    { &ProviderLegacyIAccessiblePatternVtbl },
    1,
    "Provider_child2",
    &Provider.IRawElementProviderFragment_iface, &Provider.IRawElementProviderFragmentRoot_iface,
    &Provider_child.IRawElementProviderFragment_iface, NULL,
    NULL, NULL,
    ProviderOptions_ServerSideProvider, 0, 0,
};

static struct Provider Provider_hwnd =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    { &ProviderHwndOverrideVtbl },
    { &ProviderAdviseEventsVtbl },
    { &ProviderWinEventHandlerVtbl },
    { &ProviderValuePatternVtbl },
    { &ProviderLegacyIAccessiblePatternVtbl },
    1,
    "Provider_hwnd",
    NULL, NULL,
    NULL, NULL,
    NULL, NULL,
    ProviderOptions_ClientSideProvider, 0, 0,
};

static struct Provider Provider_nc =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    { &ProviderHwndOverrideVtbl },
    { &ProviderAdviseEventsVtbl },
    { &ProviderWinEventHandlerVtbl },
    { &ProviderValuePatternVtbl },
    { &ProviderLegacyIAccessiblePatternVtbl },
    1,
    "Provider_nc",
    NULL, NULL,
    NULL, NULL,
    NULL, NULL,
    ProviderOptions_ClientSideProvider | ProviderOptions_NonClientAreaProvider,
    0, 0,
};

static struct Provider Provider_proxy =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    { &ProviderHwndOverrideVtbl },
    { &ProviderAdviseEventsVtbl },
    { &ProviderWinEventHandlerVtbl },
    { &ProviderValuePatternVtbl },
    { &ProviderLegacyIAccessiblePatternVtbl },
    1,
    "Provider_proxy",
    NULL, NULL,
    NULL, NULL,
    NULL, NULL,
    ProviderOptions_ClientSideProvider,
    0, 0,
};

static struct Provider Provider_proxy2 =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    { &ProviderHwndOverrideVtbl },
    { &ProviderAdviseEventsVtbl },
    { &ProviderWinEventHandlerVtbl },
    { &ProviderValuePatternVtbl },
    { &ProviderLegacyIAccessiblePatternVtbl },
    1,
    "Provider_proxy2",
    NULL, NULL,
    NULL, NULL,
    NULL, NULL,
    ProviderOptions_ClientSideProvider,
    0, 0,
};

static struct Provider Provider_override =
{
    { &ProviderSimpleVtbl },
    { &ProviderFragmentVtbl },
    { &ProviderFragmentRootVtbl },
    { &ProviderHwndOverrideVtbl },
    { &ProviderAdviseEventsVtbl },
    { &ProviderWinEventHandlerVtbl },
    { &ProviderValuePatternVtbl },
    { &ProviderLegacyIAccessiblePatternVtbl },
    1,
    "Provider_override",
    NULL, NULL,
    NULL, NULL,
    NULL, NULL,
    ProviderOptions_ServerSideProvider | ProviderOptions_OverrideProvider,
    0, 0,
};

#define DEFINE_PROVIDER(name) \
    static struct Provider Provider_ ## name = \
    { \
        { &ProviderSimpleVtbl }, \
        { &ProviderFragmentVtbl }, \
        { &ProviderFragmentRootVtbl }, \
        { &ProviderHwndOverrideVtbl }, \
        { &ProviderAdviseEventsVtbl }, \
        { &ProviderWinEventHandlerVtbl }, \
        { &ProviderValuePatternVtbl }, \
        { &ProviderLegacyIAccessiblePatternVtbl }, \
        1, \
        "Provider_" # name "", \
        NULL, NULL, \
        NULL, NULL, \
        NULL, NULL, \
        ProviderOptions_ServerSideProvider, 0, 0 \
    }

DEFINE_PROVIDER(hwnd_child);
DEFINE_PROVIDER(hwnd_child2);
DEFINE_PROVIDER(nc_child);
DEFINE_PROVIDER(nc_child2);
DEFINE_PROVIDER(child_child);
DEFINE_PROVIDER(child_child2);
DEFINE_PROVIDER(child2_child);
DEFINE_PROVIDER(child2_child_child);
DEFINE_PROVIDER(hwnd2);
DEFINE_PROVIDER(nc2);
DEFINE_PROVIDER(hwnd3);
DEFINE_PROVIDER(nc3);

static IAccessible *acc_client;
static IRawElementProviderSimple *prov_root;
static LRESULT WINAPI test_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_GETOBJECT:
        if (lParam == (DWORD)OBJID_CLIENT)
        {
            CHECK_EXPECT(winproc_GETOBJECT_CLIENT);
            if (acc_client)
                return LresultFromObject(&IID_IAccessible, wParam, (IUnknown *)acc_client);

            break;
        }
        else if (lParam == UiaRootObjectId)
        {
            CHECK_EXPECT(winproc_GETOBJECT_UiaRoot);
            if (prov_root)
                return UiaReturnRawElementProvider(hwnd, wParam, lParam, prov_root);

            break;
        }

        break;

    default:
        break;
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static IRawElementProviderSimple *child_win_prov_root;
static LRESULT WINAPI child_test_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_GETOBJECT:
        if (lParam == UiaRootObjectId)
        {
            CHECK_EXPECT(child_winproc_GETOBJECT_UiaRoot);
            if (child_win_prov_root)
                return UiaReturnRawElementProvider(hwnd, wParam, lParam, child_win_prov_root);

            break;
        }

        break;

    default:
        break;
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static void test_UiaHostProviderFromHwnd(void)
{
    IRawElementProviderSimple *p, *p2;
    enum ProviderOptions prov_opt;
    WNDCLASSA cls;
    HRESULT hr;
    HWND hwnd;
    VARIANT v;
    int i;

    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "HostProviderFromHwnd class";

    RegisterClassA(&cls);

    hwnd = CreateWindowExA(0, "HostProviderFromHwnd class", "Test window 1",
            WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
            0, 0, 100, 100, GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);
    ok(hwnd != NULL, "Failed to create a test window.\n");

    p = (void *)0xdeadbeef;
    hr = UiaHostProviderFromHwnd(NULL, &p);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(p == NULL, "Unexpected instance.\n");

    hr = UiaHostProviderFromHwnd(hwnd, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    p = NULL;
    hr = UiaHostProviderFromHwnd(hwnd, &p);
    ok(hr == S_OK, "Failed to get host provider, hr %#lx.\n", hr);

    p2 = NULL;
    hr = UiaHostProviderFromHwnd(hwnd, &p2);
    ok(hr == S_OK, "Failed to get host provider, hr %#lx.\n", hr);
    ok(p != p2, "Unexpected instance.\n");
    IRawElementProviderSimple_Release(p2);

    hr = IRawElementProviderSimple_get_HostRawElementProvider(p, &p2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(p2 == NULL, "Unexpected instance.\n");

    hr = IRawElementProviderSimple_GetPropertyValue(p, UIA_NativeWindowHandlePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == HandleToUlong(hwnd), "V_I4(&v) = %#lx, expected %#lx\n", V_I4(&v), HandleToUlong(hwnd));

    hr = IRawElementProviderSimple_GetPropertyValue(p, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BSTR, "V_VT(&v) = %d\n", V_VT(&v));
    VariantClear(&v);

    /* No patterns are implemented on the HWND Host provider. */
    for (i = UIA_InvokePatternId; i < (UIA_CustomNavigationPatternId + 1); i++)
    {
        IUnknown *unk;

        unk = (void *)0xdeadbeef;
        hr = IRawElementProviderSimple_GetPatternProvider(p, i, &unk);
        ok(hr == S_OK, "Unexpected hr %#lx, %d.\n", hr, i);
        ok(!unk, "Pattern %d returned %p\n", i, unk);
    }

    hr = IRawElementProviderSimple_get_ProviderOptions(p, &prov_opt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok((prov_opt == ProviderOptions_ServerSideProvider) ||
            broken(prov_opt == ProviderOptions_ClientSideProvider), /* Windows < 10 1507 */
            "Unexpected provider options %#x\n", prov_opt);

    /* Test behavior post Window destruction. */
    DestroyWindow(hwnd);

    hr = IRawElementProviderSimple_GetPropertyValue(p, UIA_NativeWindowHandlePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == HandleToUlong(hwnd), "V_I4(&v) = %#lx, expected %#lx\n", V_I4(&v), HandleToUlong(hwnd));

    hr = IRawElementProviderSimple_GetPropertyValue(p, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BSTR, "V_VT(&v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hr = IRawElementProviderSimple_get_ProviderOptions(p, &prov_opt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok((prov_opt == ProviderOptions_ServerSideProvider) ||
            broken(prov_opt == ProviderOptions_ClientSideProvider), /* Windows < 10 1507 */
            "Unexpected provider options %#x\n", prov_opt);

    IRawElementProviderSimple_Release(p);

    UnregisterClassA("HostProviderFromHwnd class", NULL);
}

static DWORD WINAPI uia_reserved_val_iface_marshal_thread(LPVOID param)
{
    IStream **stream = param;
    IUnknown *unk_ns, *unk_ns2, *unk_ma, *unk_ma2;
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoGetInterfaceAndReleaseStream(stream[0], &IID_IUnknown, (void **)&unk_ns);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoGetInterfaceAndReleaseStream(stream[1], &IID_IUnknown, (void **)&unk_ma);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = UiaGetReservedNotSupportedValue(&unk_ns2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = UiaGetReservedMixedAttributeValue(&unk_ma2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(unk_ns2 == unk_ns, "UiaGetReservedNotSupported pointer mismatch, unk_ns2 %p, unk_ns %p\n", unk_ns2, unk_ns);
    ok(unk_ma2 == unk_ma, "UiaGetReservedMixedAttribute pointer mismatch, unk_ma2 %p, unk_ma %p\n", unk_ma2, unk_ma);

    CoUninitialize();

    return 0;
}

static void test_uia_reserved_value_ifaces(void)
{
    IUnknown *unk_ns, *unk_ns2, *unk_ma, *unk_ma2;
    IStream *stream[2];
    IMarshal *marshal;
    HANDLE thread;
    ULONG refcnt;
    HRESULT hr;

    /* ReservedNotSupportedValue. */
    hr = UiaGetReservedNotSupportedValue(NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaGetReservedNotSupportedValue(&unk_ns);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk_ns != NULL, "UiaGetReservedNotSupportedValue returned NULL interface.\n");

    refcnt = IUnknown_AddRef(unk_ns);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    refcnt = IUnknown_AddRef(unk_ns);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    refcnt = IUnknown_Release(unk_ns);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    hr = UiaGetReservedNotSupportedValue(&unk_ns2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk_ns2 != NULL, "UiaGetReservedNotSupportedValue returned NULL interface.");
    ok(unk_ns2 == unk_ns, "UiaGetReservedNotSupported pointer mismatch, unk_ns2 %p, unk_ns %p\n", unk_ns2, unk_ns);

    marshal = NULL;
    hr = IUnknown_QueryInterface(unk_ns, &IID_IMarshal, (void **)&marshal);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(marshal != NULL, "Failed to get IMarshal interface.\n");

    refcnt = IMarshal_AddRef(marshal);
    ok(refcnt == 2, "Expected refcnt %d, got %ld\n", 2, refcnt);

    refcnt = IMarshal_Release(marshal);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    refcnt = IMarshal_Release(marshal);
    ok(refcnt == 0, "Expected refcnt %d, got %ld\n", 0, refcnt);

    /* ReservedMixedAttributeValue. */
    hr = UiaGetReservedMixedAttributeValue(NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaGetReservedMixedAttributeValue(&unk_ma);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk_ma != NULL, "UiaGetReservedMixedAttributeValue returned NULL interface.");

    refcnt = IUnknown_AddRef(unk_ma);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    refcnt = IUnknown_AddRef(unk_ma);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    refcnt = IUnknown_Release(unk_ma);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    hr = UiaGetReservedMixedAttributeValue(&unk_ma2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk_ma2 != NULL, "UiaGetReservedMixedAttributeValue returned NULL interface.");
    ok(unk_ma2 == unk_ma, "UiaGetReservedMixedAttribute pointer mismatch, unk_ma2 %p, unk_ma %p\n", unk_ma2, unk_ma);

    marshal = NULL;
    hr = IUnknown_QueryInterface(unk_ma, &IID_IMarshal, (void **)&marshal);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(marshal != NULL, "Failed to get IMarshal interface.\n");

    refcnt = IMarshal_AddRef(marshal);
    ok(refcnt == 2, "Expected refcnt %d, got %ld\n", 2, refcnt);

    refcnt = IMarshal_Release(marshal);
    ok(refcnt == 1, "Expected refcnt %d, got %ld\n", 1, refcnt);

    refcnt = IMarshal_Release(marshal);
    ok(refcnt == 0, "Expected refcnt %d, got %ld\n", 0, refcnt);

    /* Test cross-thread marshaling behavior. */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, unk_ns, &stream[0]);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, unk_ma, &stream[1]);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    thread = CreateThread(NULL, 0, uia_reserved_val_iface_marshal_thread, (void *)stream, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;
        while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);

    CoUninitialize();
}

static struct ProxyEventSink
{
    IProxyProviderWinEventSink IProxyProviderWinEventSink_iface;
    LONG ref;

    IRawElementProviderSimple *event_elprov;
    int event_id;

    IRawElementProviderSimple *prop_change_elprov;
    int prop_change_prop_id;
    VARIANT prop_change_value;

    IRawElementProviderSimple *structure_change_elprov;
    int structure_change_type;
    SAFEARRAY *structure_change_rt_id;
} ProxyEventSink;

static void proxy_event_sink_clear(void)
{
    if (ProxyEventSink.event_elprov)
        IRawElementProviderSimple_Release(ProxyEventSink.event_elprov);
    ProxyEventSink.event_elprov = NULL;
    ProxyEventSink.event_id = 0;

    if (ProxyEventSink.prop_change_elprov)
        IRawElementProviderSimple_Release(ProxyEventSink.prop_change_elprov);
    ProxyEventSink.prop_change_elprov = NULL;
    ProxyEventSink.prop_change_prop_id = 0;
    VariantClear(&ProxyEventSink.prop_change_value);

    if (ProxyEventSink.structure_change_elprov)
        IRawElementProviderSimple_Release(ProxyEventSink.structure_change_elprov);
    ProxyEventSink.structure_change_elprov = NULL;
    ProxyEventSink.structure_change_type = 0;
    SafeArrayDestroy(ProxyEventSink.structure_change_rt_id);
}

static inline struct ProxyEventSink *impl_from_ProxyEventSink(IProxyProviderWinEventSink *iface)
{
    return CONTAINING_RECORD(iface, struct ProxyEventSink, IProxyProviderWinEventSink_iface);
}

static HRESULT WINAPI ProxyEventSink_QueryInterface(IProxyProviderWinEventSink *iface, REFIID riid, void **obj)
{
    *obj = NULL;
    if (IsEqualIID(riid, &IID_IProxyProviderWinEventSink) || IsEqualIID(riid, &IID_IUnknown))
        *obj = iface;
    else
        return E_NOINTERFACE;

    IProxyProviderWinEventSink_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI ProxyEventSink_AddRef(IProxyProviderWinEventSink *iface)
{
    struct ProxyEventSink *This = impl_from_ProxyEventSink(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ProxyEventSink_Release(IProxyProviderWinEventSink *iface)
{
    struct ProxyEventSink *This = impl_from_ProxyEventSink(iface);
    return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI ProxyEventSink_AddAutomationPropertyChangedEvent(IProxyProviderWinEventSink *iface,
        IRawElementProviderSimple *elprov, PROPERTYID prop_id, VARIANT new_value)
{
    struct ProxyEventSink *This = impl_from_ProxyEventSink(iface);

    CHECK_EXPECT(ProxyEventSink_AddAutomationPropertyChangedEvent);
    This->prop_change_elprov = elprov;
    if (elprov)
        IRawElementProviderSimple_AddRef(elprov);

    This->prop_change_prop_id = prop_id;
    VariantCopy(&This->prop_change_value, &new_value);

    return S_OK;
}

static HRESULT WINAPI ProxyEventSink_AddAutomationEvent(IProxyProviderWinEventSink *iface,
        IRawElementProviderSimple *elprov, EVENTID event_id)
{
    struct ProxyEventSink *This = impl_from_ProxyEventSink(iface);

    CHECK_EXPECT(ProxyEventSink_AddAutomationEvent);
    This->event_elprov = elprov;
    if (elprov)
        IRawElementProviderSimple_AddRef(elprov);
    This->event_id = event_id;

    return S_OK;
}

static HRESULT WINAPI ProxyEventSink_AddStructureChangedEvent(IProxyProviderWinEventSink *iface,
        IRawElementProviderSimple *elprov, enum StructureChangeType structure_change_type, SAFEARRAY *runtime_id)
{
    struct ProxyEventSink *This = impl_from_ProxyEventSink(iface);
    HRESULT hr;

    CHECK_EXPECT(ProxyEventSink_AddStructureChangedEvent);
    This->structure_change_elprov = elprov;
    if (elprov)
        IRawElementProviderSimple_AddRef(elprov);
    This->structure_change_type = structure_change_type;

    hr = SafeArrayCopy(runtime_id, &This->structure_change_rt_id);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    return S_OK;
}

static const IProxyProviderWinEventSinkVtbl ProxyEventSinkVtbl = {
    ProxyEventSink_QueryInterface,
    ProxyEventSink_AddRef,
    ProxyEventSink_Release,
    ProxyEventSink_AddAutomationPropertyChangedEvent,
    ProxyEventSink_AddAutomationEvent,
    ProxyEventSink_AddStructureChangedEvent,
};

static struct ProxyEventSink ProxyEventSink =
{
    { &ProxyEventSinkVtbl },
    1,
};

static void set_accessible_props(struct Accessible *acc, INT role, INT state,
        LONG child_count, LPCWSTR name, LONG left, LONG top, LONG width, LONG height);
static void set_accessible_ia2_props(struct Accessible *acc, BOOL enable_ia2, LONG unique_id);
static void test_uia_prov_from_acc_winevent_handler(HWND hwnd)
{
    IProxyProviderWinEventHandler *handler;
    IRawElementProviderSimple *elprov;
    HRESULT hr;

    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, 0, 0, L"acc_name", 0, 0, 0, 0);
    Accessible.ow_hwnd = hwnd;
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IProxyProviderWinEventHandler, (void **)&handler);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!handler, "Handler == NULL\n");

    /* EVENT_SYSTEM_ALERT */
    SET_EXPECT(ProxyEventSink_AddAutomationEvent);
    hr = IProxyProviderWinEventHandler_RespondToWinEvent(handler, EVENT_SYSTEM_ALERT, hwnd, OBJID_CLIENT, CHILDID_SELF,
            &ProxyEventSink.IProxyProviderWinEventSink_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(ProxyEventSink_AddAutomationEvent);
    ok(ProxyEventSink.event_id == UIA_SystemAlertEventId, "Unexpected event_id %d\n", ProxyEventSink.event_id);
    ok(ProxyEventSink.event_elprov == elprov, "Unexpected event_elprov %p\n", ProxyEventSink.event_elprov);
    proxy_event_sink_clear();

    /* EVENT_OBJECT_FOCUS is not handled. */
    hr = IProxyProviderWinEventHandler_RespondToWinEvent(handler, EVENT_OBJECT_FOCUS, hwnd, OBJID_CLIENT, CHILDID_SELF,
            &ProxyEventSink.IProxyProviderWinEventSink_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* EVENT_OBJECT_NAMECHANGE. */
    SET_EXPECT(ProxyEventSink_AddAutomationPropertyChangedEvent);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accName);
    hr = IProxyProviderWinEventHandler_RespondToWinEvent(handler, EVENT_OBJECT_NAMECHANGE, hwnd, OBJID_CLIENT, CHILDID_SELF,
            &ProxyEventSink.IProxyProviderWinEventSink_iface);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(ProxyEventSink.prop_change_elprov == elprov, "Unexpected prop_change_elprov %p\n", ProxyEventSink.prop_change_elprov);
    todo_wine ok(ProxyEventSink.prop_change_prop_id == UIA_NamePropertyId, "Unexpected prop_change_prop_id %d\n",
            ProxyEventSink.prop_change_prop_id);
    todo_wine ok(V_VT(&ProxyEventSink.prop_change_value) == VT_BSTR, "Unexpected prop_change_value vt %d\n",
            V_VT(&ProxyEventSink.prop_change_value));
    if (V_VT(&ProxyEventSink.prop_change_value) == VT_BSTR)
        ok(!lstrcmpW(V_BSTR(&ProxyEventSink.prop_change_value), Accessible.name), "Unexpected BSTR %s\n",
                wine_dbgstr_w(V_BSTR(&ProxyEventSink.prop_change_value)));
    todo_wine CHECK_CALLED(ProxyEventSink_AddAutomationPropertyChangedEvent);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible, get_accName);
    proxy_event_sink_clear();

    /* EVENT_OBJECT_REORDER. */
    acc_client = &Accessible.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(ProxyEventSink_AddStructureChangedEvent);
    hr = IProxyProviderWinEventHandler_RespondToWinEvent(handler, EVENT_OBJECT_REORDER, hwnd, OBJID_CLIENT, CHILDID_SELF,
            &ProxyEventSink.IProxyProviderWinEventSink_iface);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(ProxyEventSink.structure_change_elprov == elprov, "Unexpected structure_change_elprov %p\n",
            ProxyEventSink.structure_change_elprov);
    todo_wine ok(ProxyEventSink.structure_change_type == StructureChangeType_ChildrenInvalidated, "Unexpected structure_change_type %d\n",
            ProxyEventSink.structure_change_type);
    ok(!ProxyEventSink.structure_change_rt_id, "structure_change_rt_id != NULL\n");
    todo_wine CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    todo_wine CHECK_CALLED(ProxyEventSink_AddStructureChangedEvent);
    proxy_event_sink_clear();

    acc_client = NULL;
    IProxyProviderWinEventHandler_Release(handler);
    IRawElementProviderSimple_Release(elprov);
}

DEFINE_GUID(SID_AccFromDAWrapper, 0x33f139ee, 0xe509, 0x47f7, 0xbf,0x39, 0x83,0x76,0x44,0xf7,0x45,0x76);
static IAccessible *msaa_acc_da_unwrap(IAccessible *acc)
{
    IServiceProvider *sp;
    HRESULT hr;

    hr = IAccessible_QueryInterface(acc, &IID_IServiceProvider, (void**)&sp);
    if (SUCCEEDED(hr))
    {
        IAccessible *acc2 = NULL;

        hr = IServiceProvider_QueryService(sp, &SID_AccFromDAWrapper, &IID_IAccessible, (void**)&acc2);
        IServiceProvider_Release(sp);
        if (SUCCEEDED(hr) && acc2)
            return acc2;
    }

    return NULL;
}

#define check_msaa_prov_acc( elprov, acc, cid) \
        check_msaa_prov_acc_( ((IUnknown *)elprov), (acc), (cid), __LINE__)
static void check_msaa_prov_acc_(IUnknown *elprov, IAccessible *acc, INT cid, int line)
{
    ILegacyIAccessibleProvider *accprov;
    IAccessible *acc2, *acc3;
    INT child_id;
    HRESULT hr;

    hr = IUnknown_QueryInterface(elprov, &IID_ILegacyIAccessibleProvider, (void **)&accprov);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(!!accprov, "accprov == NULL\n");

    acc2 = acc3 = NULL;
    hr = ILegacyIAccessibleProvider_GetIAccessible(accprov, &acc2);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /*
     * Potentially get our IAccessible out of a direct annotation wrapper
     * IAccessible.
     */
    if (acc && acc2 && (acc != acc2) && (acc3 = msaa_acc_da_unwrap(acc2)))
    {
        IAccessible_Release(acc2);
        acc2 = acc3;
    }
    ok_(__FILE__, line)(acc2 == acc, "acc2 != acc\n");
    if (acc2)
        IAccessible_Release(acc2);

    hr = ILegacyIAccessibleProvider_get_ChildId(accprov, &child_id);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(child_id == cid, "child_id != cid\n");

    ILegacyIAccessibleProvider_Release(accprov);
}

#define check_msaa_prov_host_elem_prov( elem, exp_host_prov) \
        check_msaa_prov_host_elem_prov_( ((IUnknown *)elem), (exp_host_prov), __LINE__)
static void check_msaa_prov_host_elem_prov_(IUnknown *elem, BOOL exp_host_prov, int line)
{
    IRawElementProviderSimple *elprov, *elprov2;
    HRESULT hr;

    hr = IUnknown_QueryInterface(elem, &IID_IRawElementProviderSimple, (void **)&elprov);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(!!elprov, "elprov == NULL\n");

    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)((elprov2 != (void *)0xdeadbeef) && !!elprov2 == exp_host_prov, "Unexpected provider %p from get_HostRawElementProvider\n", elprov2);

    if (elprov2)
        IRawElementProviderSimple_Release(elprov2);
    IRawElementProviderSimple_Release(elprov);
}

static void test_uia_prov_from_acc_fragment_root(HWND hwnd)
{
    IRawElementProviderFragmentRoot *elroot, *elroot2;
    IRawElementProviderFragment *elfrag, *elfrag2;
    IRawElementProviderSimple *elprov;
    SAFEARRAY *ret_arr;
    ULONG old_ref;
    HRESULT hr;

    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSED, 0, L"acc_name", 0, 0, 0, 0);
    set_accessible_ia2_props(&Accessible, FALSE, 0);
    Accessible.ow_hwnd = hwnd;

    elprov = NULL;
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov, "elprov == NULL\n");

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");

    /* GetEmbeddedFragmentRoots test. */
    ret_arr = (void *)0xdeadbeef;
    hr = IRawElementProviderFragment_GetEmbeddedFragmentRoots(elfrag, &ret_arr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!ret_arr, "ret_arr != NULL\n");

    /*
     * get_FragmentRoot does the equivalent of calling
     * AccessibleObjectFromWindow with OBJID_CLIENT on the HWND associated
     * with our IAccessible. Unlike UiaProviderFromIAccessible, it will create
     * a provider from a default oleacc proxy.
     */
    elroot = NULL;
    acc_client = NULL;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = IRawElementProviderFragment_get_FragmentRoot(elfrag, &elroot);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elroot, "elroot == NULL\n");
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    /*
     * ILegacyIAccessibleProvider::GetIAccessible returns a NULL
     * IAccessible if the provider represents an oleacc proxy.
     */
    check_msaa_prov_acc(elroot, NULL, CHILDID_SELF);

    /*
     * Returns a provider from get_HostRawElementProvider without having
     * to query the HWND.
     */
    check_msaa_prov_host_elem_prov(elroot, TRUE);

    hr = IRawElementProviderFragmentRoot_QueryInterface(elroot, &IID_IRawElementProviderFragment, (void **)&elfrag2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");

    /*
     * Even on a provider retrieved from get_FragmentRoot, the HWND is
     * queried and a new fragment root is returned rather than just
     * returning our current fragment root interface.
     */
    elroot2 = NULL;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = IRawElementProviderFragment_get_FragmentRoot(elfrag2, &elroot2);
    IRawElementProviderFragment_Release(elfrag2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elroot2, "elroot2 == NULL\n");
    check_msaa_prov_acc(elroot2, NULL, CHILDID_SELF);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    ok(!iface_cmp((IUnknown *)elroot, (IUnknown *)elroot2), "elroot2 == elroot\n");
    IRawElementProviderFragmentRoot_Release(elroot2);
    IRawElementProviderFragmentRoot_Release(elroot);

    /*
     * Accessible is now the IAccessible for our HWND, so we'll get it instead
     * of an oleacc proxy.
     */
    acc_client = &Accessible.IAccessible_iface;
    elroot = NULL;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = IRawElementProviderFragment_get_FragmentRoot(elfrag, &elroot);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elroot, "elroot == NULL\n");
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    check_msaa_prov_acc(elroot, &Accessible.IAccessible_iface, CHILDID_SELF);

    /*
     * Returns a provider from get_HostRawElementProvider without having
     * to query the HWND, same as before.
     */
    check_msaa_prov_host_elem_prov(elroot, TRUE);

    hr = IRawElementProviderFragmentRoot_QueryInterface(elroot, &IID_IRawElementProviderFragment, (void **)&elfrag2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");

    /* Same deal as before, unique FragmentRoot even on a known root. */
    elroot2 = NULL;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = IRawElementProviderFragment_get_FragmentRoot(elfrag2, &elroot2);
    IRawElementProviderFragment_Release(elfrag2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elroot2, "elroot2 == NULL\n");
    check_msaa_prov_acc(elroot2, &Accessible.IAccessible_iface, CHILDID_SELF);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    ok(!iface_cmp((IUnknown *)elroot, (IUnknown *)elroot2), "elroot2 == elroot\n");
    IRawElementProviderFragmentRoot_Release(elroot2);

    IRawElementProviderFragmentRoot_Release(elroot);
    IRawElementProviderFragment_Release(elfrag);

    /*
     * IRawElementProviderFragmentRoot::GetFocus will call get_accFocus.
     */
    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragmentRoot, (void **)&elroot);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elroot, "elroot == NULL\n");

    /* Focus is CHILDID_SELF, returns NULL. */
    elfrag = (void *)0xdeadbeef;
    Accessible.focus_child_id = CHILDID_SELF;
    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag, "elfrag != NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);

    /*
     * get_accFocus returns child ID 1, which is a simple child element.
     * get_accState for child ID 1 returns STATE_SYSTEM_INVISIBLE, so no
     * element will be returned.
     */
    elfrag = (void *)0xdeadbeef;
    Accessible.focus_child_id = 1;

    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag, "elfrag != NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);

    /*
     * get_accFocus returns child ID 3, which is another simple child
     * element. get_accState for child ID 3 does not have
     * STATE_SYSTEM_INVISIBLE set, so it will return an element.
     */
    elfrag = (void *)0xdeadbeef;
    Accessible.focus_child_id = 3;

    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");
    check_msaa_prov_acc(elfrag, &Accessible.IAccessible_iface, 3);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);

    IRawElementProviderFragment_Release(elfrag);

    /*
     * get_accFocus returns child ID 2 which is a full IAccessible,
     * Accessible_child.
     */
    elfrag = (void *)0xdeadbeef;
    Accessible.focus_child_id = 2;
    Accessible_child.state = STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_OFFSCREEN;

    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible_child, accNavigate);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accFocus);
    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, accNavigate);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accFocus);

    check_msaa_prov_acc(elfrag, &Accessible_child.IAccessible_iface, CHILDID_SELF);
    IRawElementProviderFragment_Release(elfrag);

    /*
     * get_accFocus returns child ID 2 which is a full IAccessible,
     * Accessible_child. It returns failure from get_accState so it isn't
     * returned.
     */
    elfrag = (void *)0xdeadbeef;
    Accessible.focus_child_id = 2;
    Accessible_child.state = 0;

    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible_child, accNavigate);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accState);
    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag, "elfrag != NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, accNavigate);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accState);

    /*
     * get_accFocus returns child ID 7 which is a full IAccessible,
     * Accessible. This is the same IAccessible interface as the one we called
     * get_accFocus on, so it is ignored. Same behavior as CHILDID_SELF.
     */
    elfrag = (void *)0xdeadbeef;
    Accessible.focus_child_id = 7;
    Accessible.state = STATE_SYSTEM_FOCUSABLE;

    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag, "elfrag != NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);

    /*
     * Return E_NOTIMPL from get_accFocus, returns a new provider representing
     * the same IAccessible.
     */
    elfrag = (void *)0xdeadbeef;
    old_ref = Accessible.ref;
    Accessible.focus_child_id = -1;

    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");
    ok(Accessible.ref > old_ref, "Unexpected ref %ld\n", Accessible.ref);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);

    /* Two unique COM objects that represent the same IAccessible. */
    ok(!iface_cmp((IUnknown *)elroot, (IUnknown *)elfrag), "elroot == elfrag\n");
    check_msaa_prov_acc(elfrag, &Accessible.IAccessible_iface, CHILDID_SELF);
    IRawElementProviderFragment_Release(elfrag);
    ok(Accessible.ref == old_ref, "Unexpected ref %ld\n", Accessible.ref);
    Accessible.focus_child_id = CHILDID_SELF;

    /*
     * Similar to CHILDID_SELF, if the same IAccessible interface is returned
     * as a VT_DISPATCH, we'll get no element.
     */
    elfrag = (void *)0xdeadbeef;
    Accessible.focus_acc = &Accessible.IAccessible_iface;

    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag, "elfrag != NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);

    /*
     * Return Accessible_child as a VT_DISPATCH - will get an element.
     */
    elfrag = (void *)0xdeadbeef;
    Accessible.focus_acc = &Accessible_child.IAccessible_iface;
    Accessible_child.state = STATE_SYSTEM_FOCUSABLE;

    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible_child, accNavigate);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accFocus);
    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, accNavigate);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accFocus);

    check_msaa_prov_acc(elfrag, &Accessible_child.IAccessible_iface, CHILDID_SELF);
    IRawElementProviderFragment_Release(elfrag);

    /*
     * Fail get_accFocus on child.
     */
    Accessible_child.focus_child_id = -1;
    Accessible_child.state = STATE_SYSTEM_FOCUSABLE;

    elfrag = (void *)0xdeadbeef;
    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible_child, accNavigate);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accFocus);
    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, accNavigate);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accFocus);

    check_msaa_prov_acc(elfrag, &Accessible_child.IAccessible_iface, CHILDID_SELF);
    IRawElementProviderFragment_Release(elfrag);

    IRawElementProviderFragmentRoot_Release(elroot);
    IRawElementProviderSimple_Release(elprov);

    /*
     * Test simple child element.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, 1, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");

    /* GetEmbeddedFragmentRoots test. */
    ret_arr = (void *)0xdeadbeef;
    hr = IRawElementProviderFragment_GetEmbeddedFragmentRoots(elfrag, &ret_arr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!ret_arr, "ret_arr != NULL\n");

    /*
     * Simple child element queries HWND as well, does not just return its
     * parent.
     */
    elroot = NULL;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = IRawElementProviderFragment_get_FragmentRoot(elfrag, &elroot);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elroot, "elroot == NULL\n");
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    check_msaa_prov_acc(elroot, &Accessible.IAccessible_iface, CHILDID_SELF);
    check_msaa_prov_host_elem_prov(elroot, TRUE);

    IRawElementProviderFragmentRoot_Release(elroot);
    IRawElementProviderFragment_Release(elfrag);

    /*
     * IRawElementProviderFragmentRoot::GetFocus will not call get_accFocus
     * on a simple child IAccessible.
     */
    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragmentRoot, (void **)&elroot);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elroot, "elroot == NULL\n");

    elfrag = (void *)0xdeadbeef;
    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag, "elfrag != NULL\n");

    IRawElementProviderFragmentRoot_Release(elroot);
    IRawElementProviderSimple_Release(elprov);

    /*
     * Test child of root HWND IAccessible.
     */
    set_accessible_props(&Accessible_child, ROLE_SYSTEM_TEXT, 0, 0, L"acc_child_name", 0, 0, 0, 0);
    set_accessible_ia2_props(&Accessible_child, FALSE, 0);

    elprov = NULL;
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent); /* Gets HWND from parent IAccessible. */
    SET_ACC_METHOD_EXPECT(&Accessible_child, accNavigate);
    hr = pUiaProviderFromIAccessible(&Accessible_child.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov, "elprov == NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, accNavigate);

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");

    /* GetEmbeddedFragmentRoots test. */
    ret_arr = (void *)0xdeadbeef;
    hr = IRawElementProviderFragment_GetEmbeddedFragmentRoots(elfrag, &ret_arr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!ret_arr, "ret_arr != NULL\n");

    /*
     * Again, same behavior as simple children. It doesn't just retrieve the
     * parent IAccessible, it queries the HWND.
     */
    elroot = NULL;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = IRawElementProviderFragment_get_FragmentRoot(elfrag, &elroot);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elroot, "elroot == NULL\n");
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    check_msaa_prov_acc(elroot, &Accessible.IAccessible_iface, CHILDID_SELF);

    IRawElementProviderFragmentRoot_Release(elroot);
    IRawElementProviderFragment_Release(elfrag);

    /*
     * GetFocus tests.
     */
    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragmentRoot, (void **)&elroot);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elroot, "elroot == NULL\n");

    /*
     * get_accFocus returns E_NOTIMPL, returns new provider for same
     * IAccessible.
     */
    elfrag = (void *)0xdeadbeef;
    old_ref = Accessible_child.ref;
    Accessible_child.focus_child_id = -1;
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accFocus);
    hr = IRawElementProviderFragmentRoot_GetFocus(elroot, &elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");
    ok(Accessible_child.ref > old_ref, "Unexpected ref %ld\n", Accessible.ref);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accFocus);

    /* Again, two unique COM objects that represent the same IAccessible. */
    ok(!iface_cmp((IUnknown *)elroot, (IUnknown *)elfrag), "elroot == elfrag\n");
    check_msaa_prov_acc(elfrag, &Accessible_child.IAccessible_iface, CHILDID_SELF);
    IRawElementProviderFragment_Release(elfrag);
    ok(Accessible_child.ref == old_ref, "Unexpected ref %ld\n", Accessible.ref);

    IRawElementProviderFragmentRoot_Release(elroot);
    IRawElementProviderSimple_Release(elprov);

    Accessible.focus_child_id = Accessible_child.focus_child_id = CHILDID_SELF;
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(Accessible_child.ref == 1, "Unexpected refcnt %ld\n", Accessible_child.ref);
    acc_client = NULL;
}

struct msaa_role_uia_type {
    INT acc_role;
    INT uia_control_type;
};

static const struct msaa_role_uia_type msaa_role_uia_types[] = {
    { ROLE_SYSTEM_TITLEBAR,           UIA_TitleBarControlTypeId },
    { ROLE_SYSTEM_MENUBAR,            UIA_MenuBarControlTypeId },
    { ROLE_SYSTEM_SCROLLBAR,          UIA_ScrollBarControlTypeId },
    { ROLE_SYSTEM_GRIP,               UIA_ThumbControlTypeId },
    { ROLE_SYSTEM_WINDOW,             UIA_WindowControlTypeId },
    { ROLE_SYSTEM_MENUPOPUP,          UIA_MenuControlTypeId },
    { ROLE_SYSTEM_MENUITEM,           UIA_MenuItemControlTypeId },
    { ROLE_SYSTEM_TOOLTIP,            UIA_ToolTipControlTypeId },
    { ROLE_SYSTEM_APPLICATION,        UIA_WindowControlTypeId },
    { ROLE_SYSTEM_DOCUMENT,           UIA_DocumentControlTypeId },
    { ROLE_SYSTEM_PANE,               UIA_PaneControlTypeId },
    { ROLE_SYSTEM_GROUPING,           UIA_GroupControlTypeId },
    { ROLE_SYSTEM_SEPARATOR,          UIA_SeparatorControlTypeId },
    { ROLE_SYSTEM_TOOLBAR,            UIA_ToolBarControlTypeId },
    { ROLE_SYSTEM_STATUSBAR,          UIA_StatusBarControlTypeId },
    { ROLE_SYSTEM_TABLE,              UIA_TableControlTypeId },
    { ROLE_SYSTEM_COLUMNHEADER,       UIA_HeaderControlTypeId },
    { ROLE_SYSTEM_ROWHEADER,          UIA_HeaderControlTypeId },
    { ROLE_SYSTEM_CELL,               UIA_DataItemControlTypeId },
    { ROLE_SYSTEM_LINK,               UIA_HyperlinkControlTypeId },
    { ROLE_SYSTEM_LIST,               UIA_ListControlTypeId },
    { ROLE_SYSTEM_LISTITEM,           UIA_ListItemControlTypeId },
    { ROLE_SYSTEM_OUTLINE,            UIA_TreeControlTypeId },
    { ROLE_SYSTEM_OUTLINEITEM,        UIA_TreeItemControlTypeId },
    { ROLE_SYSTEM_PAGETAB,            UIA_TabItemControlTypeId },
    { ROLE_SYSTEM_INDICATOR,          UIA_ThumbControlTypeId },
    { ROLE_SYSTEM_GRAPHIC,            UIA_ImageControlTypeId },
    { ROLE_SYSTEM_STATICTEXT,         UIA_TextControlTypeId },
    { ROLE_SYSTEM_TEXT,               UIA_EditControlTypeId },
    { ROLE_SYSTEM_PUSHBUTTON,         UIA_ButtonControlTypeId },
    { ROLE_SYSTEM_CHECKBUTTON,        UIA_CheckBoxControlTypeId },
    { ROLE_SYSTEM_RADIOBUTTON,        UIA_RadioButtonControlTypeId },
    { ROLE_SYSTEM_COMBOBOX,           UIA_ComboBoxControlTypeId },
    { ROLE_SYSTEM_PROGRESSBAR,        UIA_ProgressBarControlTypeId },
    { ROLE_SYSTEM_SLIDER,             UIA_SliderControlTypeId },
    { ROLE_SYSTEM_SPINBUTTON,         UIA_SpinnerControlTypeId },
    { ROLE_SYSTEM_BUTTONDROPDOWN,     UIA_SplitButtonControlTypeId },
    { ROLE_SYSTEM_BUTTONMENU,         UIA_MenuItemControlTypeId },
    { ROLE_SYSTEM_BUTTONDROPDOWNGRID, UIA_ButtonControlTypeId },
    { ROLE_SYSTEM_PAGETABLIST,        UIA_TabControlTypeId },
    { ROLE_SYSTEM_SPLITBUTTON,        UIA_SplitButtonControlTypeId },
    /* These accessible roles have no equivalent in UI Automation. */
    { ROLE_SYSTEM_SOUND,              0 },
    { ROLE_SYSTEM_CURSOR,             0 },
    { ROLE_SYSTEM_CARET,              0 },
    { ROLE_SYSTEM_ALERT,              0 },
    { ROLE_SYSTEM_CLIENT,             0 },
    { ROLE_SYSTEM_CHART,              0 },
    { ROLE_SYSTEM_DIALOG,             0 },
    { ROLE_SYSTEM_BORDER,             0 },
    { ROLE_SYSTEM_COLUMN,             0 },
    { ROLE_SYSTEM_ROW,                0 },
    { ROLE_SYSTEM_HELPBALLOON,        0 },
    { ROLE_SYSTEM_CHARACTER,          0 },
    { ROLE_SYSTEM_PROPERTYPAGE,       0 },
    { ROLE_SYSTEM_DROPLIST,           0 },
    { ROLE_SYSTEM_DIAL,               0 },
    { ROLE_SYSTEM_HOTKEYFIELD,        0 },
    { ROLE_SYSTEM_DIAGRAM,            0 },
    { ROLE_SYSTEM_ANIMATION,          0 },
    { ROLE_SYSTEM_EQUATION,           0 },
    { ROLE_SYSTEM_WHITESPACE,         0 },
    { ROLE_SYSTEM_IPADDRESS,          0 },
    { ROLE_SYSTEM_OUTLINEBUTTON,      0 },
};

struct msaa_state_uia_prop {
    INT acc_state;
    INT prop_id;
};

static const struct msaa_state_uia_prop msaa_state_uia_props[] = {
    { STATE_SYSTEM_FOCUSED,      UIA_HasKeyboardFocusPropertyId },
    { STATE_SYSTEM_FOCUSABLE,    UIA_IsKeyboardFocusablePropertyId },
    { ~STATE_SYSTEM_UNAVAILABLE, UIA_IsEnabledPropertyId },
    { STATE_SYSTEM_PROTECTED,    UIA_IsPasswordPropertyId },
};

static void set_accessible_props(struct Accessible *acc, INT role, INT state,
        LONG child_count, LPCWSTR name, LONG left, LONG top, LONG width, LONG height)
{

    acc->role = role;
    acc->state = state;
    acc->child_count = child_count;
    acc->name = name;
    acc->left = left;
    acc->top = top;
    acc->width = width;
    acc->height = height;
}

static void set_accessible_ia2_props(struct Accessible *acc, BOOL enable_ia2, LONG unique_id)
{
    acc->enable_ia2 = enable_ia2;
    acc->unique_id = unique_id;
}

static void test_uia_prov_from_acc_ia2(void)
{
    IRawElementProviderSimple *elprov, *elprov2;
    HRESULT hr;

    /* Only one exposes an IA2 interface, no match. */
    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, 0, 0, L"acc_name", 0, 0, 0, 0);
    set_accessible_ia2_props(&Accessible, TRUE, 0);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_TEXT, 0, 0, L"acc_name", 0, 0, 0, 0);
    set_accessible_ia2_props(&Accessible2, FALSE, 0);

    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (Accessible.ref != 3)
    {
        IRawElementProviderSimple_Release(elprov);
        win_skip("UiaProviderFromIAccessible has no IAccessible2 support, skipping tests.\n");
        return;
    }

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    /* The four below are only called on Win10v1909. */
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible2, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    /*
     * Win10v1909 has IAccessible2 support, but it's not used for checking if
     * two IAccessible interfaces match. Skip the comparison tests for this
     * Windows version.
     */
    if (Accessible.called_get_accRole)
    {
        IRawElementProviderSimple_Release(elprov);
        CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);
        CHECK_ACC_METHOD_CALLED(&Accessible2, get_accRole);
        CHECK_ACC_METHOD_CALLED(&Accessible2, QI_IAccIdentity);
        CHECK_ACC_METHOD_CALLED(&Accessible2, get_accParent);
        win_skip("Win10v1909 doesn't support IAccessible2 interface comparison, skipping tests.\n");
        return;
    }
    Accessible.called_get_accRole = Accessible.expect_get_accRole = 0;
    Accessible2.called_get_accRole = Accessible2.expect_get_accRole = 0;
    Accessible2.called_QI_IAccIdentity = Accessible2.expect_QI_IAccIdentity = 0;
    Accessible2.called_get_accParent = Accessible2.expect_get_accParent = 0;

    Accessible.role = Accessible2.role = 0;
    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * If &Accessible returns a failure code on get_uniqueID, &Accessible2's
     * uniqueID is not checked.
     */
    set_accessible_ia2_props(&Accessible, TRUE, 0);
    set_accessible_ia2_props(&Accessible2, TRUE, 0);
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accName);
    SET_ACC_METHOD_EXPECT(&Accessible, get_uniqueID);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accName);
    SET_ACC_METHOD_EXPECT(&Accessible2, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accName);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_uniqueID);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accName);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, QI_IAccIdentity);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, get_accParent);
    IRawElementProviderSimple_Release(elprov2);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Unique ID matches. */
    set_accessible_ia2_props(&Accessible, TRUE, 1);
    set_accessible_ia2_props(&Accessible2, TRUE, 1);
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible, get_uniqueID);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_uniqueID);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_uniqueID);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_uniqueID);
    IRawElementProviderSimple_Release(elprov2);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Unique ID mismatch. */
    set_accessible_ia2_props(&Accessible, TRUE, 1);
    set_accessible_ia2_props(&Accessible2, TRUE, 2);
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible, get_uniqueID);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_uniqueID);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_uniqueID);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_uniqueID);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, 0, 0, 0, NULL, 0, 0, 0, 0);
    set_accessible_ia2_props(&Accessible, FALSE, 0);
    set_accessible_props(&Accessible2, 0, 0, 0, NULL, 0, 0, 0, 0);
    set_accessible_ia2_props(&Accessible2, FALSE, 0);
}

#define check_fragment_acc( fragment, acc, cid) \
        check_fragment_acc_( (fragment), (acc), (cid), __LINE__)
static void check_fragment_acc_(IRawElementProviderFragment *elfrag, IAccessible *acc,
        INT cid, int line)
{
    ILegacyIAccessibleProvider *accprov;
    IAccessible *accessible;
    INT child_id;
    HRESULT hr;

    hr = IRawElementProviderFragment_QueryInterface(elfrag, &IID_ILegacyIAccessibleProvider, (void **)&accprov);
    ok_(__FILE__, line) (hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line) (!!accprov, "accprov == NULL\n");

    hr = ILegacyIAccessibleProvider_GetIAccessible(accprov, &accessible);
    ok_(__FILE__, line) (hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line) (accessible == acc, "accessible != acc\n");
    IAccessible_Release(accessible);

    hr = ILegacyIAccessibleProvider_get_ChildId(accprov, &child_id);
    ok_(__FILE__, line) (hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line) (child_id == cid, "child_id != cid\n");

    ILegacyIAccessibleProvider_Release(accprov);
}

static void test_uia_prov_from_acc_navigation(void)
{
    IRawElementProviderFragment *elfrag, *elfrag2, *elfrag3;
    IRawElementProviderSimple *elprov, *elprov2;
    HRESULT hr;

    /*
     * Full IAccessible parent, with 4 children:
     * childid 1 is a simple element, with STATE_SYSTEM_INVISIBLE.
     * childid 2 is Accessible_child.
     * childid 3 is a simple element with STATE_SYSTEM_NORMAL.
     * childid 4 is Accessible_child2.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");

    /*
     * First time doing NavigateDirection_Parent will result in the same root
     * accessible check as get_HostRawElementProvider. If this IAccessible is
     * the root for its associated HWND, NavigateDirection_Parent and
     * NavigateDirection_Next/PreviousSibling will do nothing, as UI Automation
     * provides non-client area providers for the root IAccessible's parent
     * and siblings.
     */
    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 4,
            L"acc_name", 0, 0, 50, 50);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 4,
            L"acc_name", 0, 0, 50, 50);
    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accName);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible2, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accName);
    SET_ACC_METHOD_EXPECT(&Accessible2, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accParent);
    elfrag2 = (void *)0xdeadbeef;
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_Parent, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accName);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible2, accLocation);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accName);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, QI_IAccIdentity);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, get_accParent);
    acc_client = NULL;

    /* No check against root IAccessible, since it was done previously. */
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    /* Do nothing. */
    elfrag2 = (void *)0xdeadbeef;
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_Parent, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");

    elfrag2 = (void *)0xdeadbeef;
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_NextSibling, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");

    elfrag2 = (void *)0xdeadbeef;
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_PreviousSibling, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");

    /*
     * Retrieve childid 2 (Accessible_child) as first child. childid 1 is skipped due to
     * having a state of STATE_SYSTEM_INVISIBLE.
     */
    set_accessible_props(&Accessible_child, 0, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible_child2, 0, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accChildCount, 3);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accChild, 2);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible_child, accNavigate);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_FirstChild, &elfrag2);
    ok(Accessible_child.ref == 2, "Unexpected refcnt %ld\n", Accessible_child.ref);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible, get_accChildCount, 3);
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible, get_accChild, 2);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, accNavigate);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);

    check_fragment_acc(elfrag2, &Accessible_child.IAccessible_iface, CHILDID_SELF);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    hr = IRawElementProviderFragment_Navigate(elfrag2, NavigateDirection_NextSibling, &elfrag3);
    ok(Accessible.ref == 5, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag3, "elfrag2 == NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    check_fragment_acc(elfrag3, &Accessible.IAccessible_iface, 3);

    IRawElementProviderFragment_Release(elfrag3);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible_child.ref == 1, "Unexpected refcnt %ld\n", Accessible_child.ref);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Retrieve childid 3 as first child now that Accessible_child is invisible. */
    set_accessible_props(&Accessible_child, 0, STATE_SYSTEM_INVISIBLE, 0, NULL, 0, 0, 0, 0);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accChildCount, 4);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accChild, 3);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accState, 2);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accState);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_FirstChild, &elfrag2);
    ok(Accessible.ref == 4, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible, get_accChildCount, 4);
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible, get_accChild, 3);
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible, get_accState, 2);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accState);
    check_fragment_acc(elfrag2, &Accessible.IAccessible_iface, 3);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Retrieve childid 4 (Accessible_child2) as last child. */
    set_accessible_props(&Accessible_child2, 0, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accChildCount, 2);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, accNavigate);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accParent);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_LastChild, &elfrag2);
    ok(Accessible_child2.ref == 2, "Unexpected refcnt %ld\n", Accessible_child2.ref);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible, get_accChildCount, 2);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, accNavigate);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accParent);

    check_fragment_acc(elfrag2, &Accessible_child2.IAccessible_iface, CHILDID_SELF);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    hr = IRawElementProviderFragment_Navigate(elfrag2, NavigateDirection_PreviousSibling, &elfrag3);
    ok(Accessible.ref == 5, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag3, "elfrag2 == NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    check_fragment_acc(elfrag3, &Accessible.IAccessible_iface, 3);

    IRawElementProviderFragment_Release(elfrag3);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible_child2.ref == 1, "Unexpected refcnt %ld\n", Accessible_child2.ref);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Retrieve childid 3 as last child, now that Accessible_child2 is STATE_SYSTEM_INVISIBLE. */
    set_accessible_props(&Accessible_child2, 0, STATE_SYSTEM_INVISIBLE, 0, NULL, 0, 0, 0, 0);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accChildCount, 3);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accChild, 2);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accState);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_LastChild, &elfrag2);
    ok(Accessible.ref == 4, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible, get_accChildCount, 3);
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible, get_accChild, 2);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accState);
    check_fragment_acc(elfrag2, &Accessible.IAccessible_iface, 3);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    IRawElementProviderFragment_Release(elfrag);
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Full IAccessible child tests.
     */
    SET_ACC_METHOD_EXPECT(&Accessible_child, accNavigate);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent);
    hr = pUiaProviderFromIAccessible(&Accessible_child.IAccessible_iface, 0, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, accNavigate);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);
    ok(Accessible_child.ref == 2, "Unexpected refcnt %ld\n", Accessible_child.ref);

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");

    /*
     * After determining this isn't the root IAccessible, get_accParent will
     * be used to get the parent.
     */
    set_accessible_props(&Accessible2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible_child, ROLE_SYSTEM_CLIENT, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);
    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible2, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accParent);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_Parent, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accRole);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, QI_IAccIdentity);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, get_accParent);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    check_fragment_acc(elfrag2, &Accessible.IAccessible_iface, CHILDID_SELF);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);
    acc_client = NULL;

    /* Second call only does get_accParent, no root check. */
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_Parent, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);
    check_fragment_acc(elfrag2, &Accessible.IAccessible_iface, CHILDID_SELF);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* ChildCount of 0, do nothing for First/Last child.*/
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accChildCount);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_FirstChild, &elfrag2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accChildCount);

    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accChildCount);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_LastChild, &elfrag2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accChildCount);

    /*
     * In the case of sibling navigation on an IAccessible that wasn't
     * received through previous navigation from a parent (i.e, from
     * NavigateDirection_First/LastChild), we have to figure out which
     * IAccessible child we represent by comparing against all children of our
     * IAccessible parent. If we find more than one IAccessible that matches,
     * or none at all that do, navigation will fail.
     */
    set_accessible_props(&Accessible_child, ROLE_SYSTEM_CLIENT, STATE_SYSTEM_FOCUSABLE, 1,
            L"acc_child", 0, 0, 50, 50);
    set_accessible_props(&Accessible_child2, ROLE_SYSTEM_CLIENT, STATE_SYSTEM_FOCUSABLE, 1,
            L"acc_child", 0, 0, 50, 50);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accChildCount, 5);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accChild, 4);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible_child, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accName);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accName);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_NextSibling, &elfrag2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible, get_accChildCount, 5);
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible, get_accChild, 4);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, accLocation);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accName);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, accLocation);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accName);

    /* Now they have a role mismatch, we can determine our position. */
    set_accessible_props(&Accessible_child2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1,
            L"acc_child", 0, 0, 50, 50);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accChildCount, 6);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accChild, 5);
    /* Check ChildID 1 for STATE_SYSTEM_INVISIBLE. */
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accRole);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_PreviousSibling, &elfrag2);
    /*
     * Even though we didn't get a new fragment, now that we know our
     * position, a reference is added to the parent IAccessible.
     */
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible, get_accChildCount, 6);
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible, get_accChild, 5);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accRole);

    /* Now that we know our position, no extra nav work. */
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_NextSibling, &elfrag2);
    ok(Accessible.ref == 4, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    if (elfrag2)
    {
        check_fragment_acc(elfrag2, &Accessible.IAccessible_iface, 3);
        IRawElementProviderFragment_Release(elfrag2);
        ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    }

    IRawElementProviderFragment_Release(elfrag);
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible_child.ref == 1, "Unexpected refcnt %ld\n", Accessible_child.ref);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Simple element child tests.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, 1, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");

    /*
     * Simple child elements don't check the root IAccessible, because they
     * can't be the root IAccessible.
     */
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_Parent, &elfrag2);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    check_fragment_acc(elfrag2, &Accessible.IAccessible_iface, CHILDID_SELF);
    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Test NavigateDirection_First/LastChild on simple child element. Does
     * nothing, as simple children cannot have children.
     */
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_FirstChild, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");

    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_LastChild, &elfrag2);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elfrag2, "elfrag2 != NULL\n");

    /*
     * NavigateDirection_Next/PreviousSibling behaves normally, no IAccessible
     * comparisons.
     */
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible_child, accNavigate);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent);
    hr = IRawElementProviderFragment_Navigate(elfrag, NavigateDirection_NextSibling, &elfrag2);
    ok(Accessible_child.ref == 2, "Unexpected refcnt %ld\n", Accessible_child.ref);
    ok(Accessible.ref == 4, "Unexpected refcnt %ld\n", Accessible.ref);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag2, "elfrag2 == NULL\n");
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, accNavigate);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);
    check_fragment_acc(elfrag2, &Accessible_child.IAccessible_iface, CHILDID_SELF);

    IRawElementProviderFragment_Release(elfrag2);
    ok(Accessible_child.ref == 1, "Unexpected refcnt %ld\n", Accessible_child.ref);
    ok(Accessible.ref == 3, "Unexpected refcnt %ld\n", Accessible.ref);
    IRawElementProviderFragment_Release(elfrag);
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, 0, 0, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible2, 0, 0, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible_child, 0, 0, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible_child2, 0, 0, 0, NULL, 0, 0, 0, 0);
}

static void test_uia_prov_from_acc_properties(void)
{
    IRawElementProviderSimple *elprov;
    RECT rect[2] = { 0 };
    HRESULT hr;
    VARIANT v;
    int i, x;

    /* MSAA role to UIA control type test. */
    VariantInit(&v);
    for (i = 0; i < ARRAY_SIZE(msaa_role_uia_types); i++)
    {
        const struct msaa_role_uia_type *role = &msaa_role_uia_types[i];
        ILegacyIAccessibleProvider *accprov;
        DWORD role_val;
        IUnknown *unk;

        /*
         * Roles get cached once a valid one is mapped, so create a new
         * element for each role.
         */
        hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

        Accessible.role = role->acc_role;
        SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
        VariantClear(&v);
        hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_ControlTypePropertyId, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (role->uia_control_type)
            ok(check_variant_i4(&v, role->uia_control_type), "MSAA role %d: V_I4(&v) = %ld\n", role->acc_role, V_I4(&v));
        else
            ok(V_VT(&v) == VT_EMPTY, "MSAA role %d: V_VT(&v) = %d\n", role->acc_role, V_VT(&v));
        CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);

        if (!role->uia_control_type)
            SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
        VariantClear(&v);
        hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_ControlTypePropertyId, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (role->uia_control_type)
            ok(check_variant_i4(&v, role->uia_control_type), "MSAA role %d: V_I4(&v) = %ld\n", role->acc_role, V_I4(&v));
        else
            ok(V_VT(&v) == VT_EMPTY, "MSAA role %d: V_VT(&v) = %d\n", role->acc_role, V_VT(&v));
        if (!role->uia_control_type)
            CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);

        hr = IRawElementProviderSimple_GetPatternProvider(elprov, UIA_LegacyIAccessiblePatternId, &unk);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!!unk, "unk == NULL\n");

        hr = IUnknown_QueryInterface(unk, &IID_ILegacyIAccessibleProvider, (void **)&accprov);
        IUnknown_Release(unk);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!!accprov, "accprov == NULL\n");

        SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
        hr = ILegacyIAccessibleProvider_get_Role(accprov, &role_val);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(role_val == Accessible.role, "role_val != Accessible.role\n");
        CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);

        ILegacyIAccessibleProvider_Release(accprov);
        IRawElementProviderSimple_Release(elprov);
        ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);
    }

    /* ROLE_SYSTEM_CLOCK has no mapping in Windows < 10 1809. */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    Accessible.role = ROLE_SYSTEM_CLOCK;
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    VariantClear(&v);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(check_variant_i4(&v, UIA_ButtonControlTypeId) || broken(V_VT(&v) == VT_EMPTY), /* Windows < 10 1809 */
            "MSAA role %d: V_I4(&v) = %ld\n", Accessible.role, V_I4(&v));
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);

    if (V_VT(&v) == VT_EMPTY)
        SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    VariantClear(&v);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_ControlTypePropertyId, &v);
    ok(check_variant_i4(&v, UIA_ButtonControlTypeId) || broken(V_VT(&v) == VT_EMPTY), /* Windows < 10 1809 */
            "MSAA role %d: V_I4(&v) = %ld\n", Accessible.role, V_I4(&v));
    if (V_VT(&v) == VT_EMPTY)
        CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);

    Accessible.role = 0;
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    /* UIA PropertyId's that correspond directly to individual MSAA state flags. */
    for (i = 0; i < ARRAY_SIZE(msaa_state_uia_props); i++)
    {
        const struct msaa_state_uia_prop *state = &msaa_state_uia_props[i];

        for (x = 0; x < 2; x++)
        {
            Accessible.state = x ? state->acc_state : ~state->acc_state;
            SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
            hr = IRawElementProviderSimple_GetPropertyValue(elprov, state->prop_id, &v);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(check_variant_bool(&v, x), "V_BOOL(&v) = %#x\n", V_BOOL(&v));
            CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
        }

        /* Failure HRESULTs are passed through. */
        Accessible.state = 0;
        SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
        hr = IRawElementProviderSimple_GetPropertyValue(elprov, state->prop_id, &v);
        ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&v) == VT_EMPTY, "Unexpected V_VT %d\n", V_VT(&v));
        CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    }
    Accessible.state = 0;

    /*
     * UIA_IsOffscreenPropertyId relies upon either STATE_SYSTEM_OFFSCREEN
     * being set, or accLocation returning a location that is within the
     * client area bounding box of the HWND it is contained within.
     */
    set_accessible_props(&Accessible, 0, STATE_SYSTEM_OFFSCREEN, 0, L"Accessible", 0, 0, 0, 0);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_IsOffscreenPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BOOL, "V_VT(&v) = %d\n", V_VT(&v));
    ok(check_variant_bool(&v, TRUE), "Unexpected BOOL %#x\n", V_BOOL(&v));
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);

    /* accLocation fails, will return FALSE. */
    set_accessible_props(&Accessible, 0, ~STATE_SYSTEM_OFFSCREEN, 0, L"Accessible", 0, 0, 0, 0);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_IsOffscreenPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BOOL, "V_VT(&v) = %d\n", V_VT(&v));
    ok(check_variant_bool(&v, FALSE), "Unexpected BOOL %#x\n", V_BOOL(&v));
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);

    /* Window is visible, Accessible is within its bounds. */
    ShowWindow(Accessible.ow_hwnd, SW_SHOW);
    ok(GetClientRect(Accessible.ow_hwnd, &rect[0]), "GetClientRect returned FALSE\n");
    MapWindowPoints(Accessible.ow_hwnd, NULL, (POINT *)&rect[0], 2);

    set_accessible_props(&Accessible, 0, ~STATE_SYSTEM_OFFSCREEN, 0, L"Accessible", rect[0].left, rect[0].top,
            (rect[0].right - rect[0].left), (rect[0].bottom - rect[0].top));
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_IsOffscreenPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BOOL, "Unexpected VT %d\n", V_VT(&v));
    ok(check_variant_bool(&v, FALSE), "Unexpected BOOL %#x\n", V_BOOL(&v));
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);

    /*
     * Window is invisible, Accessible is within its bounds. Window visibility
     * doesn't effect whether or not an IAccessible is considered offscreen.
     */
    ShowWindow(Accessible.ow_hwnd, SW_HIDE);
    set_accessible_props(&Accessible, 0, ~STATE_SYSTEM_OFFSCREEN, 0, L"Accessible", rect[0].left, rect[0].top,
            (rect[0].right - rect[0].left), (rect[0].bottom - rect[0].top));
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_IsOffscreenPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BOOL, "Unexpected VT %d\n", V_VT(&v));
    ok(check_variant_bool(&v, FALSE), "Unexpected BOOL %#x\n", V_BOOL(&v));
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);

    /* Accessible now outside of its window's bounds. */
    set_accessible_props(&Accessible, 0, ~STATE_SYSTEM_OFFSCREEN, 0, L"Accessible", rect[0].right, rect[0].bottom,
            10, 10);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_IsOffscreenPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BOOL, "V_VT(&v) = %d\n", V_VT(&v));
    ok(check_variant_bool(&v, TRUE), "Unexpected BOOL %#x\n", V_BOOL(&v));
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);

    /* Accessible within window bounds, but not client area bounds. */
    ok(GetWindowRect(Accessible.ow_hwnd, &rect[1]), "GetWindowRect returned FALSE\n");
    set_accessible_props(&Accessible, 0, ~STATE_SYSTEM_OFFSCREEN, 0, L"Accessible", rect[1].left, rect[1].top,
            (rect[0].left - rect[1].left) - 1, (rect[0].top - rect[1].top) - 1);

    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_IsOffscreenPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BOOL, "V_VT(&v) = %d\n", V_VT(&v));
    ok(check_variant_bool(&v, TRUE), "Unexpected BOOL %#x\n", V_BOOL(&v));
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* UIA_NamePropertyId tests. */
    set_accessible_props(&Accessible, 0, 0, 0, L"Accessible", 0, 0, 0, 0);
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    SET_ACC_METHOD_EXPECT(&Accessible, get_accName);
    VariantInit(&v);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_NamePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BSTR, "Unexpected VT %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), Accessible.name), "Unexpected BSTR %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accName);

    /* Name is not cached. */
    set_accessible_props(&Accessible, 0, 0, 0, L"Accessible2", 0, 0, 0, 0);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accName);
    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_NamePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BSTR, "Unexpected VT %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), Accessible.name), "Unexpected BSTR %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accName);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);
}

static void test_UiaProviderFromIAccessible(void)
{
    ILegacyIAccessibleProvider *accprov;
    IRawElementProviderSimple *elprov, *elprov2;
    IRawElementProviderFragment *elfrag;
    enum ProviderOptions prov_opt;
    struct UiaRect rect;
    IAccessible *acc;
    IUnknown *unk;
    WNDCLASSA cls;
    HRESULT hr;
    HWND hwnd;
    VARIANT v;
    INT cid;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "UiaProviderFromIAccessible class";

    RegisterClassA(&cls);

    hwnd = CreateWindowA("UiaProviderFromIAccessible class", "Test window", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);

    hr = pUiaProviderFromIAccessible(NULL, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /*
     * UiaProviderFromIAccessible will not wrap an MSAA proxy, this is
     * detected by checking for the 'IIS_IsOleaccProxy' service from the
     * IServiceProvider interface.
     */
    hr = CreateStdAccessibleObject(hwnd, OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!!acc, "acc == NULL\n");

    hr = pUiaProviderFromIAccessible(acc, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    IAccessible_Release(acc);

    /* Don't return an HWND from accNavigate or OleWindow. */
    SET_ACC_METHOD_EXPECT(&Accessible, accNavigate);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accParent);
    Accessible.acc_hwnd = NULL;
    Accessible.ow_hwnd = NULL;
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    CHECK_ACC_METHOD_CALLED(&Accessible, accNavigate);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accParent);

    /* Return an HWND from accNavigate, not OleWindow. */
    SET_ACC_METHOD_EXPECT(&Accessible, accNavigate);
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    acc_client = &Accessible.IAccessible_iface;
    Accessible.acc_hwnd = hwnd;
    Accessible.ow_hwnd = NULL;
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_ACC_METHOD_CALLED(&Accessible, accNavigate);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);
    acc_client = NULL;

    /* Skip tests on Win10v1507. */
    if (called_winproc_GETOBJECT_CLIENT)
    {
        win_skip("UiaProviderFromIAccessible behaves inconsistently on Win10 1507, skipping tests.\n");
        return;
    }
    expect_winproc_GETOBJECT_CLIENT = FALSE;

    /* Return an HWND from parent IAccessible's IOleWindow interface. */
    SET_ACC_METHOD_EXPECT(&Accessible_child, accNavigate);
    SET_ACC_METHOD_EXPECT(&Accessible_child, get_accParent);
    Accessible.acc_hwnd = NULL;
    Accessible.ow_hwnd = hwnd;
    hr = pUiaProviderFromIAccessible(&Accessible_child.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, accNavigate);
    CHECK_ACC_METHOD_CALLED(&Accessible_child, get_accParent);
    ok(Accessible_child.ref == 2, "Unexpected refcnt %ld\n", Accessible_child.ref);
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible_child.ref == 1, "Unexpected refcnt %ld\n", Accessible_child.ref);

    /* Return an HWND from OleWindow, not accNavigate. */
    Accessible.acc_hwnd = NULL;
    Accessible.ow_hwnd = hwnd;
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    hr = IRawElementProviderSimple_get_ProviderOptions(elprov, &prov_opt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok((prov_opt == (ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading)) ||
            broken(prov_opt == ProviderOptions_ClientSideProvider), /* Windows < 10 1507 */
            "Unexpected provider options %#x\n", prov_opt);

    test_implements_interface(elprov, &IID_IRawElementProviderFragmentRoot, TRUE);

    hr = IRawElementProviderSimple_GetPropertyValue(elprov, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_BSTR, "V_VT(&v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hr = IRawElementProviderSimple_GetPatternProvider(elprov, UIA_LegacyIAccessiblePatternId, &unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!unk, "unk == NULL\n");
    ok(iface_cmp((IUnknown *)elprov, unk), "unk != elprov\n");

    hr = IUnknown_QueryInterface(unk, &IID_ILegacyIAccessibleProvider, (void **)&accprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!accprov, "accprov == NULL\n");

    hr = ILegacyIAccessibleProvider_get_ChildId(accprov, &cid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(cid == CHILDID_SELF, "cid != CHILDID_SELF\n");

    hr = ILegacyIAccessibleProvider_GetIAccessible(accprov, &acc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(acc == &Accessible.IAccessible_iface, "acc != &Accessible.IAccessible_iface\n");
    IAccessible_Release(acc);
    IUnknown_Release(unk);
    ILegacyIAccessibleProvider_Release(accprov);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* ChildID other than CHILDID_SELF. */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, 1, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Even simple children implement IRawElementProviderFragmentRoot. */
    test_implements_interface(elprov, &IID_IRawElementProviderFragmentRoot, TRUE);

    /*
     * Simple child element (IAccessible without CHILDID_SELF) cannot be root
     * IAccessible. No checks against the root HWND IAccessible will be done.
     */
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL\n");

    hr = IRawElementProviderSimple_GetPatternProvider(elprov, UIA_LegacyIAccessiblePatternId, &unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!unk, "unk == NULL\n");
    ok(iface_cmp((IUnknown *)elprov, unk), "unk != elprov\n");

    hr = IUnknown_QueryInterface(unk, &IID_ILegacyIAccessibleProvider, (void **)&accprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!accprov, "accprov == NULL\n");

    hr = ILegacyIAccessibleProvider_get_ChildId(accprov, &cid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(cid == 1, "cid != CHILDID_SELF\n");

    hr = ILegacyIAccessibleProvider_GetIAccessible(accprov, &acc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(acc == &Accessible.IAccessible_iface, "acc != &Accessible.IAccessible_iface\n");
    IAccessible_Release(acc);
    IUnknown_Release(unk);
    ILegacyIAccessibleProvider_Release(accprov);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * &Accessible.IAccessible_iface will be compared against the default
     * client accessible object. Since we have all properties set to 0,
     * we return failure HRESULTs and all properties will get queried but not
     * compared.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accName);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL\n");
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accName);

    /*
     * Interface that isn't the HWND root, still implements
     * IRawElementProviderFragmentRoot.
     */
    test_implements_interface(elprov, &IID_IRawElementProviderFragmentRoot, TRUE);

    /* Second call won't send WM_GETOBJECT. */
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL\n");

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Return &Accessible.IAccessible_iface in response to OBJID_CLIENT,
     * interface pointers will be compared, no method calls to check property
     * values.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    elprov2 = (void *)0xdeadbeef;
    acc_client = &Accessible.IAccessible_iface;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    /* Second call, no checks. */
    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Return &Accessible2.IAccessible_iface in response to OBJID_CLIENT,
     * interface pointers won't match, so properties will be compared.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1,
            L"acc_name", 0, 0, 50, 50);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1,
            L"acc_name", 0, 0, 50, 50);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accName);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible2, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accName);
    /*
     * The IAccessible returned by WM_GETOBJECT will be checked for an
     * IAccIdentity interface to see if Dynamic Annotation properties should
     * be queried. If not present on the current IAccessible, it will check
     * the parent IAccessible for one.
     */
    SET_ACC_METHOD_EXPECT(&Accessible2, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accName);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible2, accLocation);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accName);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, QI_IAccIdentity);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, get_accParent);
    IRawElementProviderSimple_Release(elprov2);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * If a failure HRESULT is returned from the IRawElementProviderSimple
     * IAccessible, the corresponding AOFW IAccessible method isn't called.
     * An exception is get_accChildCount, which is always called, but only
     * checked if the HRESULT return value is not a failure. If Role/State/Name
     * are not queried, no IAccIdentity check is done.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, 0, 0, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1,
            L"acc_name", 0, 0, 50, 50);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accName);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accName);

    acc_client = NULL;
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Properties are checked in a sequence of accRole, accState,
     * accChildCount, accLocation, and finally accName. If a mismatch is found
     * early in the sequence, the rest aren't checked.
     */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_CLIENT, STATE_SYSTEM_FOCUSABLE, 0, NULL, 0, 0, 0, 0);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible2, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accRole);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, QI_IAccIdentity);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, get_accParent);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* 4/5 properties match, considered a match. */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1, NULL, 0, 0, 50, 50);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1, NULL, 0, 0, 50, 50);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accName);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible2, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible2, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accName);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible2, accLocation);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, QI_IAccIdentity);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, get_accParent);
    IRawElementProviderSimple_Release(elprov2);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* 3/5 properties match, not considered a match. */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1, NULL, 0, 0, 0, 0);
    set_accessible_props(&Accessible2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 1, NULL, 0, 0, 0, 0);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accName);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible2, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accName);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accChildCount);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, QI_IAccIdentity);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, get_accParent);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!elprov2, "elprov != NULL, elprov %p\n", elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Only name matches, considered a match. */
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    set_accessible_props(&Accessible, 0, 0, 0, L"acc_name", 0, 0, 0, 0);
    set_accessible_props(&Accessible2, 0, 0, 0, L"acc_name", 0, 0, 0, 0);

    acc_client = &Accessible2.IAccessible_iface;
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accName);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accChildCount);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accName);
    SET_ACC_METHOD_EXPECT(&Accessible2, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible2, get_accParent);
    elprov2 = (void *)0xdeadbeef;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    ok(Accessible2.ref == 1, "Unexpected refcnt %ld\n", Accessible2.ref);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accName);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accChildCount);
    CHECK_ACC_METHOD_CALLED(&Accessible2, get_accName);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, QI_IAccIdentity);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible2, get_accParent);
    IRawElementProviderSimple_Release(elprov2);

    elprov2 = (void *)0xdeadbeef;
    acc_client = NULL;
    hr = IRawElementProviderSimple_get_HostRawElementProvider(elprov, &elprov2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elprov2, "elprov == NULL, elprov %p\n", elprov2);
    IRawElementProviderSimple_Release(elprov2);

    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Test IRawElementProviderFragment_get_BoundingRectangle.
     */
    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSABLE, 0, L"acc_name", 25, 25, 50, 50);
    /* Test the case where Accessible is not the root for its HWND. */
    acc_client = NULL;
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");

    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible, accLocation);
    hr = IRawElementProviderFragment_get_BoundingRectangle(elfrag, &rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rect.left == (double)Accessible.left, "Unexpected left value %f\n", rect.left);
    ok(rect.top == (double)Accessible.top, "Unexpected top value %f\n", rect.top);
    ok(rect.width == (double)Accessible.width, "Unexpected width value %f\n", rect.width);
    ok(rect.height == (double)Accessible.height, "Unexpected height value %f\n", rect.height);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_ACC_METHOD_CALLED(&Accessible, accLocation);

    /* If Accessible has STATE_SYSTEM_OFFSCREEN, it will return an empty rect. */
    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_OFFSCREEN, 0, L"acc_name", 0, 0, 50, 50);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    hr = IRawElementProviderFragment_get_BoundingRectangle(elfrag, &rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rect.left == 0.0, "Unexpected left value %f\n", rect.left);
    ok(rect.top == 0.0, "Unexpected top value %f\n", rect.top);
    ok(rect.width == 0.0, "Unexpected width value %f\n", rect.width);
    ok(rect.height == 0.0, "Unexpected height value %f\n", rect.height);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);

    IRawElementProviderFragment_Release(elfrag);
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /* Test case where accessible is the root accessible. */
    set_accessible_props(&Accessible, ROLE_SYSTEM_DOCUMENT, 0, 0, L"acc_name", 0, 0, 0, 0);
    acc_client = &Accessible.IAccessible_iface;
    hr = pUiaProviderFromIAccessible(&Accessible.IAccessible_iface, CHILDID_SELF, UIA_PFIA_DEFAULT, &elprov);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Accessible.ref == 2, "Unexpected refcnt %ld\n", Accessible.ref);

    hr = IRawElementProviderSimple_QueryInterface(elprov, &IID_IRawElementProviderFragment, (void **)&elfrag);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elfrag, "elfrag == NULL\n");

    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = IRawElementProviderFragment_get_BoundingRectangle(elfrag, &rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rect.left == 0.0, "Unexpected left value %f\n", rect.left);
    ok(rect.top == 0.0, "Unexpected top value %f\n", rect.top);
    ok(rect.width == 0.0, "Unexpected width value %f\n", rect.width);
    ok(rect.height == 0.0, "Unexpected height value %f\n", rect.height);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    /* Second call does nothing. */
    hr = IRawElementProviderFragment_get_BoundingRectangle(elfrag, &rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rect.left == 0.0, "Unexpected left value %f\n", rect.left);
    ok(rect.top == 0.0, "Unexpected top value %f\n", rect.top);
    ok(rect.width == 0.0, "Unexpected width value %f\n", rect.width);
    ok(rect.height == 0.0, "Unexpected height value %f\n", rect.height);

    IRawElementProviderFragment_Release(elfrag);
    IRawElementProviderSimple_Release(elprov);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);
    acc_client = NULL;

    test_uia_prov_from_acc_properties();
    test_uia_prov_from_acc_navigation();
    test_uia_prov_from_acc_ia2();
    test_uia_prov_from_acc_fragment_root(hwnd);
    test_uia_prov_from_acc_winevent_handler(hwnd);

    CoUninitialize();
    DestroyWindow(hwnd);
    UnregisterClassA("pUiaProviderFromIAccessible class", NULL);
    Accessible.acc_hwnd = NULL;
    Accessible.ow_hwnd = NULL;
}

struct uia_lookup_id {
    const GUID *guid;
    int id;
};

static const struct uia_lookup_id uia_property_lookup_ids[] = {
    { &RuntimeId_Property_GUID,                           UIA_RuntimeIdPropertyId },
    { &BoundingRectangle_Property_GUID,                   UIA_BoundingRectanglePropertyId },
    { &ProcessId_Property_GUID,                           UIA_ProcessIdPropertyId },
    { &ControlType_Property_GUID,                         UIA_ControlTypePropertyId },
    { &LocalizedControlType_Property_GUID,                UIA_LocalizedControlTypePropertyId },
    { &Name_Property_GUID,                                UIA_NamePropertyId },
    { &AcceleratorKey_Property_GUID,                      UIA_AcceleratorKeyPropertyId },
    { &AccessKey_Property_GUID,                           UIA_AccessKeyPropertyId },
    { &HasKeyboardFocus_Property_GUID,                    UIA_HasKeyboardFocusPropertyId },
    { &IsKeyboardFocusable_Property_GUID,                 UIA_IsKeyboardFocusablePropertyId },
    { &IsEnabled_Property_GUID,                           UIA_IsEnabledPropertyId },
    { &AutomationId_Property_GUID,                        UIA_AutomationIdPropertyId },
    { &ClassName_Property_GUID,                           UIA_ClassNamePropertyId },
    { &HelpText_Property_GUID,                            UIA_HelpTextPropertyId },
    { &ClickablePoint_Property_GUID,                      UIA_ClickablePointPropertyId },
    { &Culture_Property_GUID,                             UIA_CulturePropertyId },
    { &IsControlElement_Property_GUID,                    UIA_IsControlElementPropertyId },
    { &IsContentElement_Property_GUID,                    UIA_IsContentElementPropertyId },
    { &LabeledBy_Property_GUID,                           UIA_LabeledByPropertyId },
    { &IsPassword_Property_GUID,                          UIA_IsPasswordPropertyId },
    { &NewNativeWindowHandle_Property_GUID,               UIA_NativeWindowHandlePropertyId },
    { &ItemType_Property_GUID,                            UIA_ItemTypePropertyId },
    { &IsOffscreen_Property_GUID,                         UIA_IsOffscreenPropertyId },
    { &Orientation_Property_GUID,                         UIA_OrientationPropertyId },
    { &FrameworkId_Property_GUID,                         UIA_FrameworkIdPropertyId },
    { &IsRequiredForForm_Property_GUID,                   UIA_IsRequiredForFormPropertyId },
    { &ItemStatus_Property_GUID,                          UIA_ItemStatusPropertyId },
    { &IsDockPatternAvailable_Property_GUID,              UIA_IsDockPatternAvailablePropertyId },
    { &IsExpandCollapsePatternAvailable_Property_GUID,    UIA_IsExpandCollapsePatternAvailablePropertyId },
    { &IsGridItemPatternAvailable_Property_GUID,          UIA_IsGridItemPatternAvailablePropertyId },
    { &IsGridPatternAvailable_Property_GUID,              UIA_IsGridPatternAvailablePropertyId },
    { &IsInvokePatternAvailable_Property_GUID,            UIA_IsInvokePatternAvailablePropertyId },
    { &IsMultipleViewPatternAvailable_Property_GUID,      UIA_IsMultipleViewPatternAvailablePropertyId },
    { &IsRangeValuePatternAvailable_Property_GUID,        UIA_IsRangeValuePatternAvailablePropertyId },
    { &IsScrollPatternAvailable_Property_GUID,            UIA_IsScrollPatternAvailablePropertyId },
    { &IsScrollItemPatternAvailable_Property_GUID,        UIA_IsScrollItemPatternAvailablePropertyId },
    { &IsSelectionItemPatternAvailable_Property_GUID,     UIA_IsSelectionItemPatternAvailablePropertyId },
    { &IsSelectionPatternAvailable_Property_GUID,         UIA_IsSelectionPatternAvailablePropertyId },
    { &IsTablePatternAvailable_Property_GUID,             UIA_IsTablePatternAvailablePropertyId },
    { &IsTableItemPatternAvailable_Property_GUID,         UIA_IsTableItemPatternAvailablePropertyId },
    { &IsTextPatternAvailable_Property_GUID,              UIA_IsTextPatternAvailablePropertyId },
    { &IsTogglePatternAvailable_Property_GUID,            UIA_IsTogglePatternAvailablePropertyId },
    { &IsTransformPatternAvailable_Property_GUID,         UIA_IsTransformPatternAvailablePropertyId },
    { &IsValuePatternAvailable_Property_GUID,             UIA_IsValuePatternAvailablePropertyId },
    { &IsWindowPatternAvailable_Property_GUID,            UIA_IsWindowPatternAvailablePropertyId },
    { &Value_Value_Property_GUID,                         UIA_ValueValuePropertyId },
    { &Value_IsReadOnly_Property_GUID,                    UIA_ValueIsReadOnlyPropertyId },
    { &RangeValue_Value_Property_GUID,                    UIA_RangeValueValuePropertyId },
    { &RangeValue_IsReadOnly_Property_GUID,               UIA_RangeValueIsReadOnlyPropertyId },
    { &RangeValue_Minimum_Property_GUID,                  UIA_RangeValueMinimumPropertyId },
    { &RangeValue_Maximum_Property_GUID,                  UIA_RangeValueMaximumPropertyId },
    { &RangeValue_LargeChange_Property_GUID,              UIA_RangeValueLargeChangePropertyId },
    { &RangeValue_SmallChange_Property_GUID,              UIA_RangeValueSmallChangePropertyId },
    { &Scroll_HorizontalScrollPercent_Property_GUID,      UIA_ScrollHorizontalScrollPercentPropertyId },
    { &Scroll_HorizontalViewSize_Property_GUID,           UIA_ScrollHorizontalViewSizePropertyId },
    { &Scroll_VerticalScrollPercent_Property_GUID,        UIA_ScrollVerticalScrollPercentPropertyId },
    { &Scroll_VerticalViewSize_Property_GUID,             UIA_ScrollVerticalViewSizePropertyId },
    { &Scroll_HorizontallyScrollable_Property_GUID,       UIA_ScrollHorizontallyScrollablePropertyId },
    { &Scroll_VerticallyScrollable_Property_GUID,         UIA_ScrollVerticallyScrollablePropertyId },
    { &Selection_Selection_Property_GUID,                 UIA_SelectionSelectionPropertyId },
    { &Selection_CanSelectMultiple_Property_GUID,         UIA_SelectionCanSelectMultiplePropertyId },
    { &Selection_IsSelectionRequired_Property_GUID,       UIA_SelectionIsSelectionRequiredPropertyId },
    { &Grid_RowCount_Property_GUID,                       UIA_GridRowCountPropertyId },
    { &Grid_ColumnCount_Property_GUID,                    UIA_GridColumnCountPropertyId },
    { &GridItem_Row_Property_GUID,                        UIA_GridItemRowPropertyId },
    { &GridItem_Column_Property_GUID,                     UIA_GridItemColumnPropertyId },
    { &GridItem_RowSpan_Property_GUID,                    UIA_GridItemRowSpanPropertyId },
    { &GridItem_ColumnSpan_Property_GUID,                 UIA_GridItemColumnSpanPropertyId },
    { &GridItem_Parent_Property_GUID,                     UIA_GridItemContainingGridPropertyId },
    { &Dock_DockPosition_Property_GUID,                   UIA_DockDockPositionPropertyId },
    { &ExpandCollapse_ExpandCollapseState_Property_GUID,  UIA_ExpandCollapseExpandCollapseStatePropertyId },
    { &MultipleView_CurrentView_Property_GUID,            UIA_MultipleViewCurrentViewPropertyId },
    { &MultipleView_SupportedViews_Property_GUID,         UIA_MultipleViewSupportedViewsPropertyId },
    { &Window_CanMaximize_Property_GUID,                  UIA_WindowCanMaximizePropertyId },
    { &Window_CanMinimize_Property_GUID,                  UIA_WindowCanMinimizePropertyId },
    { &Window_WindowVisualState_Property_GUID,            UIA_WindowWindowVisualStatePropertyId },
    { &Window_WindowInteractionState_Property_GUID,       UIA_WindowWindowInteractionStatePropertyId },
    { &Window_IsModal_Property_GUID,                      UIA_WindowIsModalPropertyId },
    { &Window_IsTopmost_Property_GUID,                    UIA_WindowIsTopmostPropertyId },
    { &SelectionItem_IsSelected_Property_GUID,            UIA_SelectionItemIsSelectedPropertyId },
    { &SelectionItem_SelectionContainer_Property_GUID,    UIA_SelectionItemSelectionContainerPropertyId },
    { &Table_RowHeaders_Property_GUID,                    UIA_TableRowHeadersPropertyId },
    { &Table_ColumnHeaders_Property_GUID,                 UIA_TableColumnHeadersPropertyId },
    { &Table_RowOrColumnMajor_Property_GUID,              UIA_TableRowOrColumnMajorPropertyId },
    { &TableItem_RowHeaderItems_Property_GUID,            UIA_TableItemRowHeaderItemsPropertyId },
    { &TableItem_ColumnHeaderItems_Property_GUID,         UIA_TableItemColumnHeaderItemsPropertyId },
    { &Toggle_ToggleState_Property_GUID,                  UIA_ToggleToggleStatePropertyId },
    { &Transform_CanMove_Property_GUID,                   UIA_TransformCanMovePropertyId },
    { &Transform_CanResize_Property_GUID,                 UIA_TransformCanResizePropertyId },
    { &Transform_CanRotate_Property_GUID,                 UIA_TransformCanRotatePropertyId },
    { &IsLegacyIAccessiblePatternAvailable_Property_GUID, UIA_IsLegacyIAccessiblePatternAvailablePropertyId },
    { &LegacyIAccessible_ChildId_Property_GUID,           UIA_LegacyIAccessibleChildIdPropertyId },
    { &LegacyIAccessible_Name_Property_GUID,              UIA_LegacyIAccessibleNamePropertyId },
    { &LegacyIAccessible_Value_Property_GUID,             UIA_LegacyIAccessibleValuePropertyId },
    { &LegacyIAccessible_Description_Property_GUID,       UIA_LegacyIAccessibleDescriptionPropertyId },
    { &LegacyIAccessible_Role_Property_GUID,              UIA_LegacyIAccessibleRolePropertyId },
    { &LegacyIAccessible_State_Property_GUID,             UIA_LegacyIAccessibleStatePropertyId },
    { &LegacyIAccessible_Help_Property_GUID,              UIA_LegacyIAccessibleHelpPropertyId },
    { &LegacyIAccessible_KeyboardShortcut_Property_GUID,  UIA_LegacyIAccessibleKeyboardShortcutPropertyId },
    { &LegacyIAccessible_Selection_Property_GUID,         UIA_LegacyIAccessibleSelectionPropertyId },
    { &LegacyIAccessible_DefaultAction_Property_GUID,     UIA_LegacyIAccessibleDefaultActionPropertyId },
    { &AriaRole_Property_GUID,                            UIA_AriaRolePropertyId },
    { &AriaProperties_Property_GUID,                      UIA_AriaPropertiesPropertyId },
    { &IsDataValidForForm_Property_GUID,                  UIA_IsDataValidForFormPropertyId },
    { &ControllerFor_Property_GUID,                       UIA_ControllerForPropertyId },
    { &DescribedBy_Property_GUID,                         UIA_DescribedByPropertyId },
    { &FlowsTo_Property_GUID,                             UIA_FlowsToPropertyId },
    { &ProviderDescription_Property_GUID,                 UIA_ProviderDescriptionPropertyId },
    { &IsItemContainerPatternAvailable_Property_GUID,     UIA_IsItemContainerPatternAvailablePropertyId },
    { &IsVirtualizedItemPatternAvailable_Property_GUID,   UIA_IsVirtualizedItemPatternAvailablePropertyId },
    { &IsSynchronizedInputPatternAvailable_Property_GUID, UIA_IsSynchronizedInputPatternAvailablePropertyId },
    /* Implemented on Win8+ */
    { &OptimizeForVisualContent_Property_GUID,            UIA_OptimizeForVisualContentPropertyId },
    { &IsObjectModelPatternAvailable_Property_GUID,       UIA_IsObjectModelPatternAvailablePropertyId },
    { &Annotation_AnnotationTypeId_Property_GUID,         UIA_AnnotationAnnotationTypeIdPropertyId },
    { &Annotation_AnnotationTypeName_Property_GUID,       UIA_AnnotationAnnotationTypeNamePropertyId },
    { &Annotation_Author_Property_GUID,                   UIA_AnnotationAuthorPropertyId },
    { &Annotation_DateTime_Property_GUID,                 UIA_AnnotationDateTimePropertyId },
    { &Annotation_Target_Property_GUID,                   UIA_AnnotationTargetPropertyId },
    { &IsAnnotationPatternAvailable_Property_GUID,        UIA_IsAnnotationPatternAvailablePropertyId },
    { &IsTextPattern2Available_Property_GUID,             UIA_IsTextPattern2AvailablePropertyId },
    { &Styles_StyleId_Property_GUID,                      UIA_StylesStyleIdPropertyId },
    { &Styles_StyleName_Property_GUID,                    UIA_StylesStyleNamePropertyId },
    { &Styles_FillColor_Property_GUID,                    UIA_StylesFillColorPropertyId },
    { &Styles_FillPatternStyle_Property_GUID,             UIA_StylesFillPatternStylePropertyId },
    { &Styles_Shape_Property_GUID,                        UIA_StylesShapePropertyId },
    { &Styles_FillPatternColor_Property_GUID,             UIA_StylesFillPatternColorPropertyId },
    { &Styles_ExtendedProperties_Property_GUID,           UIA_StylesExtendedPropertiesPropertyId },
    { &IsStylesPatternAvailable_Property_GUID,            UIA_IsStylesPatternAvailablePropertyId },
    { &IsSpreadsheetPatternAvailable_Property_GUID,       UIA_IsSpreadsheetPatternAvailablePropertyId },
    { &SpreadsheetItem_Formula_Property_GUID,             UIA_SpreadsheetItemFormulaPropertyId },
    { &SpreadsheetItem_AnnotationObjects_Property_GUID,   UIA_SpreadsheetItemAnnotationObjectsPropertyId },
    { &SpreadsheetItem_AnnotationTypes_Property_GUID,     UIA_SpreadsheetItemAnnotationTypesPropertyId },
    { &IsSpreadsheetItemPatternAvailable_Property_GUID,   UIA_IsSpreadsheetItemPatternAvailablePropertyId },
    { &Transform2_CanZoom_Property_GUID,                  UIA_Transform2CanZoomPropertyId },
    { &IsTransformPattern2Available_Property_GUID,        UIA_IsTransformPattern2AvailablePropertyId },
    { &LiveSetting_Property_GUID,                         UIA_LiveSettingPropertyId },
    { &IsTextChildPatternAvailable_Property_GUID,         UIA_IsTextChildPatternAvailablePropertyId },
    { &IsDragPatternAvailable_Property_GUID,              UIA_IsDragPatternAvailablePropertyId },
    { &Drag_IsGrabbed_Property_GUID,                      UIA_DragIsGrabbedPropertyId },
    { &Drag_DropEffect_Property_GUID,                     UIA_DragDropEffectPropertyId },
    { &Drag_DropEffects_Property_GUID,                    UIA_DragDropEffectsPropertyId },
    { &IsDropTargetPatternAvailable_Property_GUID,        UIA_IsDropTargetPatternAvailablePropertyId },
    { &DropTarget_DropTargetEffect_Property_GUID,         UIA_DropTargetDropTargetEffectPropertyId },
    { &DropTarget_DropTargetEffects_Property_GUID,        UIA_DropTargetDropTargetEffectsPropertyId },
    { &Drag_GrabbedItems_Property_GUID,                   UIA_DragGrabbedItemsPropertyId },
    { &Transform2_ZoomLevel_Property_GUID,                UIA_Transform2ZoomLevelPropertyId },
    { &Transform2_ZoomMinimum_Property_GUID,              UIA_Transform2ZoomMinimumPropertyId },
    { &Transform2_ZoomMaximum_Property_GUID,              UIA_Transform2ZoomMaximumPropertyId },
    { &FlowsFrom_Property_GUID,                           UIA_FlowsFromPropertyId },
    { &IsTextEditPatternAvailable_Property_GUID,          UIA_IsTextEditPatternAvailablePropertyId },
    { &IsPeripheral_Property_GUID,                        UIA_IsPeripheralPropertyId },
    /* Implemented on Win10v1507+. */
    { &IsCustomNavigationPatternAvailable_Property_GUID,  UIA_IsCustomNavigationPatternAvailablePropertyId },
    { &PositionInSet_Property_GUID,                       UIA_PositionInSetPropertyId },
    { &SizeOfSet_Property_GUID,                           UIA_SizeOfSetPropertyId },
    { &Level_Property_GUID,                               UIA_LevelPropertyId },
    { &AnnotationTypes_Property_GUID,                     UIA_AnnotationTypesPropertyId },
    { &AnnotationObjects_Property_GUID,                   UIA_AnnotationObjectsPropertyId },
    /* Implemented on Win10v1809+. */
    { &LandmarkType_Property_GUID,                        UIA_LandmarkTypePropertyId },
    { &LocalizedLandmarkType_Property_GUID,               UIA_LocalizedLandmarkTypePropertyId },
    { &FullDescription_Property_GUID,                     UIA_FullDescriptionPropertyId },
    { &FillColor_Property_GUID,                           UIA_FillColorPropertyId },
    { &OutlineColor_Property_GUID,                        UIA_OutlineColorPropertyId },
    { &FillType_Property_GUID,                            UIA_FillTypePropertyId },
    { &VisualEffects_Property_GUID,                       UIA_VisualEffectsPropertyId },
    { &OutlineThickness_Property_GUID,                    UIA_OutlineThicknessPropertyId },
    { &CenterPoint_Property_GUID,                         UIA_CenterPointPropertyId },
    { &Rotation_Property_GUID,                            UIA_RotationPropertyId },
    { &Size_Property_GUID,                                UIA_SizePropertyId },
    { &IsSelectionPattern2Available_Property_GUID,        UIA_IsSelectionPattern2AvailablePropertyId },
    { &Selection2_FirstSelectedItem_Property_GUID,        UIA_Selection2FirstSelectedItemPropertyId },
    { &Selection2_LastSelectedItem_Property_GUID,         UIA_Selection2LastSelectedItemPropertyId },
    { &Selection2_CurrentSelectedItem_Property_GUID,      UIA_Selection2CurrentSelectedItemPropertyId },
    { &Selection2_ItemCount_Property_GUID,                UIA_Selection2ItemCountPropertyId },
    { &HeadingLevel_Property_GUID,                        UIA_HeadingLevelPropertyId },
    { &IsDialog_Property_GUID,                            UIA_IsDialogPropertyId },
};

static const struct uia_lookup_id uia_event_lookup_ids[] = {
    { &ToolTipOpened_Event_GUID,                                  UIA_ToolTipOpenedEventId },
    { &ToolTipClosed_Event_GUID,                                  UIA_ToolTipClosedEventId },
    { &StructureChanged_Event_GUID,                               UIA_StructureChangedEventId },
    { &MenuOpened_Event_GUID,                                     UIA_MenuOpenedEventId },
    { &AutomationPropertyChanged_Event_GUID,                      UIA_AutomationPropertyChangedEventId },
    { &AutomationFocusChanged_Event_GUID,                         UIA_AutomationFocusChangedEventId },
    { &AsyncContentLoaded_Event_GUID,                             UIA_AsyncContentLoadedEventId },
    { &MenuClosed_Event_GUID,                                     UIA_MenuClosedEventId },
    { &LayoutInvalidated_Event_GUID,                              UIA_LayoutInvalidatedEventId },
    { &Invoke_Invoked_Event_GUID,                                 UIA_Invoke_InvokedEventId },
    { &SelectionItem_ElementAddedToSelectionEvent_Event_GUID,     UIA_SelectionItem_ElementAddedToSelectionEventId },
    { &SelectionItem_ElementRemovedFromSelectionEvent_Event_GUID, UIA_SelectionItem_ElementRemovedFromSelectionEventId },
    { &SelectionItem_ElementSelectedEvent_Event_GUID,             UIA_SelectionItem_ElementSelectedEventId },
    { &Selection_InvalidatedEvent_Event_GUID,                     UIA_Selection_InvalidatedEventId },
    { &Text_TextSelectionChangedEvent_Event_GUID,                 UIA_Text_TextSelectionChangedEventId },
    { &Text_TextChangedEvent_Event_GUID,                          UIA_Text_TextChangedEventId },
    { &Window_WindowOpened_Event_GUID,                            UIA_Window_WindowOpenedEventId },
    { &Window_WindowClosed_Event_GUID,                            UIA_Window_WindowClosedEventId },
    { &MenuModeStart_Event_GUID,                                  UIA_MenuModeStartEventId },
    { &MenuModeEnd_Event_GUID,                                    UIA_MenuModeEndEventId },
    { &InputReachedTarget_Event_GUID,                             UIA_InputReachedTargetEventId },
    { &InputReachedOtherElement_Event_GUID,                       UIA_InputReachedOtherElementEventId },
    { &InputDiscarded_Event_GUID,                                 UIA_InputDiscardedEventId },
    /* Implemented on Win8+ */
    { &SystemAlert_Event_GUID,                                    UIA_SystemAlertEventId },
    { &LiveRegionChanged_Event_GUID,                              UIA_LiveRegionChangedEventId },
    { &HostedFragmentRootsInvalidated_Event_GUID,                 UIA_HostedFragmentRootsInvalidatedEventId },
    { &Drag_DragStart_Event_GUID,                                 UIA_Drag_DragStartEventId },
    { &Drag_DragCancel_Event_GUID,                                UIA_Drag_DragCancelEventId },
    { &Drag_DragComplete_Event_GUID,                              UIA_Drag_DragCompleteEventId },
    { &DropTarget_DragEnter_Event_GUID,                           UIA_DropTarget_DragEnterEventId },
    { &DropTarget_DragLeave_Event_GUID,                           UIA_DropTarget_DragLeaveEventId },
    { &DropTarget_Dropped_Event_GUID,                             UIA_DropTarget_DroppedEventId },
    { &TextEdit_TextChanged_Event_GUID,                           UIA_TextEdit_TextChangedEventId },
    { &TextEdit_ConversionTargetChanged_Event_GUID,               UIA_TextEdit_ConversionTargetChangedEventId },
    /* Implemented on Win10v1809+. */
    { &Changes_Event_GUID,                                        UIA_ChangesEventId },
    { &Notification_Event_GUID,                                   UIA_NotificationEventId },
};

static const struct uia_lookup_id uia_pattern_lookup_ids[] = {
    { &Invoke_Pattern_GUID,            UIA_InvokePatternId },
    { &Selection_Pattern_GUID,         UIA_SelectionPatternId },
    { &Value_Pattern_GUID,             UIA_ValuePatternId },
    { &RangeValue_Pattern_GUID,        UIA_RangeValuePatternId },
    { &Scroll_Pattern_GUID,            UIA_ScrollPatternId },
    { &ExpandCollapse_Pattern_GUID,    UIA_ExpandCollapsePatternId },
    { &Grid_Pattern_GUID,              UIA_GridPatternId },
    { &GridItem_Pattern_GUID,          UIA_GridItemPatternId },
    { &MultipleView_Pattern_GUID,      UIA_MultipleViewPatternId },
    { &Window_Pattern_GUID,            UIA_WindowPatternId },
    { &SelectionItem_Pattern_GUID,     UIA_SelectionItemPatternId },
    { &Dock_Pattern_GUID,              UIA_DockPatternId },
    { &Table_Pattern_GUID,             UIA_TablePatternId },
    { &TableItem_Pattern_GUID,         UIA_TableItemPatternId },
    { &Text_Pattern_GUID,              UIA_TextPatternId },
    { &Toggle_Pattern_GUID,            UIA_TogglePatternId },
    { &Transform_Pattern_GUID,         UIA_TransformPatternId },
    { &ScrollItem_Pattern_GUID,        UIA_ScrollItemPatternId },
    { &LegacyIAccessible_Pattern_GUID, UIA_LegacyIAccessiblePatternId },
    { &ItemContainer_Pattern_GUID,     UIA_ItemContainerPatternId },
    { &VirtualizedItem_Pattern_GUID,   UIA_VirtualizedItemPatternId },
    { &SynchronizedInput_Pattern_GUID, UIA_SynchronizedInputPatternId },
    /* Implemented on Win8+ */
    { &ObjectModel_Pattern_GUID,       UIA_ObjectModelPatternId },
    { &Annotation_Pattern_GUID,        UIA_AnnotationPatternId },
    { &Text_Pattern2_GUID,             UIA_TextPattern2Id },
    { &Styles_Pattern_GUID,            UIA_StylesPatternId },
    { &Spreadsheet_Pattern_GUID,       UIA_SpreadsheetPatternId },
    { &SpreadsheetItem_Pattern_GUID,   UIA_SpreadsheetItemPatternId },
    { &Tranform_Pattern2_GUID,         UIA_TransformPattern2Id },
    { &TextChild_Pattern_GUID,         UIA_TextChildPatternId },
    { &Drag_Pattern_GUID,              UIA_DragPatternId },
    { &DropTarget_Pattern_GUID,        UIA_DropTargetPatternId },
    { &TextEdit_Pattern_GUID,          UIA_TextEditPatternId },
    /* Implemented on Win10+. */
    { &CustomNavigation_Pattern_GUID,  UIA_CustomNavigationPatternId },
};

static const struct uia_lookup_id uia_control_type_lookup_ids[] = {
    { &Button_Control_GUID,       UIA_ButtonControlTypeId },
    { &Calendar_Control_GUID,     UIA_CalendarControlTypeId },
    { &CheckBox_Control_GUID,     UIA_CheckBoxControlTypeId },
    { &ComboBox_Control_GUID,     UIA_ComboBoxControlTypeId },
    { &Edit_Control_GUID,         UIA_EditControlTypeId },
    { &Hyperlink_Control_GUID,    UIA_HyperlinkControlTypeId },
    { &Image_Control_GUID,        UIA_ImageControlTypeId },
    { &ListItem_Control_GUID,     UIA_ListItemControlTypeId },
    { &List_Control_GUID,         UIA_ListControlTypeId },
    { &Menu_Control_GUID,         UIA_MenuControlTypeId },
    { &MenuBar_Control_GUID,      UIA_MenuBarControlTypeId },
    { &MenuItem_Control_GUID,     UIA_MenuItemControlTypeId },
    { &ProgressBar_Control_GUID,  UIA_ProgressBarControlTypeId },
    { &RadioButton_Control_GUID,  UIA_RadioButtonControlTypeId },
    { &ScrollBar_Control_GUID,    UIA_ScrollBarControlTypeId },
    { &Slider_Control_GUID,       UIA_SliderControlTypeId },
    { &Spinner_Control_GUID,      UIA_SpinnerControlTypeId },
    { &StatusBar_Control_GUID,    UIA_StatusBarControlTypeId },
    { &Tab_Control_GUID,          UIA_TabControlTypeId },
    { &TabItem_Control_GUID,      UIA_TabItemControlTypeId },
    { &Text_Control_GUID,         UIA_TextControlTypeId },
    { &ToolBar_Control_GUID,      UIA_ToolBarControlTypeId },
    { &ToolTip_Control_GUID,      UIA_ToolTipControlTypeId },
    { &Tree_Control_GUID,         UIA_TreeControlTypeId },
    { &TreeItem_Control_GUID,     UIA_TreeItemControlTypeId },
    { &Custom_Control_GUID,       UIA_CustomControlTypeId },
    { &Group_Control_GUID,        UIA_GroupControlTypeId },
    { &Thumb_Control_GUID,        UIA_ThumbControlTypeId },
    { &DataGrid_Control_GUID,     UIA_DataGridControlTypeId },
    { &DataItem_Control_GUID,     UIA_DataItemControlTypeId },
    { &Document_Control_GUID,     UIA_DocumentControlTypeId },
    { &SplitButton_Control_GUID,  UIA_SplitButtonControlTypeId },
    { &Window_Control_GUID,       UIA_WindowControlTypeId },
    { &Pane_Control_GUID,         UIA_PaneControlTypeId },
    { &Header_Control_GUID,       UIA_HeaderControlTypeId },
    { &HeaderItem_Control_GUID,   UIA_HeaderItemControlTypeId },
    { &Table_Control_GUID,        UIA_TableControlTypeId },
    { &TitleBar_Control_GUID,     UIA_TitleBarControlTypeId },
    { &Separator_Control_GUID,    UIA_SeparatorControlTypeId },
    /* Implemented on Win8+ */
    { &SemanticZoom_Control_GUID, UIA_SemanticZoomControlTypeId },
    { &AppBar_Control_GUID,       UIA_AppBarControlTypeId },
};

static void test_UiaLookupId(void)
{
    static const struct {
        const char *id_type_name;
        int id_type;
        const struct uia_lookup_id *ids;
        int ids_count;
    } tests[] =
    {
        { "property", AutomationIdentifierType_Property, uia_property_lookup_ids, ARRAY_SIZE(uia_property_lookup_ids) },
        { "event",    AutomationIdentifierType_Event,    uia_event_lookup_ids,    ARRAY_SIZE(uia_event_lookup_ids) },
        { "pattern",  AutomationIdentifierType_Pattern,  uia_pattern_lookup_ids,  ARRAY_SIZE(uia_pattern_lookup_ids) },
        { "control_type", AutomationIdentifierType_ControlType, uia_control_type_lookup_ids, ARRAY_SIZE(uia_control_type_lookup_ids) },
    };
    unsigned int i, y;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        for (y = 0; y < tests[i].ids_count; y++)
        {
            int id = UiaLookupId(tests[i].id_type, tests[i].ids[y].guid);

            if (!id)
            {
                win_skip("No %s id for GUID %s, skipping further tests.\n", tests[i].id_type_name, debugstr_guid(tests[i].ids[y].guid));
                break;
            }

            ok(id == tests[i].ids[y].id, "Unexpected %s id, expected %d, got %d\n", tests[i].id_type_name, tests[i].ids[y].id, id);
        }
    }
}

static const struct prov_method_sequence node_from_prov1[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { 0 }
};

static const struct prov_method_sequence node_from_prov2[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_prov3[] = {
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_prov4[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_prov5[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* These three are only done on Win10v1507 and below. */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider2, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* This is only done on Win10v1507. */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    /* Only called on Windows versions past Win10v1507. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    /* Win10v1507 and below call this. */
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_prov6[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS },
    /* Only called on Windows versions past Win10v1507. */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider2, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* This is only done on Win10v1507. */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    /* Only called on Windows versions past Win10v1507. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_prov7[] = {
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS },
    /* Only called on Windows versions past Win10v1507. */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider2, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* This is only done on Win10v1507. */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    /* Only called on Windows versions past Win10v1507. */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_prov8[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { 0 }
};

static void check_uia_prop_val(PROPERTYID prop_id, enum UIAutomationType type, VARIANT *v, BOOL from_com);
static DWORD WINAPI uia_node_from_provider_test_com_thread(LPVOID param)
{
    HUIANODE node = param;
    HRESULT hr;
    VARIANT v;

    /*
     * Since this is a node representing an IRawElementProviderSimple with
     * ProviderOptions_UseComThreading set, it is only usable in threads that
     * have initialized COM.
     */
    hr = UiaGetPropertyValue(node, UIA_ProcessIdPropertyId, &v);
    ok(hr == CO_E_NOTINITIALIZED, "Unexpected hr %#lx\n", hr);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = UiaGetPropertyValue(node, UIA_ProcessIdPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_uia_prop_val(UIA_ProcessIdPropertyId, UIAutomationType_Int, &v, FALSE);

    /*
     * When retrieving a UIAutomationType_Element property, if UseComThreading
     * is set, we'll get an HUIANODE that will make calls inside of the
     * apartment of the node it is retrieved from. I.e, if we received a node
     * with UseComThreading set from another node with UseComThreading set
     * inside of an STA, the returned node will have all of its methods called
     * from the STA thread.
     */
    Provider_child.prov_opts = ProviderOptions_UseComThreading | ProviderOptions_ServerSideProvider;
    Provider_child.expected_tid = Provider.expected_tid;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_uia_prop_val(UIA_LabeledByPropertyId, UIAutomationType_Element, &v, FALSE);

    /* Unset ProviderOptions_UseComThreading. */
    Provider_child.prov_opts = ProviderOptions_ServerSideProvider;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    /*
     * ProviderOptions_UseComThreading not set, GetPropertyValue will be
     * called on the current thread.
     */
    Provider_child.expected_tid = GetCurrentThreadId();
    check_uia_prop_val(UIA_LabeledByPropertyId, UIAutomationType_Element, &v, FALSE);

    CoUninitialize();

    return 0;
}

static void test_uia_node_from_prov_com_threading(void)
{
    HANDLE thread;
    HUIANODE node;
    HRESULT hr;

    /* Test ProviderOptions_UseComThreading. */
    Provider.hwnd = NULL;
    prov_root = NULL;
    Provider.prov_opts = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok_method_sequence(node_from_prov8, "node_from_prov8");

    /*
     * On Windows versions prior to Windows 10, UiaNodeFromProvider ignores the
     * ProviderOptions_UseComThreading flag.
     */
    if (hr == S_OK)
    {
        win_skip("Skipping ProviderOptions_UseComThreading tests for UiaNodeFromProvider.\n");
        UiaNodeRelease(node);
        return;
    }
    ok(hr == CO_E_NOTINITIALIZED, "Unexpected hr %#lx.\n", hr);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok_method_sequence(node_from_prov8, "node_from_prov8");

    Provider.expected_tid = GetCurrentThreadId();
    thread = CreateThread(NULL, 0, uia_node_from_provider_test_com_thread, (void *)node, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;
        while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    Provider_child.expected_tid = Provider.expected_tid = 0;

    CoUninitialize();
}
static void test_UiaNodeFromProvider(void)
{
    WNDCLASSA cls;
    HUIANODE node;
    HRESULT hr;
    ULONG ref;
    HWND hwnd;
    VARIANT v;

    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "UiaNodeFromProvider class";

    RegisterClassA(&cls);

    hwnd = CreateWindowA("UiaNodeFromProvider class", "Test window", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);

    /* Run these tests early, we end up in an implicit MTA later. */
    test_uia_node_from_prov_com_threading();

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = UiaNodeFromProvider(NULL, &node);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Must have a successful call to get_ProviderOptions. */
    Provider.prov_opts = 0;
    node = (void *)0xdeadbeef;
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(!node, "node != NULL\n");
    ok_method_sequence(node_from_prov1, "node_from_prov1");

    /* No HWND exposed through Provider. */
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    node = (void *)0xdeadbeef;
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", TRUE);
    VariantClear(&v);

    ok_method_sequence(node_from_prov2, "node_from_prov2");

    /* HUIANODE represents a COM interface. */
    ref = IUnknown_AddRef((IUnknown *)node);
    ok(ref == 2, "Unexpected refcnt %ld\n", ref);

    ref = IUnknown_AddRef((IUnknown *)node);
    ok(ref == 3, "Unexpected refcnt %ld\n", ref);

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");

    ref = IUnknown_Release((IUnknown *)node);
    ok(ref == 1, "Unexpected refcnt %ld\n", ref);

    ref = IUnknown_Release((IUnknown *)node);
    ok(ref == 0, "Unexpected refcnt %ld\n", ref);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    /*
     * No HWND exposed through Provider_child, but it returns a parent from
     * NavigateDirection_Parent. Behavior doesn't change.
     */
    Provider_child.prov_opts = ProviderOptions_ServerSideProvider;
    node = (void *)0xdeadbeef;
    hr = UiaNodeFromProvider(&Provider_child.IRawElementProviderSimple_iface, &node);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
    VariantClear(&v);

    ok_method_sequence(node_from_prov3, "node_from_prov3");
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    /* HWND exposed, but Provider2 not returned from WM_GETOBJECT. */
    Provider.hwnd = hwnd;
    prov_root = NULL;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Win10v1507 and below send this, Windows 7 sends it twice. */
    SET_EXPECT_MULTI(winproc_GETOBJECT_CLIENT, 2);
    Provider.ignore_hwnd_prop = TRUE;
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc_todo(V_BSTR(&v), L"Main", L"Provider", FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Hwnd", NULL, TRUE);
    VariantClear(&v);

    Provider.ignore_hwnd_prop = FALSE;
    ok_method_sequence(node_from_prov4, "node_from_prov4");

    ok(!!node, "node == NULL\n");
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    /*
     * Provider is our main provider, since Provider2 is also a main, it won't
     * get added.
     */
    Provider.hwnd = Provider2.hwnd = hwnd;
    Provider.prov_opts = Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.ignore_hwnd_prop = Provider2.ignore_hwnd_prop = TRUE;
    prov_root = &Provider2.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Windows 7 sends this. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    /* Win10v1507 and below hold a reference to the root provider for the HWND */
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok(!!node, "node == NULL\n");

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc_todo(V_BSTR(&v), L"Main", L"Provider", FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Hwnd", NULL, TRUE);
    VariantClear(&v);

    Provider.ignore_hwnd_prop = Provider2.ignore_hwnd_prop = FALSE;
    ok_method_sequence(node_from_prov5, "node_from_prov5");

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider2.ref == 1, "Unexpected refcnt %ld\n", Provider2.ref);

    /*
     * Provider is classified as an Hwnd provider, Provider2 will become our
     * Main provider since we don't have one already.
     */
    Provider.prov_opts = ProviderOptions_ClientSideProvider;
    Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    prov_root = &Provider2.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Windows 7 sends this. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider2", TRUE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider", FALSE);
    VariantClear(&v);

    ok_method_sequence(node_from_prov6, "node_from_prov6");

    ok(Provider2.ref == 2, "Unexpected refcnt %ld\n", Provider2.ref);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    ok(!!node, "node == NULL\n");
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider2.ref == 1, "Unexpected refcnt %ld\n", Provider2.ref);

    /* Provider_child has a parent, so it will be "(parent link)". */
    Provider_child.prov_opts = ProviderOptions_ClientSideProvider;
    Provider_child.hwnd = hwnd;
    Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    prov_root = &Provider2.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Windows 7 sends this. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = UiaNodeFromProvider(&Provider_child.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider2", FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider_child", TRUE);
    VariantClear(&v);

    ok_method_sequence(node_from_prov7, "node_from_prov7");

    ok(Provider2.ref == 2, "Unexpected refcnt %ld\n", Provider2.ref);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    ok(!!node, "node == NULL\n");
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider2.ref == 1, "Unexpected refcnt %ld\n", Provider2.ref);

    CoUninitialize();
    DestroyWindow(hwnd);
    UnregisterClassA("UiaNodeFromProvider class", NULL);
    prov_root = NULL;
}

/* Sequence for types other than UIAutomationType_Element. */
static const struct prov_method_sequence get_prop_seq[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    { 0 }
};

/* Sequence for getting a property that returns an invalid type. */
static const struct prov_method_sequence get_prop_invalid_type_seq[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    /* Windows 7 calls this. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { 0 }
};

/* UIAutomationType_Element sequence. */
static const struct prov_method_sequence get_elem_prop_seq[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL },
    { 0 }
};

/* UIAutomationType_ElementArray sequence. */
static const struct prov_method_sequence get_elem_arr_prop_seq[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    { &Provider_child2, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_child2, PROV_GET_PROVIDER_OPTIONS },
    { &Provider_child, PROV_GET_PROPERTY_VALUE },
    { &Provider_child2, PROV_GET_PROPERTY_VALUE },
    { 0 }
};

static const struct prov_method_sequence get_pattern_prop_seq[] = {
    { &Provider, PROV_GET_PATTERN_PROV },
    { 0 }
};

static const struct prov_method_sequence get_pattern_prop_seq2[] = {
    { &Provider, PROV_GET_PATTERN_PROV },
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_TODO },
    { 0 }
};

static const struct prov_method_sequence get_bounding_rect_seq[] = {
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_GET_BOUNDING_RECT },
    /*
     * Win10v21H2+ and above call these, attempting to get the fragment root's
     * HWND. I'm guessing this is an attempt to get the HWND's DPI for DPI scaling.
     */
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { 0 }
};

static const struct prov_method_sequence get_bounding_rect_seq2[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_GET_BOUNDING_RECT },
    /*
     * Win10v21H2+ and above call these, attempting to get the fragment root's
     * HWND. I'm guessing this is an attempt to get the HWND's DPI for DPI scaling.
     */
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { 0 }
};

static const struct prov_method_sequence get_bounding_rect_seq3[] = {
    { &Provider_child, FRAG_GET_BOUNDING_RECT },
    /*
     * Win10v21H2+ and above call these, attempting to get the fragment root's
     * HWND. I'm guessing this is an attempt to get the HWND's DPI for DPI scaling.
     */
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { 0 }
};

static const struct prov_method_sequence get_empty_bounding_rect_seq[] = {
    { &Provider_child, FRAG_GET_BOUNDING_RECT },
    { 0 }
};

static void set_uia_rect(struct UiaRect *rect, double left, double top, double width, double height)
{
    rect->left = left;
    rect->top = top;
    rect->width = width;
    rect->height = height;
}

#define check_uia_rect_val( v, rect ) \
        check_uia_rect_val_( (v), (rect), __FILE__, __LINE__)
static void check_uia_rect_val_(VARIANT *v, struct UiaRect *rect, const char *file, int line)
{
    LONG lbound, ubound, elems, idx;
    SAFEARRAY *sa;
    double tmp[4];
    VARTYPE vt;
    HRESULT hr;
    UINT dims;

    ok_(file, line)(V_VT(v) == (VT_R8 | VT_ARRAY), "Unexpected rect VT hr %d.\n", V_VT(v));
    if (V_VT(v) != (VT_R8 | VT_ARRAY))
        return;

    sa = V_ARRAY(v);
    hr = SafeArrayGetVartype(sa, &vt);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(file, line)(vt == VT_R8, "Unexpected vt %d\n", vt);

    dims = SafeArrayGetDim(sa);
    ok_(file, line)(dims == 1, "Unexpected dims %d\n", dims);

    lbound = ubound = elems = 0;
    hr = SafeArrayGetLBound(sa, 1, &lbound);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx for SafeArrayGetLBound\n", hr);
    ok_(file, line)(lbound == 0, "Unexpected lbound %ld\n", lbound);

    hr = SafeArrayGetUBound(sa, 1, &ubound);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx for SafeArrayGetUBound\n", hr);
    ok_(file, line)(ubound == 3, "Unexpected ubound %ld\n", ubound);

    elems = (ubound - lbound) + 1;
    ok_(file, line)(elems == 4, "Unexpected rect elems %ld\n", elems);

    for (idx = 0; idx < ARRAY_SIZE(tmp); idx++)
    {
        hr = SafeArrayGetElement(sa, &idx, &tmp[idx]);
        ok_(file, line)(hr == S_OK, "Unexpected hr %#lx for SafeArrayGetElement at idx %ld.\n", hr, idx);
    }

    ok_(file, line)(tmp[0] == rect->left, "Unexpected left value %f, expected %f\n", tmp[0], rect->left);
    ok_(file, line)(tmp[1] == rect->top, "Unexpected top value %f, expected %f\n", tmp[1], rect->top);
    ok_(file, line)(tmp[2] == rect->width, "Unexpected width value %f, expected %f\n", tmp[2], rect->width);
    ok_(file, line)(tmp[3] == rect->height, "Unexpected height value %f, expected %f\n", tmp[3], rect->height);
}

#define check_uia_rect_rect_val( rect, uia_rect ) \
        check_uia_rect_rect_val_( (rect), (uia_rect), __FILE__, __LINE__)
static void check_uia_rect_rect_val_(RECT *rect, struct UiaRect *uia_rect, const char *file, int line)
{
    ok_(file, line)(rect->left == (LONG)uia_rect->left, "Unexpected left value %ld, expected %ld\n", rect->left, (LONG)uia_rect->left);
    ok_(file, line)(rect->top == (LONG)uia_rect->top, "Unexpected top value %ld, expected %ld\n", rect->top, (LONG)uia_rect->top);
    ok_(file, line)(rect->right == (LONG)(uia_rect->left + uia_rect->width), "Unexpected right value %ld, expected %ld\n", rect->right,
            (LONG)(uia_rect->left + uia_rect->width));
    ok_(file, line)(rect->bottom == (LONG)(uia_rect->top + uia_rect->height), "Unexpected bottom value %ld, expected %ld\n", rect->bottom,
            (LONG)(uia_rect->top + uia_rect->height));
}

static void check_uia_prop_val(PROPERTYID prop_id, enum UIAutomationType type, VARIANT *v, BOOL from_com)
{
    LONG idx;

    switch (type)
    {
    case UIAutomationType_String:
        ok(V_VT(v) == VT_BSTR, "Unexpected VT %d\n", V_VT(v));
        ok(!lstrcmpW(V_BSTR(v), uia_bstr_prop_str), "Unexpected BSTR %s\n", wine_dbgstr_w(V_BSTR(v)));
        ok_method_sequence(get_prop_seq, NULL);
        break;

    case UIAutomationType_Bool:
        ok(V_VT(v) == VT_BOOL, "Unexpected VT %d\n", V_VT(v));

        /* UIA_IsKeyboardFocusablePropertyId is broken on Win8 and Win10v1507. */
        if (prop_id == UIA_IsKeyboardFocusablePropertyId)
            ok(check_variant_bool(v, TRUE) || broken(check_variant_bool(v, FALSE)),
                    "Unexpected BOOL %#x\n", V_BOOL(v));
        else
            ok(check_variant_bool(v, TRUE), "Unexpected BOOL %#x\n", V_BOOL(v));
        ok_method_sequence(get_prop_seq, NULL);
        break;

    case UIAutomationType_Int:
        ok(V_VT(v) == VT_I4, "Unexpected VT %d\n", V_VT(v));

        if (prop_id == UIA_NativeWindowHandlePropertyId)
            ok(ULongToHandle(V_I4(v)) == Provider.hwnd, "Unexpected I4 %#lx\n", V_I4(v));
        else
            ok(V_I4(v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(v));
        ok_method_sequence(get_prop_seq, NULL);
        break;

    case UIAutomationType_IntArray:
        ok(V_VT(v) == (VT_ARRAY | VT_I4), "Unexpected VT %d\n", V_VT(v));

        for (idx = 0; idx < ARRAY_SIZE(uia_i4_arr_prop_val); idx++)
        {
            ULONG val;

            SafeArrayGetElement(V_ARRAY(v), &idx, &val);
            ok(val == uia_i4_arr_prop_val[idx], "Unexpected I4 %#lx at idx %ld\n", val, idx);
        }
        ok_method_sequence(get_prop_seq, NULL);
        break;

    case UIAutomationType_Double:
        ok(V_VT(v) == VT_R8, "Unexpected VT %d\n", V_VT(v));
        ok(V_R8(v) == uia_r8_prop_val, "Unexpected R8 %lf\n", V_R8(v));
        ok_method_sequence(get_prop_seq, NULL);
        break;

    case UIAutomationType_DoubleArray:
        ok(V_VT(v) == (VT_ARRAY | VT_R8), "Unexpected VT %d\n", V_VT(v));
        for (idx = 0; idx < ARRAY_SIZE(uia_r8_arr_prop_val); idx++)
        {
            double val;

            SafeArrayGetElement(V_ARRAY(v), &idx, &val);
            ok(val == uia_r8_arr_prop_val[idx], "Unexpected R8 %lf at idx %ld\n", val, idx);
        }
        ok_method_sequence(get_prop_seq, NULL);
        break;

    case UIAutomationType_Element:
    {
        HUIANODE tmp_node;
        HRESULT hr;
        VARIANT v1;

        if (from_com)
        {
            IUIAutomationElement *elem;

            ok(V_VT(v) == VT_UNKNOWN, "Unexpected VT %d\n", V_VT(v));
            hr = IUnknown_QueryInterface(V_UNKNOWN(v), &IID_IUIAutomationElement, (void **)&elem);
            VariantClear(v);
            ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
            ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

            hr = IUIAutomationElement_GetCurrentPropertyValueEx(elem, UIA_ControlTypePropertyId, TRUE, &v1);
            ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
            IUIAutomationElement_Release(elem);
        }
        else
        {
#ifdef _WIN64
            ok(V_VT(v) == VT_I8, "Unexpected VT %d\n", V_VT(v));
            tmp_node = (HUIANODE)V_I8(v);
#else
            ok(V_VT(v) == VT_I4, "Unexpected VT %d\n", V_VT(v));
            tmp_node = (HUIANODE)V_I4(v);
#endif
            ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

            hr = UiaGetPropertyValue(tmp_node, UIA_ControlTypePropertyId, &v1);
            ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
            ok(UiaNodeRelease(tmp_node), "Failed to release node\n");
        }

        ok(V_VT(&v1) == VT_I4, "Unexpected VT %d\n", V_VT(&v1));
        ok(V_I4(&v1) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v1));
        ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

        ok_method_sequence(get_elem_prop_seq, NULL);
        break;
    }

    case UIAutomationType_ElementArray:
        ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
        ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);
        if (from_com)
        {
            IUIAutomationElementArray *elem_arr = NULL;
            HRESULT hr;
            int len;

            ok(V_VT(v) == VT_UNKNOWN, "Unexpected VT %d\n", V_VT(v));
            hr = IUnknown_QueryInterface(V_UNKNOWN(v), &IID_IUIAutomationElementArray, (void **)&elem_arr);
            ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
            ok(!!elem_arr, "elem_arr == NULL\n");
            if (!elem_arr)
            {
                VariantClear(v);
                break;
            }

            hr = IUIAutomationElementArray_get_Length(elem_arr, &len);
            ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
            ok(len == ARRAY_SIZE(uia_unk_arr_prop_val), "Unexpected length %d\n", len);

            for (idx = 0; idx < ARRAY_SIZE(uia_unk_arr_prop_val); idx++)
            {
                IUIAutomationElement *tmp_elem = NULL;
                VARIANT v1;

                hr = IUIAutomationElementArray_GetElement(elem_arr, idx, &tmp_elem);
                ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
                ok(!!tmp_elem, "tmp_elem == NULL\n");

                hr = IUIAutomationElement_GetCurrentPropertyValueEx(tmp_elem, UIA_ControlTypePropertyId, TRUE, &v1);
                ok(hr == S_OK, "elem[%ld] Unexpected hr %#lx\n", idx, hr);
                ok(V_VT(&v1) == VT_I4, "elem[%ld] Unexpected VT %d\n", idx, V_VT(&v1));
                ok(V_I4(&v1) == uia_i4_prop_val, "elem[%ld] Unexpected I4 %#lx\n", idx, V_I4(&v1));

                IUIAutomationElement_Release(tmp_elem);
                VariantClear(&v1);
            }

            IUIAutomationElementArray_Release(elem_arr);
        }
        else
        {
            ok(V_VT(v) == (VT_ARRAY | VT_UNKNOWN), "Unexpected VT %d\n", V_VT(v));
            if (V_VT(v) != (VT_ARRAY | VT_UNKNOWN))
                break;

            for (idx = 0; idx < ARRAY_SIZE(uia_unk_arr_prop_val); idx++)
            {
                HUIANODE tmp_node;
                HRESULT hr;
                VARIANT v1;

                SafeArrayGetElement(V_ARRAY(v), &idx, &tmp_node);

                hr = UiaGetPropertyValue(tmp_node, UIA_ControlTypePropertyId, &v1);
                ok(hr == S_OK, "node[%ld] Unexpected hr %#lx\n", idx, hr);
                ok(V_VT(&v1) == VT_I4, "node[%ld] Unexpected VT %d\n", idx, V_VT(&v1));
                ok(V_I4(&v1) == uia_i4_prop_val, "node[%ld] Unexpected I4 %#lx\n", idx, V_I4(&v1));

                ok(UiaNodeRelease(tmp_node), "Failed to release node[%ld]\n", idx);
                VariantClear(&v1);
            }
        }

        VariantClear(v);
        ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);
        ok(Provider_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_child2.ref);
        ok_method_sequence(get_elem_arr_prop_seq, NULL);
        break;

    default:
        break;
    }

    VariantClear(v);
    V_VT(v) = VT_EMPTY;
}

struct uia_element_property {
    const GUID *prop_guid;
    enum UIAutomationType type;
    BOOL skip_invalid;
};

static const struct uia_element_property element_properties[] = {
    { &ProcessId_Property_GUID,                UIAutomationType_Int, TRUE },
    { &ControlType_Property_GUID,              UIAutomationType_Int },
    { &LocalizedControlType_Property_GUID,     UIAutomationType_String, TRUE },
    { &Name_Property_GUID,                     UIAutomationType_String },
    { &AcceleratorKey_Property_GUID,           UIAutomationType_String },
    { &AccessKey_Property_GUID,                UIAutomationType_String },
    { &HasKeyboardFocus_Property_GUID,         UIAutomationType_Bool },
    { &IsKeyboardFocusable_Property_GUID,      UIAutomationType_Bool },
    { &IsEnabled_Property_GUID,                UIAutomationType_Bool },
    { &AutomationId_Property_GUID,             UIAutomationType_String },
    { &ClassName_Property_GUID,                UIAutomationType_String },
    { &HelpText_Property_GUID,                 UIAutomationType_String },
    { &Culture_Property_GUID,                  UIAutomationType_Int },
    { &IsControlElement_Property_GUID,         UIAutomationType_Bool },
    { &IsContentElement_Property_GUID,         UIAutomationType_Bool },
    { &LabeledBy_Property_GUID,                UIAutomationType_Element },
    { &IsPassword_Property_GUID,               UIAutomationType_Bool },
    { &NewNativeWindowHandle_Property_GUID,    UIAutomationType_Int },
    { &ItemType_Property_GUID,                 UIAutomationType_String },
    { &IsOffscreen_Property_GUID,              UIAutomationType_Bool },
    { &Orientation_Property_GUID,              UIAutomationType_Int },
    { &FrameworkId_Property_GUID,              UIAutomationType_String },
    { &IsRequiredForForm_Property_GUID,        UIAutomationType_Bool },
    { &ItemStatus_Property_GUID,               UIAutomationType_String },
    { &AriaRole_Property_GUID,                 UIAutomationType_String },
    { &AriaProperties_Property_GUID,           UIAutomationType_String },
    { &IsDataValidForForm_Property_GUID,       UIAutomationType_Bool },
    { &ControllerFor_Property_GUID,            UIAutomationType_ElementArray },
    { &DescribedBy_Property_GUID,              UIAutomationType_ElementArray },
    { &FlowsTo_Property_GUID,                  UIAutomationType_ElementArray },
    /* Implemented on Win8+ */
    { &OptimizeForVisualContent_Property_GUID, UIAutomationType_Bool },
    { &LiveSetting_Property_GUID,              UIAutomationType_Int },
    { &FlowsFrom_Property_GUID,                UIAutomationType_ElementArray },
    { &IsPeripheral_Property_GUID,             UIAutomationType_Bool },
    /* Implemented on Win10v1507+. */
    { &PositionInSet_Property_GUID,            UIAutomationType_Int },
    { &SizeOfSet_Property_GUID,                UIAutomationType_Int },
    { &Level_Property_GUID,                    UIAutomationType_Int },
    { &AnnotationTypes_Property_GUID,          UIAutomationType_IntArray },
    { &AnnotationObjects_Property_GUID,        UIAutomationType_ElementArray },
    /* Implemented on Win10v1809+. */
    { &LandmarkType_Property_GUID,             UIAutomationType_Int },
    { &LocalizedLandmarkType_Property_GUID,    UIAutomationType_String, TRUE },
    { &FullDescription_Property_GUID,          UIAutomationType_String },
    { &FillColor_Property_GUID,                UIAutomationType_Int },
    { &OutlineColor_Property_GUID,             UIAutomationType_IntArray },
    { &FillType_Property_GUID,                 UIAutomationType_Int },
    { &VisualEffects_Property_GUID,            UIAutomationType_Int },
    { &OutlineThickness_Property_GUID,         UIAutomationType_DoubleArray },
    { &Rotation_Property_GUID,                 UIAutomationType_Double },
    { &Size_Property_GUID,                     UIAutomationType_DoubleArray },
    { &HeadingLevel_Property_GUID,             UIAutomationType_Int },
    { &IsDialog_Property_GUID,                 UIAutomationType_Bool },
};

static void test_UiaGetPropertyValue(void)
{
    const struct uia_element_property *elem_prop;
    struct UiaRect rect;
    IUnknown *unk_ns;
    unsigned int i;
    HUIANODE node;
    int prop_id;
    HRESULT hr;
    VARIANT v;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider_child.prov_opts = Provider_child2.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = Provider_child.hwnd = Provider_child2.hwnd = NULL;
    node = (void *)0xdeadbeef;
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok_method_sequence(node_from_prov8, NULL);

    hr = UiaGetReservedNotSupportedValue(&unk_ns);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(element_properties); i++)
    {
        elem_prop = &element_properties[i];

        Provider.ret_invalid_prop_type = FALSE;
        VariantClear(&v);
        prop_id = UiaLookupId(AutomationIdentifierType_Property, elem_prop->prop_guid);
        if (!prop_id)
        {
            win_skip("No propertyId for GUID %s, skipping further tests.\n", debugstr_guid(elem_prop->prop_guid));
            break;
        }
        winetest_push_context("prop_id %d", prop_id);
        hr = UiaGetPropertyValue(node, prop_id, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_uia_prop_val(prop_id, elem_prop->type, &v, FALSE);

        /*
         * Some properties have special behavior if an invalid value is
         * returned, skip them here.
         */
        if (!elem_prop->skip_invalid)
        {
            Provider.ret_invalid_prop_type = TRUE;
            hr = UiaGetPropertyValue(node, prop_id, &v);
            if (hr == E_NOTIMPL)
                todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            else
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            if (SUCCEEDED(hr))
            {
                ok_method_sequence(get_prop_invalid_type_seq, NULL);
                ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
                ok(V_UNKNOWN(&v) == unk_ns, "unexpected IUnknown %p\n", V_UNKNOWN(&v));
                VariantClear(&v);
            }
        }

        winetest_pop_context();
    }

    /* IValueProvider pattern property IDs. */
    Provider.value_pattern_data.is_supported = FALSE;
    hr = UiaGetPropertyValue(node, UIA_ValueIsReadOnlyPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(V_UNKNOWN(&v) == unk_ns, "unexpected IUnknown %p\n", V_UNKNOWN(&v));
    ok_method_sequence(get_pattern_prop_seq, NULL);
    VariantClear(&v);

    Provider.value_pattern_data.is_supported = TRUE;
    for (i = 0; i < 2; i++)
    {
        Provider.value_pattern_data.is_read_only = i;

        hr = UiaGetPropertyValue(node, UIA_ValueIsReadOnlyPropertyId, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&v) == VT_BOOL, "Unexpected VT %d\n", V_VT(&v));
        ok(check_variant_bool(&v, i), "Unexpected BOOL %#x\n", V_BOOL(&v));
        ok_method_sequence(get_pattern_prop_seq, NULL);
        VariantClear(&v);
    }

    /* ILegacyIAccessibleProvider pattern property IDs. */
    Provider.legacy_acc_pattern_data.is_supported = FALSE;
    hr = UiaGetPropertyValue(node, UIA_LegacyIAccessibleChildIdPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(V_UNKNOWN(&v) == unk_ns, "unexpected IUnknown %p\n", V_UNKNOWN(&v));
    ok_method_sequence(get_pattern_prop_seq2, NULL);
    VariantClear(&v);

    Provider.legacy_acc_pattern_data.is_supported = TRUE;
    for (i = 0; i < 2; i++)
    {
        Provider.legacy_acc_pattern_data.child_id = i;

        hr = UiaGetPropertyValue(node, UIA_LegacyIAccessibleChildIdPropertyId, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
        ok(V_I4(&v) == i, "Unexpected I4 %#lx\n", V_I4(&v));
        ok_method_sequence(get_pattern_prop_seq, NULL);
        VariantClear(&v);
    }

    for (i = 0; i < 2; i++)
    {
        Provider.legacy_acc_pattern_data.role = i;

        hr = UiaGetPropertyValue(node, UIA_LegacyIAccessibleRolePropertyId, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
        ok(V_I4(&v) == i, "Unexpected I4 %#lx\n", V_I4(&v));
        ok_method_sequence(get_pattern_prop_seq, NULL);
        VariantClear(&v);
    }

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, FALSE);

    /*
     * Windows 7 will call get_FragmentRoot in an endless loop until the fragment root returns an HWND.
     * It's the only version with this behavior.
     */
    if (!UiaLookupId(AutomationIdentifierType_Property, &OptimizeForVisualContent_Property_GUID))
    {
        win_skip("Skipping UIA_BoundingRectanglePropertyId tests for Win7\n");
        goto exit;
    }

    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, FALSE);
    node = (void *)0xdeadbeef;
    hr = UiaNodeFromProvider(&Provider_child.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    /* Non-empty bounding rectangle, will return a VT_R8 SAFEARRAY. */
    set_uia_rect(&rect, 0, 0, 50, 50);
    Provider_child.bounds_rect = rect;
    hr = UiaGetPropertyValue(node, UIA_BoundingRectanglePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_rect_val(&v, &rect);
    VariantClear(&v);

    ok_method_sequence(get_bounding_rect_seq, "get_bounding_rect_seq");

    /* Empty bounding rectangle will return ReservedNotSupportedValue. */
    set_uia_rect(&rect, 0, 0, 0, 0);
    Provider_child.bounds_rect = rect;
    hr = UiaGetPropertyValue(node, UIA_BoundingRectanglePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(V_UNKNOWN(&v) == unk_ns, "unexpected IUnknown %p\n", V_UNKNOWN(&v));
    VariantClear(&v);
    ok_method_sequence(get_empty_bounding_rect_seq, "get_empty_bounding_rect_seq");

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, FALSE);

exit:

    IUnknown_Release(unk_ns);
    CoUninitialize();
}

static const struct prov_method_sequence get_runtime_id1[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { 0 }
};

static const struct prov_method_sequence get_runtime_id2[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT },
    { 0 }
};

static const struct prov_method_sequence get_runtime_id3[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    /* Not called on Windows 7. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    /* Only called on Win8+. */
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider2, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { 0 }
};

static const struct prov_method_sequence get_runtime_id4[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    /* Not called on Windows 7. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    /* These methods are only called on Win8+. */
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { 0 }
};

static const struct prov_method_sequence get_runtime_id5[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { 0 }
};

static void test_UiaGetRuntimeId(void)
{
    const int root_prov_opts[] = { ProviderOptions_ServerSideProvider, ProviderOptions_ServerSideProvider | ProviderOptions_OverrideProvider,
                                   ProviderOptions_ClientSideProvider | ProviderOptions_NonClientAreaProvider };
    int rt_id[4], tmp, i;
    IUnknown *unk_ns;
    SAFEARRAY *sa;
    WNDCLASSA cls;
    HUIANODE node;
    HRESULT hr;
    HWND hwnd;
    VARIANT v;

    VariantInit(&v);
    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "UiaGetRuntimeId class";

    RegisterClassA(&cls);

    hwnd = CreateWindowA("UiaGetRuntimeId class", "Test window", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);

    hr = UiaGetReservedNotSupportedValue(&unk_ns);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    Provider.prov_opts = Provider2.prov_opts = Provider_child.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = Provider2.hwnd = Provider_child.hwnd = NULL;
    node = (void *)0xdeadbeef;
    hr = UiaNodeFromProvider(&Provider_child.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
    VariantClear(&v);

    ok_method_sequence(node_from_prov3, NULL);

    /* NULL runtime ID. */
    Provider_child.runtime_id[0] = Provider_child.runtime_id[1] = 0;
    sa = (void *)0xdeadbeef;
    hr = UiaGetRuntimeId(node, &sa);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!sa, "sa != NULL\n");
    ok_method_sequence(get_runtime_id1, "get_runtime_id1");

    /* No UiaAppendRuntimeId prefix, returns GetRuntimeId array directly. */
    Provider_child.runtime_id[0] = rt_id[0] = 5;
    Provider_child.runtime_id[1] = rt_id[1] = 2;
    sa = (void *)0xdeadbeef;
    hr = UiaGetRuntimeId(node, &sa);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!sa, "sa == NULL\n");
    check_runtime_id(rt_id, 2, sa);
    SafeArrayDestroy(sa);
    ok_method_sequence(get_runtime_id1, "get_runtime_id1");

    /*
     * If a provider returns a RuntimeId beginning with the constant
     * UiaAppendRuntimeId, UI Automation will add a prefix based on the
     * providers HWND fragment root before returning to the client. The added
     * prefix is 3 int values, with:
     *
     * idx[0] always being 42. (UIA_RUNTIME_ID_PREFIX)
     * idx[1] always being the HWND.
     * idx[2] has three different values depending on what type of provider
     * the fragment root is. Fragment roots can be an override provider, a
     * nonclient provider, or a main provider.
     */
    Provider_child.frag_root = NULL;
    Provider_child.runtime_id[0] = UiaAppendRuntimeId;
    Provider_child.runtime_id[1] = 2;
    sa = (void *)0xdeadbeef;

    /* Provider_child has no fragment root for UiaAppendRuntimeId. */
    hr = UiaGetRuntimeId(node, &sa);
    /* Windows 7 returns S_OK. */
    ok(hr == E_FAIL || broken(hr == S_OK), "Unexpected hr %#lx.\n", hr);
    ok(!sa, "sa != NULL\n");
    ok_method_sequence(get_runtime_id2, "get_runtime_id2");

    /*
     * UIA_RuntimeIdPropertyId won't return a failure code from
     * UiaGetPropertyValue.
     */
    hr = UiaGetPropertyValue(node, UIA_RuntimeIdPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(V_UNKNOWN(&v) == unk_ns, "unexpected IUnknown %p\n", V_UNKNOWN(&v));
    VariantClear(&v);
    ok_method_sequence(get_runtime_id2, "get_runtime_id2");

    /*
     * Provider_child returns a fragment root that doesn't expose an HWND. On
     * Win8+, fragment roots are navigated recursively until either a NULL
     * fragment root is returned, the same fragment root as the current one is
     * returned, or a fragment root with an HWND is returned.
     */
    Provider_child.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    Provider2.hwnd = NULL;
    Provider.frag_root = &Provider2.IRawElementProviderFragmentRoot_iface;
    Provider.hwnd = NULL;
    Provider_child.runtime_id[0] = UiaAppendRuntimeId;
    Provider_child.runtime_id[1] = 2;
    sa = (void *)0xdeadbeef;
    hr = UiaGetRuntimeId(node, &sa);
    /* Windows 7 returns S_OK. */
    ok(hr == E_FAIL || broken(hr == S_OK), "Unexpected hr %#lx.\n", hr);
    ok(!sa, "sa != NULL\n");
    ok_method_sequence(get_runtime_id3, "get_runtime_id3");

    /* Provider2 returns an HWND. */
    Provider.hwnd = NULL;
    Provider.frag_root = &Provider2.IRawElementProviderFragmentRoot_iface;
    Provider2.hwnd = hwnd;
    Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    Provider_child.runtime_id[0] = UiaAppendRuntimeId;
    Provider_child.runtime_id[1] = rt_id[3] = 2;
    sa = NULL;
    hr = UiaGetRuntimeId(node, &sa);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* Windows 7 returns a NULL RuntimeId due to no fragment recursion. */
    ok(!!sa || broken(!sa), "sa == NULL\n");
    SafeArrayDestroy(sa);

    ok_method_sequence(get_runtime_id4, "get_runtime_id4");

    /* Test RuntimeId values for different root fragment provider types. */
    Provider.frag_root = NULL;
    Provider.hwnd = hwnd;
    Provider_child.runtime_id[0] = UiaAppendRuntimeId;
    Provider_child.runtime_id[1] = rt_id[3] = 2;
    rt_id[0] = UIA_RUNTIME_ID_PREFIX;
    rt_id[1] = HandleToULong(hwnd);
    for (i = 0; i < ARRAY_SIZE(root_prov_opts); i++)
    {
        LONG lbound;

        Provider.prov_opts = root_prov_opts[i];
        sa = NULL;
        hr = UiaGetRuntimeId(node, &sa);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!!sa, "sa == NULL\n");

        hr = SafeArrayGetLBound(sa, 1, &lbound);
        ok(hr == S_OK, "Failed to get LBound with hr %#lx\n", hr);

        lbound = lbound + 2;
        hr = SafeArrayGetElement(sa, &lbound, &tmp);
        ok(hr == S_OK, "Failed to get element with hr %#lx\n", hr);
        if (i)
            ok(rt_id[2] != tmp, "Expected different runtime id value from previous\n");

        rt_id[2] = tmp;
        check_runtime_id(rt_id, 4, sa);

        SafeArrayDestroy(sa);
        ok_method_sequence(get_runtime_id5, "get_runtime_id5");
    }

    UiaNodeRelease(node);

    /* Test behavior on a node with an associated HWND. */
    Provider.prov_opts = ProviderOptions_ClientSideProvider;
    Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = Provider2.hwnd = hwnd;
    prov_root = &Provider2.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Windows 7 sends this. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    VariantInit(&v);
    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider2", TRUE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider", FALSE);
    VariantClear(&v);

    ok_method_sequence(node_from_prov6, "node_from_prov6");

    /* No methods called, RuntimeId is based on the node's HWND. */
    hr = UiaGetRuntimeId(node, &sa);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!sa, "sa == NULL\n");
    ok(!sequence_cnt, "Unexpected sequence_cnt %d\n", sequence_cnt);

    rt_id[0] = UIA_RUNTIME_ID_PREFIX;
    rt_id[1] = HandleToULong(hwnd);
    check_runtime_id(rt_id, 2, sa);
    SafeArrayDestroy(sa);
    UiaNodeRelease(node);

    DestroyWindow(hwnd);
    UnregisterClassA("UiaGetRuntimeId class", NULL);
    CoUninitialize();
}

static const struct prov_method_sequence node_from_var_seq[] = {
    NODE_CREATE_SEQ(&Provider),
    { 0 },
};

static void test_UiaHUiaNodeFromVariant(void)
{
    HUIANODE node, node2;
    ULONG node_ref;
    HRESULT hr;
    VARIANT v;

    hr = UiaHUiaNodeFromVariant(NULL, &node);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    node = (void *)0xdeadbeef;
    V_VT(&v) = VT_R8;
    hr = UiaHUiaNodeFromVariant(&v, &node);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!node, "node != NULL\n");

    node = NULL;
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, FALSE);
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok(!!node, "node == NULL\n");
    ok_method_sequence(node_from_var_seq, "node_from_var_seq");

    node2 = (void *)0xdeadbeef;
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown *)node;
    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node == node2, "node != NULL\n");

    node_ref = IUnknown_AddRef((IUnknown *)node);
    ok(node_ref == 3, "Unexpected node_ref %ld\n", node_ref);
    VariantClear(&v);
    IUnknown_Release((IUnknown *)node);

#ifdef _WIN64
    node2 = (void *)0xdeadbeef;
    V_VT(&v) = VT_I4;
    V_I4(&v) = 0xbeefdead;
    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!node2, "node2 != NULL\n");

    node2 = (void *)0xdeadbeef;
    V_VT(&v) = VT_I8;
    V_I8(&v) = (UINT64)node;
    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node2 == (void *)V_I8(&v), "node2 != V_I8\n");
#else
    node2 = (void *)0xdeadbeef;
    V_VT(&v) = VT_I8;
    V_I8(&v) = 0xbeefdead;
    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!node2, "node2 != NULL\n");

    node2 = (void *)0xdeadbeef;
    V_VT(&v) = VT_I4;
    V_I4(&v) = (UINT32)node;
    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(node2 == (void *)V_I4(&v), "node2 != V_I4\n");
#endif

    UiaNodeRelease(node);
}

static const struct prov_method_sequence node_from_hwnd1[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd2[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Windows 10+ calls this. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd3[] = {
    NODE_CREATE_SEQ2(&Provider),
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win11+ */
    /* Windows 11 sends WM_GETOBJECT twice on nested nodes. */
    NODE_CREATE_SEQ2_OPTIONAL(&Provider),
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd4[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd5[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_LabeledByPropertyId */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Only done in Windows 8+. */
    { &Provider_child, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    /* These two are only done on Windows 7. */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_child, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd6[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Next 4 are only done in Windows 8+. */
    { &Provider_child, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Next two are only done on Win10v1809+. */
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    /* Next two are only done on Win10v1809+. */
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd7[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win11+ */
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd8[] = {
    NODE_CREATE_SEQ2(&Provider),
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win11+ */
    /* Windows 11 sends WM_GETOBJECT twice on nested nodes. */
    NODE_CREATE_SEQ2_OPTIONAL(&Provider),
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd9[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Only done in Windows 8+. */
    { &Provider, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL },
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    /* These three are only done on Windows 7. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence node_from_hwnd10[] = {
    NODE_CREATE_SEQ(&Provider),
    /* Next two only done on Windows 8+. */
    { &Provider, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL },
    { &Provider, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL }, /* Only done on Win11+. */
    { 0 }
};

static const struct prov_method_sequence disconnect_prov1[] = {
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_child, PROV_GET_PROVIDER_OPTIONS },
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_FRAGMENT_ROOT },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { 0 }
};

static const struct prov_method_sequence disconnect_prov2[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, FRAG_GET_FRAGMENT_ROOT },
    { 0 }
};

static const struct prov_method_sequence disconnect_prov3[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, FRAG_GET_RUNTIME_ID },
    { 0 }
};

static const struct prov_method_sequence disconnect_prov4[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { 0 }
};

static void test_UiaNodeFromHandle_client_proc(void)
{
    APTTYPEQUALIFIER apt_qualifier;
    APTTYPE apt_type;
    WCHAR buf[2048];
    HUIANODE node;
    HRESULT hr;
    DWORD pid;
    HWND hwnd;
    VARIANT v;

    hwnd = FindWindowA("UiaNodeFromHandle class", "Test window");

    hr = CoGetApartmentType(&apt_type, &apt_qualifier);
    ok(hr == CO_E_NOTINITIALIZED, "Unexpected hr %#lx\n", hr);

    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    memset(buf, 0, sizeof(buf));
    GetWindowThreadProcessId(hwnd, &pid);
    ok(get_nested_provider_desc(V_BSTR(&v), L"Main", FALSE, buf), "Failed to get nested provider description\n");
    check_node_provider_desc_prefix(buf, pid, hwnd);
    check_node_provider_desc(buf, L"Main", L"Provider", TRUE);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Hwnd", NULL, TRUE);

    VariantClear(&v);

    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));

    /*
     * On Windows 8 and above, if a node contains a nested provider, the
     * process will be in an implicit MTA until the node is released.
     */
    hr = CoGetApartmentType(&apt_type, &apt_qualifier);
    ok(hr == S_OK || broken(hr == CO_E_NOTINITIALIZED), "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(apt_type == APTTYPE_MTA, "Unexpected apt_type %#x\n", apt_type);
        ok(apt_qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA, "Unexpected apt_qualifier %#x\n", apt_qualifier);
    }

    UiaNodeRelease(node);

    /* Node released, we're out of the implicit MTA. */
    hr = CoGetApartmentType(&apt_type, &apt_qualifier);

    /* Windows 10v1709-1809 are stuck in an implicit MTA. */
    ok(hr == CO_E_NOTINITIALIZED || broken(hr == S_OK), "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(apt_type == APTTYPE_MTA, "Unexpected apt_type %#x\n", apt_type);
        ok(apt_qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA, "Unexpected apt_qualifier %#x\n", apt_qualifier);
    }
}

static DWORD WINAPI uia_node_from_handle_test_thread(LPVOID param)
{
    HUIANODE node, node2, node3;
    HWND hwnd = (HWND)param;
    WCHAR buf[2048];
    HRESULT hr;
    VARIANT v;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    /* Only sent twice on Windows 11. */
    SET_EXPECT_MULTI(winproc_GETOBJECT_UiaRoot, 2);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    prov_root = &Provider.IRawElementProviderSimple_iface;
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = hwnd;
    Provider.runtime_id[0] = Provider.runtime_id[1] = 0;
    Provider.frag_root = NULL;
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.ignore_hwnd_prop = TRUE;
    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref >= 2, "Unexpected refcnt %ld\n", Provider.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    memset(buf, 0, sizeof(buf));
    ok(get_nested_provider_desc(V_BSTR(&v), L"Main", FALSE, buf), "Failed to get nested provider description\n");
    check_node_provider_desc_prefix(buf, GetCurrentProcessId(), hwnd);
    check_node_provider_desc(buf, L"Main", L"Provider", TRUE);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);

    VariantClear(&v);

    Provider.ignore_hwnd_prop = FALSE;
    ok_method_sequence(node_from_hwnd3, "node_from_hwnd3");

    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_uia_prop_val(UIA_ControlTypePropertyId, UIAutomationType_Int, &v, FALSE);

    /*
     * On Windows, nested providers are always called on a separate thread if
     * UseComThreading isn't set. Since we're doing COM marshaling, if we're
     * currently in an MTA, we just call the nested provider from the current
     * thread.
     */
    todo_wine ok(Provider.last_call_tid != GetCurrentThreadId() &&
            Provider.last_call_tid != GetWindowThreadProcessId(hwnd, NULL), "Expected method call on separate thread\n");

    /*
     * Elements returned from nested providers have to be able to get a
     * RuntimeId, or else we'll get E_FAIL on Win8+.
     */
    Provider_child.prov_opts = ProviderOptions_ServerSideProvider;
    Provider_child.expected_tid = Provider.expected_tid;
    Provider_child.runtime_id[0] = UiaAppendRuntimeId;
    Provider_child.runtime_id[1] = 2;
    Provider_child.frag_root = NULL;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == E_FAIL || broken(hr == S_OK), "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = UiaHUiaNodeFromVariant(&v, &node2);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
        UiaNodeRelease(node2);
    }

    ok_method_sequence(node_from_hwnd5, "node_from_hwnd5");

    /* RuntimeId check succeeds, we'll get a nested node. */
    Provider_child.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    hr = UiaGetPropertyValue(node2, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    /*
     * Even though this is a nested node, without any additional
     * providers, it will not have the 'Nested' prefix.
     */
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
    VariantClear(&v);

    hr = UiaGetPropertyValue(node2, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));
    ok_method_sequence(node_from_hwnd6, "node_from_hwnd6");

    UiaNodeRelease(node2);

    /*
     * There is a delay between nested nodes being released and the
     * corresponding IRawElementProviderSimple release on newer Windows
     * versions.
     */
    if (Provider_child.ref != 1)
        Sleep(50);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    /*
     * Returned nested elements with an HWND will have client-side providers
     * added to them.
     */
    Provider_child.hwnd = hwnd;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    Provider_child.ignore_hwnd_prop = TRUE;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    hr = UiaGetPropertyValue(node2, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    memset(buf, 0, sizeof(buf));
    ok(get_nested_provider_desc(V_BSTR(&v), L"Main", TRUE, buf), "Failed to get nested provider description\n");

    check_node_provider_desc_prefix(buf, GetCurrentProcessId(), hwnd);
    check_node_provider_desc(buf, L"Main", L"Provider_child", TRUE);

    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, FALSE);
    VariantClear(&v);

    Provider_child.ignore_hwnd_prop = FALSE;
    ok_method_sequence(node_from_hwnd7, "node_from_hwnd7");
    UiaNodeRelease(node2);

    if (Provider_child.ref != 1)
        Sleep(50);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    /* Win10v1809 can be slow to call Release on Provider. */
    if (Provider.ref != 1)
        Sleep(50);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    /* ProviderOptions_UseComThreading test from a separate thread. */
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    prov_root = &Provider.IRawElementProviderSimple_iface;
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading, NULL, FALSE);
    Provider.frag_root = NULL;
    Provider.runtime_id[0] = Provider.runtime_id[1] = 0xdeadbeef;
    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    ok_method_sequence(node_from_hwnd10, "node_from_hwnd10");

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    /* Win10v1809 can be slow to call Release on Provider. */
    if (Provider.ref != 1)
        Sleep(50);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    if (!pUiaDisconnectProvider)
    {
        win_skip("UiaDisconnectProvider not exported by uiautomationcore.dll\n");
        goto exit;
    }

    /*
     * UiaDisconnectProvider tests.
     */
    /* Only sent twice on Windows 11. */
    SET_EXPECT_MULTI(winproc_GETOBJECT_UiaRoot, 2);
    prov_root = &Provider.IRawElementProviderSimple_iface;
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = hwnd;
    Provider.runtime_id[0] = Provider.runtime_id[1] = 0;
    Provider.frag_root = NULL;
    Provider.ignore_hwnd_prop = TRUE;
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref >= 2, "Unexpected refcnt %ld\n", Provider.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    memset(buf, 0, sizeof(buf));
    ok(get_nested_provider_desc(V_BSTR(&v), L"Main", FALSE, buf), "Failed to get nested provider description\n");
    check_node_provider_desc_prefix(buf, GetCurrentProcessId(), hwnd);
    check_node_provider_desc(buf, L"Main", L"Provider", TRUE);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
    VariantClear(&v);

    Provider.ignore_hwnd_prop = FALSE;
    ok_method_sequence(node_from_hwnd3, "node_from_hwnd3");

    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_uia_prop_val(UIA_ControlTypePropertyId, UIAutomationType_Int, &v, FALSE);

    /* Nodes returned from a nested node will be tracked and disconnectable. */
    Provider_child.prov_opts = ProviderOptions_ServerSideProvider;
    Provider_child.runtime_id[0] = UiaAppendRuntimeId;
    Provider_child.runtime_id[1] = 2;
    Provider_child.hwnd = NULL;
    Provider_child.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    hr = UiaGetPropertyValue(node2, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
    VariantClear(&v);

    hr = UiaGetPropertyValue(node2, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));
    ok_method_sequence(node_from_hwnd6, "node_from_hwnd6");

    /* Get a new node for the same provider. */
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(Provider_child.ref == 3, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaHUiaNodeFromVariant(&v, &node3);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    hr = UiaGetPropertyValue(node3, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
    VariantClear(&v);

    hr = UiaGetPropertyValue(node3, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));
    ok_method_sequence(node_from_hwnd6, "node_from_hwnd6");

    /*
     * Both node2 and node3 represent Provider_child, one call to
     * UiaDisconnectProvider disconnects both.
     */
    hr = pUiaDisconnectProvider(&Provider_child.IRawElementProviderSimple_iface);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = UiaGetPropertyValue(node2, UIA_ControlTypePropertyId, &v);
    ok(hr == UIA_E_ELEMENTNOTAVAILABLE, "Unexpected hr %#lx\n", hr);

    hr = UiaGetPropertyValue(node3, UIA_ControlTypePropertyId, &v);
    ok(hr == UIA_E_ELEMENTNOTAVAILABLE, "Unexpected hr %#lx\n", hr);
    ok_method_sequence(disconnect_prov1, "disconnect_prov1");

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");
    ok(UiaNodeRelease(node3), "UiaNodeRelease returned FALSE\n");

    /*
     * Returns same failure code as UiaGetRuntimeId when we fail to get a
     * fragment root for AppendRuntimeId.
     */
    Provider.hwnd = NULL;
    Provider.runtime_id[0] = UiaAppendRuntimeId;
    Provider.runtime_id[1] = 2;
    Provider.frag_root = NULL;
    hr = pUiaDisconnectProvider(&Provider.IRawElementProviderSimple_iface);
    ok(hr == E_FAIL, "Unexpected hr %#lx\n", hr);
    ok_method_sequence(disconnect_prov2, "disconnect_prov2");

    /*
     * Comparisons for disconnection are only based on RuntimeId comparisons,
     * not interface pointer values. If an interface returns a NULL RuntimeId,
     * no disconnection will occur.
     */
    Provider.runtime_id[0] = Provider.runtime_id[1] = 0;
    hr = pUiaDisconnectProvider(&Provider.IRawElementProviderSimple_iface);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx\n", hr);
    ok_method_sequence(disconnect_prov3, "disconnect_prov3");

    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_uia_prop_val(UIA_ControlTypePropertyId, UIAutomationType_Int, &v, FALSE);

    /* Finally, disconnect node. */
    Provider.hwnd = hwnd;
    hr = pUiaDisconnectProvider(&Provider.IRawElementProviderSimple_iface);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == UIA_E_ELEMENTNOTAVAILABLE, "Unexpected hr %#lx\n", hr);
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok_method_sequence(disconnect_prov4, "disconnect_prov4");

exit:
    CoUninitialize();

    return 0;
}

static void test_UiaNodeFromHandle(const char *name)
{
    APTTYPEQUALIFIER apt_qualifier;
    PROCESS_INFORMATION proc;
    char cmdline[MAX_PATH];
    STARTUPINFOA startup;
    HUIANODE node, node2;
    APTTYPE apt_type;
    DWORD exit_code;
    WNDCLASSA cls;
    HANDLE thread;
    HRESULT hr;
    HWND hwnd;
    VARIANT v;

    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "UiaNodeFromHandle class";
    RegisterClassA(&cls);

    hwnd = CreateWindowA("UiaNodeFromHandle class", "Test window", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);

    hr = UiaNodeFromHandle(hwnd, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaNodeFromHandle(NULL, &node);
    ok(hr == UIA_E_ELEMENTNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);

    /* COM uninitialized, no provider returned by UiaReturnRawElementProvider. */
    prov_root = NULL;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent twice on Win7. */
    SET_EXPECT_MULTI(winproc_GETOBJECT_CLIENT, 2);
    hr = UiaNodeFromHandle(hwnd, &node);
    /*
     * On all versions of Windows prior to Windows 11, this would fail due to
     * COM being uninitialized, presumably because it tries to get an MSAA
     * proxy. Windows 11 now has the thread end up in an implicit MTA after
     * the call to UiaNodeFromHandle, which is probably why this now succeeds.
     * I don't know of anything that relies on this behavior, so for now we
     * won't match it.
     */
    todo_wine ok(hr == S_OK || broken(hr == E_FAIL), "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);
    if (SUCCEEDED(hr))
        UiaNodeRelease(node);

    /*
     * COM initialized, no provider returned by UiaReturnRawElementProvider.
     * In this case, we get a default MSAA proxy.
     */
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    prov_root = NULL;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT_MULTI(winproc_GETOBJECT_CLIENT, 2);
    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc_todo(V_BSTR(&v), L"Annotation", NULL, FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Main", NULL, FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
    VariantClear(&v);

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");

    /*
     * COM initialized, but provider passed into UiaReturnRawElementProvider
     * returns a failure code on get_ProviderOptions. Same behavior as before.
     */
    Provider.prov_opts = 0;
    Provider.hwnd = hwnd;
    prov_root = &Provider.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT_MULTI(winproc_GETOBJECT_CLIENT, 2);
    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc_todo(V_BSTR(&v), L"Annotation", NULL, FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Main", NULL, FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
    VariantClear(&v);

    ok_method_sequence(node_from_hwnd1, "node_from_hwnd1");

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");

    /*
     * COM initialized, but provider passed into UiaReturnRawElementProvider
     * on Win8+ will have its RuntimeId queried for UiaDisconnectProvider.
     * It will fail due to the lack of a valid fragment root, and we'll fall
     * back to an MSAA proxy. On Win7, RuntimeId isn't queried because the
     * disconnection functions weren't implemented yet.
     */
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = NULL;
    prov_root = &Provider.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT_MULTI(winproc_GETOBJECT_CLIENT, 2);
    Provider.frag_root = NULL;
    Provider.runtime_id[0] = UiaAppendRuntimeId;
    Provider.runtime_id[1] = 1;
    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 1 || broken(Provider.ref == 2), "Unexpected refcnt %ld\n", Provider.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    if (Provider.ref == 1 || get_provider_desc(V_BSTR(&v), L"Annotation:", NULL))
    {
        check_node_provider_desc_todo(V_BSTR(&v), L"Annotation", NULL, FALSE);
        check_node_provider_desc_todo(V_BSTR(&v), L"Main", NULL, FALSE);
    }
    else
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", FALSE);

    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", NULL, TRUE);
    VariantClear(&v);

    ok_method_sequence(node_from_hwnd9, "node_from_hwnd9");
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    /*
     * Bug on Windows 8 through Win10v1709 - if we have a RuntimeId failure,
     * refcount doesn't get decremented.
     */
    ok(Provider.ref == 1 || broken(Provider.ref == 2), "Unexpected refcnt %ld\n", Provider.ref);
    if (Provider.ref == 2)
        IRawElementProviderSimple_Release(&Provider.IRawElementProviderSimple_iface);

    CoUninitialize();

    /*
     * COM uninitialized, return a Provider from UiaReturnRawElementProvider
     * with ProviderOptions_ServerSideProvider.
     */
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = hwnd;
    prov_root = &Provider.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc_todo(V_BSTR(&v), L"Main", L"Provider", FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Hwnd", NULL, TRUE);
    VariantClear(&v);

    ok_method_sequence(node_from_hwnd2, "node_from_hwnd2");

    /* For same-thread HWND nodes, no disconnection will occur. */
    if (pUiaDisconnectProvider)
    {
        hr = pUiaDisconnectProvider(&Provider.IRawElementProviderSimple_iface);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

        ok_method_sequence(disconnect_prov4, "disconnect_prov4");
    }

    /*
     * This is relevant too: Since we don't get a 'nested' node, all calls
     * will occur on the current thread.
     */
    Provider.expected_tid = GetCurrentThreadId();
    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_uia_prop_val(UIA_ControlTypePropertyId, UIAutomationType_Int, &v, FALSE);

    /* UIAutomationType_Element properties will return a normal node. */
    Provider_child.prov_opts = ProviderOptions_ServerSideProvider;
    Provider_child.hwnd = NULL;
    hr = UiaGetPropertyValue(node, UIA_LabeledByPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    hr = UiaGetPropertyValue(node2, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
    VariantClear(&v);

    Provider_child.expected_tid = GetCurrentThreadId();
    hr = UiaGetPropertyValue(node2, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));
    ok_method_sequence(node_from_hwnd4, "node_from_hwnd4");

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");

    Provider.expected_tid = Provider_child.expected_tid = 0;
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");

    /*
     * On Windows 8 and above, after the first successful call to
     * UiaReturnRawElementProvider the process ends up in an implicit MTA
     * until the process exits.
     */
    hr = CoGetApartmentType(&apt_type, &apt_qualifier);
    /* Wine's provider thread doesn't always terminate immediately. */
    if (hr == S_OK && !strcmp(winetest_platform, "wine"))
    {
        Sleep(10);
        hr = CoGetApartmentType(&apt_type, &apt_qualifier);
    }
    todo_wine ok(hr == S_OK || broken(hr == CO_E_NOTINITIALIZED), "Unexpected hr %#lx\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(apt_type == APTTYPE_MTA, "Unexpected apt_type %#x\n", apt_type);
        ok(apt_qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA, "Unexpected apt_qualifier %#x\n", apt_qualifier);
    }

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    thread = CreateThread(NULL, 0, uia_node_from_handle_test_thread, (void *)hwnd, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;
        while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);

    /* Test behavior from separate process. */
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = hwnd;
    Provider.ignore_hwnd_prop = TRUE;
    prov_root = &Provider.IRawElementProviderSimple_iface;
    sprintf(cmdline, "\"%s\" uiautomation UiaNodeFromHandle_client_proc", name);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    /* Only called twice on Windows 11. */
    SET_EXPECT_MULTI(winproc_GETOBJECT_UiaRoot, 2);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &proc);
    while (MsgWaitForMultipleObjects(1, &proc.hProcess, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;
        while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    GetExitCodeProcess(proc.hProcess, &exit_code);
    if (exit_code > 255)
        ok(0, "unhandled exception %08x in child process %04x\n", (UINT)exit_code, (UINT)GetProcessId(proc.hProcess));
    else if (exit_code)
        ok(0, "%u failures in child process\n", (UINT)exit_code);

    CloseHandle(proc.hProcess);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;
    Provider.ignore_hwnd_prop = FALSE;
    ok_method_sequence(node_from_hwnd8, "node_from_hwnd8");

    CoUninitialize();

    DestroyWindow(hwnd);
    UnregisterClassA("UiaNodeFromHandle class", NULL);
    prov_root = NULL;
}

static const struct prov_method_sequence reg_prov_cb1[] = {
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win10+. */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence reg_prov_cb2[] = {
    /* These two are only done on Win10v1809+. */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win10+. */
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence reg_prov_cb3[] = {
    { &Provider_proxy2, HWND_OVERRIDE_GET_OVERRIDE_PROVIDER, METHOD_TODO },
    /* These two are only done on Win10v1809+. */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_proxy, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* These three only done on Win10+. */
    { &Provider_proxy, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_proxy, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence reg_prov_cb4[] = {
    { &Provider_proxy2, HWND_OVERRIDE_GET_OVERRIDE_PROVIDER, METHOD_TODO },
    /* These two are only done on Win10v1809+. */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_override, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider_proxy, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* These four only done on Win10+. */
    { &Provider_override, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_proxy, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_override, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_proxy, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence reg_prov_cb5[] = {
    { &Provider_override, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ControlTypePropertyId */
    { 0 }
};

static const struct prov_method_sequence reg_prov_cb6[] = {
    { &Provider_override, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ControlTypePropertyId */
    { &Provider_override, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win7. */
    { &Provider_proxy, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { 0 }
};

static const struct prov_method_sequence reg_prov_cb7[] = {
    { &Provider_override, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ControlTypePropertyId */
    { &Provider_override, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win7. */
    { &Provider_proxy, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { &Provider_proxy, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win7. */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { 0 }
};

static const struct prov_method_sequence reg_prov_cb8[] = {
    { &Provider_override, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ControlTypePropertyId */
    { &Provider_override, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win7. */
    { &Provider_proxy, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { &Provider_proxy, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win7. */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win7. */
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { 0 }
};

static const struct prov_method_sequence reg_prov_cb9[] = {
    { &Provider_override, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_ControlTypePropertyId */
    { &Provider_override, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win7. */
    { &Provider_proxy, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { &Provider_proxy, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win7. */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win7. */
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win7. */
    { 0 }
};

static const struct prov_method_sequence reg_prov_cb10[] = {
    { &Provider_proxy2, HWND_OVERRIDE_GET_OVERRIDE_PROVIDER, METHOD_TODO },
    /* These two are only done on Win10v1809+. */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_override, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider_proxy, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* These four only done on Win10+. */
    { &Provider_override, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_proxy, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { 0 }
};

static const struct prov_method_sequence reg_prov_cb11[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider_proxy2, HWND_OVERRIDE_GET_OVERRIDE_PROVIDER, METHOD_TODO },
    /* These two are only done on Win10v1809+. */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* These three only done on Win10+. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence reg_prov_cb12[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Win10v1507 and below call this. */
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_TODO },
    { &Provider2, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider2, HWND_OVERRIDE_GET_OVERRIDE_PROVIDER, METHOD_OPTIONAL }, /* Only done on Win10v1809+ */
    { &Provider_proxy2, HWND_OVERRIDE_GET_OVERRIDE_PROVIDER, METHOD_TODO },
    /* These two are only done on Win10v1809+. */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* These three only done on Win10+. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence reg_prov_cb13[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Win10v1507 and below call this. */
    { &Provider2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_TODO },
    { &Provider2, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider2, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider2, HWND_OVERRIDE_GET_OVERRIDE_PROVIDER, METHOD_OPTIONAL }, /* Only done on Win10v1809+ */
    /* Provider override only retrieved successfully on Win10v1809+ */
    { &Provider_override, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_override, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_override, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider_override, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_override, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_proxy2, HWND_OVERRIDE_GET_OVERRIDE_PROVIDER, METHOD_OPTIONAL }, /* Only done on Win10v1507 and below. */
    /* These two are only done on Win10v1809+. */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    /* Only done on Win10v1809+. */
    { &Provider_override, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_override, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win10v1809+ */
    /* These three only done on Win10+. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    /* Only done on Win10v1809+. */
    { &Provider_override, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static SAFEARRAY *get_safearray_for_elprov(IRawElementProviderSimple *elprov)
{
    SAFEARRAY *sa = NULL;
    LONG idx = 0;

    if (elprov)
    {
        sa = SafeArrayCreateVector(VT_UNKNOWN, 0, 1);
        if (sa)
            SafeArrayPutElement(sa, &idx, (void *)elprov);
    }

    return sa;
}

static IRawElementProviderSimple *base_hwnd_prov, *proxy_prov, *parent_proxy_prov, *nc_prov;
#ifdef __REACTOS__
static SAFEARRAY* WINAPI test_uia_provider_callback(HWND hwnd, enum ProviderType prov_type)
#else
static SAFEARRAY WINAPI *test_uia_provider_callback(HWND hwnd, enum ProviderType prov_type)
#endif
{
    IRawElementProviderSimple *elprov = NULL;

    switch (prov_type)
    {
    case ProviderType_BaseHwnd:
        CHECK_EXPECT(prov_callback_base_hwnd);
        if (hwnd == Provider_hwnd3.hwnd)
            elprov = &Provider_hwnd3.IRawElementProviderSimple_iface;
        else if (hwnd == Provider_hwnd2.hwnd)
            elprov = &Provider_hwnd2.IRawElementProviderSimple_iface;
        else
            elprov = base_hwnd_prov;
        break;

    case ProviderType_Proxy:
        if (Provider_proxy.hwnd == hwnd)
        {
            CHECK_EXPECT(prov_callback_proxy);
            elprov = proxy_prov;
        }
        else if (hwnd == GetParent(Provider_proxy.hwnd))
        {
            CHECK_EXPECT(prov_callback_parent_proxy);
            elprov = parent_proxy_prov;
        }
        break;

    case ProviderType_NonClientArea:
        CHECK_EXPECT(prov_callback_nonclient);
        if (hwnd == Provider_nc3.hwnd)
            elprov = &Provider_nc3.IRawElementProviderSimple_iface;
        else if (hwnd == Provider_nc2.hwnd)
            elprov = &Provider_nc2.IRawElementProviderSimple_iface;
        else
            elprov = nc_prov;
        break;

    default:
        break;
    }

    return get_safearray_for_elprov(elprov);
}

/*
 * Windows 11 will infinitely loop if a clientside provider isn't returned for
 * an HWND in certain circumstances. In order to prevent this, we use these
 * temporary clientside providers.
 */
struct ClientSideProvider
{
    IRawElementProviderSimple IRawElementProviderSimple_iface;
    LONG ref;

    enum ProviderType prov_type;
    HWND hwnd;
};

static inline struct ClientSideProvider *impl_from_ClientSideProvider(IRawElementProviderSimple *iface)
{
    return CONTAINING_RECORD(iface, struct ClientSideProvider, IRawElementProviderSimple_iface);
}

static HRESULT WINAPI ClientSideProvider_QueryInterface(IRawElementProviderSimple *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IRawElementProviderSimple) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IRawElementProviderSimple_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI ClientSideProvider_AddRef(IRawElementProviderSimple *iface)
{
    struct ClientSideProvider *This = impl_from_ClientSideProvider(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ClientSideProvider_Release(IRawElementProviderSimple *iface)
{
    struct ClientSideProvider *This = impl_from_ClientSideProvider(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI ClientSideProvider_get_ProviderOptions(IRawElementProviderSimple *iface,
        enum ProviderOptions *ret_val)
{
    struct ClientSideProvider *This = impl_from_ClientSideProvider(iface);

    *ret_val = ProviderOptions_ClientSideProvider;
    if (This->prov_type == ProviderType_NonClientArea)
        *ret_val |= ProviderOptions_NonClientAreaProvider;

    return S_OK;
}

static HRESULT WINAPI ClientSideProvider_GetPatternProvider(IRawElementProviderSimple *iface,
        PATTERNID pattern_id, IUnknown **ret_val)
{
    *ret_val = NULL;
    return S_OK;
}

static HRESULT WINAPI ClientSideProvider_GetPropertyValue(IRawElementProviderSimple *iface, PROPERTYID prop_id,
    VARIANT *ret_val)
{
    struct ClientSideProvider *This = impl_from_ClientSideProvider(iface);

    VariantInit(ret_val);
    switch (prop_id)
    {
    case UIA_NativeWindowHandlePropertyId:
        if (This->prov_type == ProviderType_BaseHwnd)
        {
            V_VT(ret_val) = VT_I4;
            V_I4(ret_val) = HandleToULong(This->hwnd);
        }
        break;

    case UIA_ProviderDescriptionPropertyId:
        V_VT(ret_val) = VT_BSTR;
        V_BSTR(ret_val) = SysAllocString(L"ClientSideProvider");
        break;

    default:
        break;
    }

    return S_OK;
}

static HRESULT WINAPI ClientSideProvider_get_HostRawElementProvider(IRawElementProviderSimple *iface,
        IRawElementProviderSimple **ret_val)
{
    struct ClientSideProvider *This = impl_from_ClientSideProvider(iface);

    return UiaHostProviderFromHwnd(This->hwnd, ret_val);
}

static IRawElementProviderSimpleVtbl ClientSideProviderVtbl = {
    ClientSideProvider_QueryInterface,
    ClientSideProvider_AddRef,
    ClientSideProvider_Release,
    ClientSideProvider_get_ProviderOptions,
    ClientSideProvider_GetPatternProvider,
    ClientSideProvider_GetPropertyValue,
    ClientSideProvider_get_HostRawElementProvider,
};

static IRawElementProviderSimple *create_temporary_clientside_provider(HWND hwnd, enum ProviderType prov_type)
{
    struct ClientSideProvider *prov = malloc(sizeof(*prov));

    if (!prov)
    {
        trace("Failed to allocate memory for temporary clientside provider\n");
        return NULL;
    }

    prov->IRawElementProviderSimple_iface.lpVtbl = &ClientSideProviderVtbl;
    prov->ref = 1;
    prov->prov_type = prov_type;
    prov->hwnd = hwnd;

    return &prov->IRawElementProviderSimple_iface;
}

static CRITICAL_SECTION clientside_provider_callback_cs;
static CRITICAL_SECTION_DEBUG clientside_provider_callback_cs_debug =
{
    0, 0, &clientside_provider_callback_cs,
    { &clientside_provider_callback_cs_debug.ProcessLocksList, &clientside_provider_callback_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": clientside_provider_callback_cs") }
};
static CRITICAL_SECTION clientside_provider_callback_cs = { &clientside_provider_callback_cs_debug, -1, 0, 0, 0, 0 };
static CONDITION_VARIABLE *clientside_provider_callback_cv;

/*
 * Same deal as test_uia_provider_callback, except we only return a provider
 * if we can match one by HWND. This is necessary due to certain versions of
 * Windows 10 unpredictably attempting to create elements in the background.
 */
#ifdef __REACTOS__
static SAFEARRAY* WINAPI uia_com_win_event_clientside_provider_callback(HWND hwnd, enum ProviderType prov_type)
#else
static SAFEARRAY WINAPI *uia_com_win_event_clientside_provider_callback(HWND hwnd, enum ProviderType prov_type)
#endif
{
    IRawElementProviderSimple *elprov = NULL;
    SAFEARRAY *sa = NULL;

    EnterCriticalSection(&clientside_provider_callback_cs);
    if (clientside_provider_callback_cv)
        WakeConditionVariable(clientside_provider_callback_cv);
    LeaveCriticalSection(&clientside_provider_callback_cs);
    switch (prov_type)
    {
    case ProviderType_BaseHwnd:
        if (hwnd == Provider_hwnd3.hwnd)
            elprov = &Provider_hwnd3.IRawElementProviderSimple_iface;
        else if (hwnd == Provider_hwnd2.hwnd)
            elprov = &Provider_hwnd2.IRawElementProviderSimple_iface;
        else if (hwnd == Provider_hwnd.hwnd)
            elprov = &Provider_hwnd.IRawElementProviderSimple_iface;

        if (elprov)
            CHECK_EXPECT(prov_callback_base_hwnd);
        break;

    case ProviderType_Proxy:
        if (Provider_proxy.hwnd == hwnd)
            elprov = proxy_prov;
        break;

    case ProviderType_NonClientArea:
        if (hwnd == Provider_nc3.hwnd)
            elprov = &Provider_nc3.IRawElementProviderSimple_iface;
        else if (hwnd == Provider_nc2.hwnd)
            elprov = &Provider_nc2.IRawElementProviderSimple_iface;
        else if (hwnd == Provider_nc.hwnd)
            elprov = &Provider_nc.IRawElementProviderSimple_iface;

        if (elprov)
            CHECK_EXPECT(prov_callback_nonclient);
        break;

    default:
        break;
    }

    if (!elprov)
    {
        if (!(elprov = create_temporary_clientside_provider(hwnd, prov_type)))
            return NULL;
    }
    else
        IRawElementProviderSimple_AddRef(elprov);

    sa = get_safearray_for_elprov(elprov);
    IRawElementProviderSimple_Release(elprov);
    return sa;
}

/*
 * Some versions of Windows 10 query multiple unrelated HWNDs when
 * winevents are fired, this function waits for these to stop before
 * continuing to the next test.
 */
#define TIME_SINCE_LAST_CALLBACK_TIMEOUT 200
static BOOL wait_for_clientside_callbacks(DWORD total_timeout)
{
    DWORD start_time = GetTickCount();
    CONDITION_VARIABLE cv;
    BOOL ret = FALSE;

    InitializeConditionVariable(&cv);
    EnterCriticalSection(&clientside_provider_callback_cs);
    clientside_provider_callback_cv = &cv;
    while (1)
    {
        BOOL ret_val = SleepConditionVariableCS(&cv, &clientside_provider_callback_cs, TIME_SINCE_LAST_CALLBACK_TIMEOUT);

        if ((GetTickCount() - start_time) >= total_timeout)
            ret = TRUE;
        else if (!ret_val && (GetLastError() == ERROR_TIMEOUT))
            ret = FALSE;
        else if (!ret_val)
            trace("SleepConditionVariableCS failed, last err %ld\n", GetLastError());
        else
            continue;

        break;
    }

    clientside_provider_callback_cv = NULL;
    LeaveCriticalSection(&clientside_provider_callback_cs);
    return ret;
}

static void test_UiaRegisterProviderCallback(void)
{
    HWND hwnd, hwnd2;
    WNDCLASSA cls;
    HUIANODE node;
    HRESULT hr;
    VARIANT v;

    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "UiaRegisterProviderCallback class";

    RegisterClassA(&cls);

    cls.lpfnWndProc = child_test_wnd_proc;
    cls.lpszClassName = "UiaRegisterProviderCallback child class";
    RegisterClassA(&cls);

    hwnd = CreateWindowA("UiaRegisterProviderCallback class", "Test window", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);

    hwnd2 = CreateWindowA("UiaRegisterProviderCallback child class", "Test child window", WS_CHILD,
            0, 0, 100, 100, hwnd, NULL, NULL, NULL);

    UiaRegisterProviderCallback(test_uia_provider_callback);

    /* No providers returned by UiaRootObjectId or the provider callback. */
    Provider_proxy.hwnd = hwnd2;
    child_win_prov_root = prov_root = NULL;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_proxy);
    SET_EXPECT(prov_callback_parent_proxy);
    hr = UiaNodeFromHandle(hwnd2, &node);
    /* Windows 7 returns S_OK with a NULL HUIANODE. */
    ok(hr == E_FAIL || broken(hr == S_OK), "Unexpected hr %#lx.\n", hr);
    ok(!node, "node != NULL\n");
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    CHECK_CALLED(prov_callback_proxy);
    todo_wine CHECK_CALLED(prov_callback_parent_proxy);

    /* Return only nonclient proxy provider. */
    base_hwnd_prov = proxy_prov = parent_proxy_prov = NULL;
    nc_prov = &Provider_nc.IRawElementProviderSimple_iface;
    child_win_prov_root = prov_root = NULL;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_proxy);
    SET_EXPECT(prov_callback_parent_proxy);
    hr = UiaNodeFromHandle(hwnd2, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_nc.ref == 2, "Unexpected refcnt %ld\n", Provider_nc.ref);
    ok(!!node, "node == NULL\n");
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    CHECK_CALLED(prov_callback_proxy);
    todo_wine CHECK_CALLED(prov_callback_parent_proxy);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd2);
    check_node_provider_desc(V_BSTR(&v), L"Nonclient", L"Provider_nc", TRUE);
    VariantClear(&v);

    ok_method_sequence(reg_prov_cb1, "reg_prov_cb1");

    UiaNodeRelease(node);
    ok(Provider_nc.ref == 1, "Unexpected refcnt %ld\n", Provider_nc.ref);

    /* Return only base_hwnd provider. */
    nc_prov = proxy_prov = parent_proxy_prov = NULL;
    base_hwnd_prov = &Provider_hwnd.IRawElementProviderSimple_iface;
    child_win_prov_root = prov_root = NULL;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_proxy);
    SET_EXPECT(prov_callback_parent_proxy);
    hr = UiaNodeFromHandle(hwnd2, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_hwnd.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    ok(!!node, "node == NULL\n");
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    CHECK_CALLED(prov_callback_proxy);
    todo_wine CHECK_CALLED(prov_callback_parent_proxy);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd2);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider_hwnd", TRUE);
    VariantClear(&v);

    ok_method_sequence(reg_prov_cb2, "reg_prov_cb2");

    UiaNodeRelease(node);
    ok(Provider_hwnd.ref == 1, "Unexpected refcnt %ld\n", Provider_hwnd.ref);

    /* Return providers for all ProviderTypes. */
    base_hwnd_prov = &Provider_hwnd.IRawElementProviderSimple_iface;
    proxy_prov = &Provider_proxy.IRawElementProviderSimple_iface;
    parent_proxy_prov = &Provider_proxy2.IRawElementProviderSimple_iface;
    nc_prov = &Provider_nc.IRawElementProviderSimple_iface;
    Provider_proxy.hwnd = hwnd2;
    child_win_prov_root = prov_root = NULL;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_proxy);
    SET_EXPECT(prov_callback_parent_proxy);
    hr = UiaNodeFromHandle(hwnd2, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!node, "node == NULL\n");
    ok(Provider_proxy.ref == 2, "Unexpected refcnt %ld\n", Provider_proxy.ref);
    ok(Provider_proxy2.ref == 1, "Unexpected refcnt %ld\n", Provider_proxy2.ref);
    ok(Provider_nc.ref == 2, "Unexpected refcnt %ld\n", Provider_nc.ref);
    ok(Provider_hwnd.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    CHECK_CALLED(prov_callback_proxy);
    todo_wine CHECK_CALLED(prov_callback_parent_proxy);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd2);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_proxy", TRUE);
    check_node_provider_desc(V_BSTR(&v), L"Nonclient", L"Provider_nc", FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider_hwnd", FALSE);
    VariantClear(&v);

    ok_method_sequence(reg_prov_cb3, "reg_prov_cb3");

    UiaNodeRelease(node);
    ok(Provider_proxy.ref == 1, "Unexpected refcnt %ld\n", Provider_proxy.ref);
    ok(Provider_nc.ref == 1, "Unexpected refcnt %ld\n", Provider_nc.ref);
    ok(Provider_hwnd.ref == 1, "Unexpected refcnt %ld\n", Provider_hwnd.ref);

    /* Return an override provider from Provider_proxy2. */
    base_hwnd_prov = &Provider_hwnd.IRawElementProviderSimple_iface;
    proxy_prov = &Provider_proxy.IRawElementProviderSimple_iface;
    parent_proxy_prov = &Provider_proxy2.IRawElementProviderSimple_iface;
    nc_prov = &Provider_nc.IRawElementProviderSimple_iface;
    Provider_proxy2.override_hwnd = hwnd2;
    child_win_prov_root = prov_root = NULL;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_proxy);
    SET_EXPECT(prov_callback_parent_proxy);
    hr = UiaNodeFromHandle(hwnd2, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!node, "node == NULL\n");
    ok(Provider_proxy.ref == 2, "Unexpected refcnt %ld\n", Provider_proxy.ref);
    ok(Provider_proxy2.ref == 1, "Unexpected refcnt %ld\n", Provider_proxy2.ref);
    ok(Provider_nc.ref == 2, "Unexpected refcnt %ld\n", Provider_nc.ref);
    ok(Provider_hwnd.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    todo_wine ok(Provider_override.ref == 2, "Unexpected refcnt %ld\n", Provider_override.ref);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    CHECK_CALLED(prov_callback_proxy);
    todo_wine CHECK_CALLED(prov_callback_parent_proxy);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd2);
    check_node_provider_desc_todo(V_BSTR(&v), L"Override", L"Provider_override", TRUE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Main", L"Provider_proxy", FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Nonclient", L"Provider_nc", FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider_hwnd", FALSE);
    VariantClear(&v);

    ok_method_sequence(reg_prov_cb4, "reg_prov_cb4");

    /*
     * Test the order that Providers are queried for properties. The order is:
     * Override provider.
     * Main provider.
     * Nonclient provider.
     * Hwnd provider.
     *
     * UI Automation tries to get a property from each in this order until one
     * returns a value. If none do, the property isn't supported.
     */
    if (Provider_override.ref == 2)
    {
        hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
        ok(V_VT(&v) == VT_I4, "Unexpected vt %d\n", V_VT(&v));
        ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));
        ok_method_sequence(reg_prov_cb5, "reg_prov_cb5");
    }

    /* Property retrieved from Provider_proxy (Main) */
    Provider_override.ret_invalid_prop_type = TRUE;
    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected vt %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));
    ok_method_sequence(reg_prov_cb6, "reg_prov_cb6");

    /* Property retrieved from Provider_nc (Nonclient) */
    Provider_override.ret_invalid_prop_type = Provider_proxy.ret_invalid_prop_type = TRUE;
    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected vt %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));
    ok_method_sequence(reg_prov_cb7, "reg_prov_cb7");

    /* Property retrieved from Provider_hwnd (Hwnd) */
    Provider_override.ret_invalid_prop_type = Provider_proxy.ret_invalid_prop_type = TRUE;
    Provider_nc.ret_invalid_prop_type = TRUE;
    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected vt %d\n", V_VT(&v));
    ok(V_I4(&v) == uia_i4_prop_val, "Unexpected I4 %#lx\n", V_I4(&v));
    ok_method_sequence(reg_prov_cb8, "reg_prov_cb8");

    /* Property retrieved from none of the providers. */
    Provider_override.ret_invalid_prop_type = Provider_proxy.ret_invalid_prop_type = TRUE;
    Provider_nc.ret_invalid_prop_type = Provider_hwnd.ret_invalid_prop_type = TRUE;
    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok_method_sequence(reg_prov_cb9, "reg_prov_cb9");

    UiaNodeRelease(node);
    ok(Provider_proxy.ref == 1, "Unexpected refcnt %ld\n", Provider_proxy.ref);
    ok(Provider_nc.ref == 1, "Unexpected refcnt %ld\n", Provider_nc.ref);
    ok(Provider_hwnd.ref == 1, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    ok(Provider_override.ref == 1, "Unexpected refcnt %ld\n", Provider_override.ref);
    Provider_override.ret_invalid_prop_type = Provider_proxy.ret_invalid_prop_type = FALSE;
    Provider_nc.ret_invalid_prop_type = Provider_hwnd.ret_invalid_prop_type = FALSE;

    /*
     * Provider_hwnd has ProviderOptions_UseComThreading, and COM hasn't been
     * initialized. One provider failing will cause the entire node to fail
     * creation on Win10+.
     */
    Provider_hwnd.prov_opts = ProviderOptions_ClientSideProvider | ProviderOptions_UseComThreading;
    node = (void *)0xdeadbeef;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_proxy);
    SET_EXPECT(prov_callback_parent_proxy);
    hr = UiaNodeFromHandle(hwnd2, &node);
    ok(hr == CO_E_NOTINITIALIZED || broken(hr == S_OK), "Unexpected hr %#lx.\n", hr);
    ok(!node || broken(!!node), "node != NULL\n");
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    CHECK_CALLED(prov_callback_proxy);
    todo_wine CHECK_CALLED(prov_callback_parent_proxy);
    ok_method_sequence(reg_prov_cb10, "reg_prov_cb10");
    UiaNodeRelease(node);
    Provider_hwnd.prov_opts = ProviderOptions_ClientSideProvider;

    /*
     * Provider returned by UiaRootObjectId on hwnd2. No ProviderType_Proxy
     * callback for hwnd2.
     */
    Provider.hwnd = hwnd2;
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    child_win_prov_root = &Provider.IRawElementProviderSimple_iface;
    Provider_proxy2.override_hwnd = NULL;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_parent_proxy);
    hr = UiaNodeFromHandle(hwnd2, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    todo_wine CHECK_CALLED(prov_callback_parent_proxy);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd2);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", TRUE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider_hwnd", FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Nonclient", L"Provider_nc", FALSE);
    VariantClear(&v);

    ok_method_sequence(reg_prov_cb11, "reg_prov_cb11");

    UiaNodeRelease(node);

    /*
     * Provider returned by UiaRootObjectId on both HWNDs. Since Provider2
     * doesn't give an HWND override provider, UIA will attempt to get a proxy
     * provider to check it for an HWND override provider.
     */
    Provider.hwnd = hwnd2;
    Provider2.hwnd = hwnd;
    Provider.prov_opts = Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    child_win_prov_root = &Provider.IRawElementProviderSimple_iface;
    prov_root = &Provider2.IRawElementProviderSimple_iface;
    Provider_proxy2.override_hwnd = NULL;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_parent_proxy);
    hr = UiaNodeFromHandle(hwnd2, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    todo_wine CHECK_CALLED(prov_callback_parent_proxy);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd2);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", TRUE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider_hwnd", FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Nonclient", L"Provider_nc", FALSE);
    VariantClear(&v);

    ok_method_sequence(reg_prov_cb12, "reg_prov_cb12");

    UiaNodeRelease(node);

    /*
     * Provider returned by UiaRootObjectId on both HWNDs. Since Provider2
     * returns an HWND override, no ProviderType_Proxy callback for hwnd.
     */
    Provider.hwnd = hwnd2;
    Provider2.hwnd = hwnd;
    Provider2.override_hwnd = Provider_override.hwnd = hwnd2;
    Provider2.ignore_hwnd_prop = Provider_override.ignore_hwnd_prop = TRUE;
    Provider.prov_opts = Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    child_win_prov_root = &Provider.IRawElementProviderSimple_iface;
    prov_root = &Provider2.IRawElementProviderSimple_iface;
    Provider_proxy2.override_hwnd = NULL;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_parent_proxy);
    hr = UiaNodeFromHandle(hwnd2, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd2);
    check_node_provider_desc_todo(V_BSTR(&v), L"Override", L"Provider_override", TRUE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Main", L"Provider", FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider_hwnd", FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Nonclient", L"Provider_nc", FALSE);
    VariantClear(&v);

    ok_method_sequence(reg_prov_cb13, "reg_prov_cb13");

    UiaNodeRelease(node);

    Provider2.ignore_hwnd_prop = Provider_override.ignore_hwnd_prop = FALSE;
    UiaRegisterProviderCallback(NULL);

    DestroyWindow(hwnd);
    UnregisterClassA("UiaRegisterProviderCallback class", NULL);
    UnregisterClassA("UiaRegisterProviderCallback child class", NULL);
}

static void set_cache_request(struct UiaCacheRequest *req, struct UiaCondition *view_cond, int scope,
        PROPERTYID *prop_ids, int prop_ids_count, PATTERNID *pattern_ids, int pattern_ids_count, int elem_mode)
{
    req->pViewCondition = view_cond;
    req->Scope = scope;

    req->pProperties = prop_ids;
    req->cProperties = prop_ids_count;
    req->pPatterns = pattern_ids;
    req->cPatterns = pattern_ids_count;

    req->automationElementMode = elem_mode;
}

static void set_property_condition(struct UiaPropertyCondition *cond, int prop_id, VARIANT *val, int flags)
{
    cond->ConditionType = ConditionType_Property;
    cond->PropertyId = prop_id;
    cond->Value = *val;
    cond->Flags = flags;
}

static void set_and_or_condition(struct UiaAndOrCondition *cond, int cond_type,
        struct UiaCondition **conds, int cond_count)
{
    cond->ConditionType = cond_type;
    cond->ppConditions = conds;
    cond->cConditions = cond_count;
}

static void set_not_condition(struct UiaNotCondition *cond, struct UiaCondition *not_cond)
{
    cond->ConditionType = ConditionType_Not;
    cond->pConditions = not_cond;
}

#define MAX_NODE_PROVIDERS 4
struct node_provider_desc {
    DWORD pid;
    HWND hwnd;

    const WCHAR *prov_type[MAX_NODE_PROVIDERS];
    const WCHAR *prov_name[MAX_NODE_PROVIDERS];
    BOOL parent_link[MAX_NODE_PROVIDERS];
    struct node_provider_desc *nested_desc[MAX_NODE_PROVIDERS];
    int prov_count;
};

static void init_node_provider_desc(struct node_provider_desc *desc, DWORD pid, HWND hwnd)
{
    memset(desc, 0, sizeof(*desc));
    desc->pid = pid;
    desc->hwnd = hwnd;
}

static void add_nested_provider_desc(struct node_provider_desc *desc, const WCHAR *prov_type, const WCHAR *prov_name,
        BOOL parent_link, struct node_provider_desc *nested_desc)
{
    desc->prov_type[desc->prov_count] = prov_type;
    desc->prov_name[desc->prov_count] = prov_name;
    desc->parent_link[desc->prov_count] = parent_link;
    desc->nested_desc[desc->prov_count] = nested_desc;
    desc->prov_count++;
}

static void add_provider_desc(struct node_provider_desc *desc, const WCHAR *prov_type, const WCHAR *prov_name,
        BOOL parent_link)
{
    add_nested_provider_desc(desc, prov_type, prov_name, parent_link, NULL);
}

#define test_node_provider_desc( desc, desc_str ) \
        test_node_provider_desc_( (desc), (desc_str), __FILE__, __LINE__)
static void test_node_provider_desc_(struct node_provider_desc *desc, BSTR desc_str, const char *file, int line)
{
    int i;

    check_node_provider_desc_prefix_(desc_str, desc->pid, desc->hwnd, file, line);
    for (i = 0; i < desc->prov_count; i++)
    {
        if (desc->nested_desc[i])
        {
            WCHAR buf[2048];

            ok_(file, line)(get_nested_provider_desc(desc_str, desc->prov_type[i], desc->parent_link[i], buf),
                    "Failed to get nested provider description\n");
            test_node_provider_desc_(desc->nested_desc[i], buf, file, line);
        }
        else
            check_node_provider_desc_(desc_str, desc->prov_type[i], desc->prov_name[i], desc->parent_link[i], FALSE, file, line);
    }
}

/*
 * Cache requests are returned as a two dimensional safearray, with the first
 * dimension being the element index, and the second index being the
 * node/property/pattern value index for the element. The first element value is
 * always an HUIANODE, followed by requested property values, and finally
 * requested pattern handles.
 */
#define test_cache_req_sa( sa, exp_lbound, exp_elems, exp_node_desc ) \
        test_cache_req_sa_( (sa), (exp_lbound), (exp_elems), (exp_node_desc), __FILE__, __LINE__)
static void test_cache_req_sa_(SAFEARRAY *sa, LONG exp_lbound[2], LONG exp_elems[2],
        struct node_provider_desc *exp_node_desc, const char *file, int line)
{
    HUIANODE node;
    HRESULT hr;
    VARTYPE vt;
    VARIANT v;
    UINT dims;
    int i;

    hr = SafeArrayGetVartype(sa, &vt);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(file, line)(vt == VT_VARIANT, "Unexpected vt %d\n", vt);

    dims = SafeArrayGetDim(sa);
    ok_(file, line)(dims == 2, "Unexpected dims %d\n", dims);

    for (i = 0; i < 2; i++)
    {
        LONG lbound, ubound, elems;

        lbound = ubound = elems = 0;
        hr = SafeArrayGetLBound(sa, 1 + i, &lbound);
        ok_(file, line)(hr == S_OK, "Unexpected hr %#lx for SafeArrayGetLBound\n", hr);
        ok_(file, line)(exp_lbound[i] == lbound, "Unexpected lbound[%d] %ld\n", i, lbound);

        hr = SafeArrayGetUBound(sa, 1 + i, &ubound);
        ok_(file, line)(hr == S_OK, "Unexpected hr %#lx for SafeArrayGetUBound\n", hr);

        elems = (ubound - lbound) + 1;
        ok_(file, line)(exp_elems[i] == elems, "Unexpected elems[%d] %ld\n", i, elems);
    }

    for (i = 0; i < exp_elems[0]; i++)
    {
        LONG idx[2] = { (exp_lbound[0] + i), exp_lbound[1] };

        hr = SafeArrayGetElement(sa, idx, &v);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

        hr = UiaHUiaNodeFromVariant(&v, &node);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
        VariantClear(&v);

        hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
        test_node_provider_desc_(&exp_node_desc[i], V_BSTR(&v), file, line);
        VariantClear(&v);

        UiaNodeRelease(node);
    }
}

#define test_provider_event_advise_added( prov, event_id, todo) \
        test_provider_event_advise_added_( (prov), (event_id), (todo), __FILE__, __LINE__)
static void test_provider_event_advise_added_(struct Provider *prov, int event_id, BOOL todo, const char *file, int line)
{
    todo_wine_if (todo) ok_(file, line)(prov->advise_events_added_event_id == event_id, "%s: Unexpected advise event added, event ID %d.\n",
            prov->prov_name, prov->advise_events_added_event_id);
}

static const struct prov_method_sequence cache_req_seq1[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId. */
    { 0 }
};

/*
 * Win10v1507 and below will attempt to do parent navigation if the
 * conditional check fails.
 */
static const struct prov_method_sequence cache_req_seq2[] = {
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { 0 }
};

static const struct prov_method_sequence cache_req_seq3[] = {
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    /* Navigates towards parent to check for clientside provider siblings. */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId. */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId. */
    { 0 }
};

static const struct prov_method_sequence cache_req_seq4[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* Dependent upon property condition. */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId. */
    { 0 }
};

/* Sequence for non-matching property condition. */
static const struct prov_method_sequence cache_req_seq5[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* Dependent upon property condition. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* Dependent upon property condition. */
    /* Only done on Win10v1507 and below. */
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { 0 }
};

static const struct prov_method_sequence cache_req_seq6[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId. */
    /* Only done on Win10v1507 and below. */
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { 0 }
};

static const struct prov_method_sequence cache_req_seq7[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId. */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId. */
    { 0 }
};

static const struct prov_method_sequence cache_req_seq8[] = {
    NODE_CREATE_SEQ(&Provider_child),
    { 0 }
};

static const struct prov_method_sequence cache_req_seq9[] = {
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    /* Done twice on Windows, but we shouldn't need to replicate this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_IsControlElementPropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    /* Only done on Win10v1507 and below. */
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { 0 }
};

static const struct prov_method_sequence cache_req_seq10[] = {
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    /* Done twice on Windows, but we shouldn't need to replicate this. */
    { &Provider_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_IsControlElementPropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId. */
    { 0 }
};

static const struct UiaCondition UiaTrueCondition  = { ConditionType_True };
static const struct UiaCondition UiaFalseCondition = { ConditionType_False };
static void test_UiaGetUpdatedCache(void)
{
    LONG exp_lbound[2], exp_elems[2], idx[2], i;
    struct Provider_prop_override prop_override;
    struct node_provider_desc exp_node_desc[2];
    struct UiaPropertyCondition prop_cond;
    struct UiaAndOrCondition and_or_cond;
    struct UiaCacheRequest cache_req;
    struct UiaCondition *cond_arr[2];
    struct UiaNotCondition not_cond;
    VARIANT v, v_arr[2];
    SAFEARRAY *out_req;
    IUnknown *unk_ns;
    BSTR tree_struct;
    int prop_ids[2];
    HUIANODE node;
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = UiaGetReservedNotSupportedValue(&unk_ns);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);

    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = NULL;
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", TRUE);
    VariantClear(&v);

    ok_method_sequence(node_from_prov2, NULL);

    /* NULL arg tests. */
    set_cache_request(&cache_req, NULL, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_None, NULL, &out_req, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaGetUpdatedCache(node, NULL, NormalizeState_None, NULL, &out_req, &tree_struct);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaGetUpdatedCache(NULL, &cache_req, NormalizeState_None, NULL, &out_req, &tree_struct);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /*
     * Cache request with NULL view condition, doesn't matter with
     * NormalizeState_None as passed in HUIANODE isn't evaluated against any
     * condition.
     */
    tree_struct = NULL; out_req = NULL;
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider", TRUE);
    set_cache_request(&cache_req, NULL, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_None, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);
    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(cache_req_seq1, "cache_req_seq1");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    /*
     * NormalizeState_View, HUIANODE gets checked against the ConditionType_False
     * condition within the cache request structure, nothing is returned.
     */
    tree_struct = NULL; out_req = NULL;
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaFalseCondition, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!out_req, "out_req != NULL\n");

    /* Empty tree structure string. */
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(!wcscmp(tree_struct, L""), "tree structure %s\n", debugstr_w(tree_struct));
    SysFreeString(tree_struct);
    ok_method_sequence(cache_req_seq2, "cache_req_seq2");

    /*
     * NormalizeState_View, HUIANODE gets checked against the ConditionType_True
     * condition within the cache request structure, returns this HUIANODE.
     */
    tree_struct = NULL; out_req = NULL;
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);
    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(cache_req_seq1, "cache_req_seq1");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    /*
     * NormalizeState_Custom, HUIANODE gets checked against our passed in
     * ConditionType_False condition.
     */
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_Custom, (struct UiaCondition *)&UiaFalseCondition, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!out_req, "out_req != NULL\n");

    /* Empty tree structure string. */
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(!wcscmp(tree_struct, L""), "tree structure %s\n", debugstr_w(tree_struct));
    SysFreeString(tree_struct);

    ok_method_sequence(cache_req_seq2, "cache_req_seq2");

    /*
     * NormalizeState_Custom, HUIANODE gets checked against the ConditionType_True
     * condition we pass in, returns this HUIANODE.
     */
    tree_struct = NULL; out_req = NULL;
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_Custom, (struct UiaCondition *)&UiaTrueCondition, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);
    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(cache_req_seq1, "cache_req_seq1");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    /*
     * CacheRequest with TreeScope_Children.
     */
    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_child", TRUE);
    add_provider_desc(&exp_node_desc[1], L"Main", L"Provider_child2", TRUE);
    tree_struct = NULL; out_req = NULL;
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Children, NULL, 0, NULL, 0, AutomationElementMode_Full);
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_None, NULL, &out_req, &tree_struct);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(!!out_req, "out_req == NULL\n");
    todo_wine ok(!!tree_struct, "tree_struct == NULL\n");
    todo_wine ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    todo_wine ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);
    if (out_req)
    {
        exp_elems[0] = 2;
        exp_elems[1] = 1;
        test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

        ok(!wcscmp(tree_struct, L"(P)P))"), "tree structure %s\n", debugstr_w(tree_struct));
        ok_method_sequence(cache_req_seq3, "cache_req_seq3");
    }

    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(Provider_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_child2.ref);
    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    /*
     * ConditionType_And tests.
     */
    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider", TRUE);
    cond_arr[0] = (struct UiaCondition *)&UiaTrueCondition;
    cond_arr[1] = (struct UiaCondition *)&UiaTrueCondition;
    set_and_or_condition(&and_or_cond, ConditionType_And, cond_arr, ARRAY_SIZE(cond_arr));
    set_cache_request(&cache_req, (struct UiaCondition *)&and_or_cond, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;

    /* Equivalent to: if (1 && 1) */
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);
    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(cache_req_seq1, NULL);

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    cond_arr[0] = (struct UiaCondition *)&UiaTrueCondition;
    cond_arr[1] = (struct UiaCondition *)&UiaFalseCondition;
    set_and_or_condition(&and_or_cond, ConditionType_And, cond_arr, ARRAY_SIZE(cond_arr));
    set_cache_request(&cache_req, (struct UiaCondition *)&and_or_cond, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;

    /* Equivalent to: if (1 && 0) */
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_method_sequence(cache_req_seq2, NULL);
    ok(!out_req, "out_req != NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(!wcscmp(tree_struct, L""), "tree structure %s\n", debugstr_w(tree_struct));

    SysFreeString(tree_struct);

    /*
     * ConditionType_Or tests.
     */
    cond_arr[0] = (struct UiaCondition *)&UiaTrueCondition;
    cond_arr[1] = (struct UiaCondition *)&UiaFalseCondition;
    set_and_or_condition(&and_or_cond, ConditionType_Or, cond_arr, ARRAY_SIZE(cond_arr));
    set_cache_request(&cache_req, (struct UiaCondition *)&and_or_cond, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;

    /* Equivalent to: if (1 || 0) */
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);
    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(cache_req_seq1, NULL);

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    cond_arr[0] = (struct UiaCondition *)&UiaFalseCondition;
    cond_arr[1] = (struct UiaCondition *)&UiaFalseCondition;
    set_and_or_condition(&and_or_cond, ConditionType_Or, cond_arr, ARRAY_SIZE(cond_arr));
    set_cache_request(&cache_req, (struct UiaCondition *)&and_or_cond, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;

    /* Equivalent to: if (0 || 0) */
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_method_sequence(cache_req_seq2, NULL);
    ok(!out_req, "out_req != NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(!wcscmp(tree_struct, L""), "tree structure %s\n", debugstr_w(tree_struct));

    SysFreeString(tree_struct);

    /*
     * ConditionType_Not tests.
     */
    set_not_condition(&not_cond, (struct UiaCondition *)&UiaFalseCondition);
    set_cache_request(&cache_req, (struct UiaCondition *)&not_cond, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;

    /* Equivalent to: if (!0) */
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);
    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(cache_req_seq1, NULL);

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    set_not_condition(&not_cond, (struct UiaCondition *)&UiaTrueCondition);
    set_cache_request(&cache_req, (struct UiaCondition *)&not_cond, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;

    /* Equivalent to: if (!1) */
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!out_req, "out_req != NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(!wcscmp(tree_struct, L""), "tree structure %s\n", debugstr_w(tree_struct));
    SysFreeString(tree_struct);
    ok_method_sequence(cache_req_seq2, NULL);

    /*
     * ConditionType_Property tests.
     */
    Provider.ret_invalid_prop_type = FALSE;

    /* Test UIAutomationType_IntArray property conditions. */
    if (UiaLookupId(AutomationIdentifierType_Property, &OutlineColor_Property_GUID))
    {
        V_VT(&v) = VT_I4 | VT_ARRAY;
        V_ARRAY(&v) = create_i4_safearray();
        set_property_condition(&prop_cond, UIA_OutlineColorPropertyId, &v, PropertyConditionFlags_None);
        set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond, TreeScope_Element, NULL, 0, NULL, 0,
                AutomationElementMode_Full);
        tree_struct = NULL; out_req = NULL;

        hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!!out_req, "out_req == NULL\n");
        ok(!!tree_struct, "tree_struct == NULL\n");

        exp_lbound[0] = exp_lbound[1] = 0;
        exp_elems[0] = exp_elems[1] = 1;
        test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);
        ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
        ok_method_sequence(cache_req_seq4, NULL);

        SafeArrayDestroy(out_req);
        SysFreeString(tree_struct);
        VariantClear(&v);

        /* Same values, except we're short by one element. */
        V_VT(&v) = VT_I4 | VT_ARRAY;
        V_ARRAY(&v) = SafeArrayCreateVector(VT_I4, 0, ARRAY_SIZE(uia_i4_arr_prop_val) - 1);

        for (i = 0; i < ARRAY_SIZE(uia_i4_arr_prop_val) - 1; i++)
            SafeArrayPutElement(V_ARRAY(&prop_cond.Value), &i, (void *)&uia_i4_arr_prop_val[i]);

        set_property_condition(&prop_cond, UIA_OutlineColorPropertyId, &v, PropertyConditionFlags_None);
        set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond, TreeScope_Element, NULL, 0, NULL, 0,
                AutomationElementMode_Full);
        tree_struct = NULL; out_req = NULL;

        hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok_method_sequence(cache_req_seq5, NULL);
        ok(!out_req, "out_req != NULL\n");
        ok(!!tree_struct, "tree_struct == NULL\n");
        ok(!wcscmp(tree_struct, L""), "tree structure %s\n", debugstr_w(tree_struct));

        SysFreeString(tree_struct);
        VariantClear(&v);
    }
    else
        win_skip("UIA_OutlineColorPropertyId unavailable, skipping property condition tests for it.\n");

    /* UIA_RuntimeIdPropertyId comparison. */
    Provider.runtime_id[0] = 0xdeadbeef; Provider.runtime_id[1] = 0xfeedbeef;
    V_VT(&v) = VT_I4 | VT_ARRAY;
    V_ARRAY(&v) = SafeArrayCreateVector(VT_I4, 0, ARRAY_SIZE(Provider.runtime_id));
    for (i = 0; i < ARRAY_SIZE(Provider.runtime_id); i++)
        SafeArrayPutElement(V_ARRAY(&v), &i, (void *)&Provider.runtime_id[i]);

    set_property_condition(&prop_cond, UIA_RuntimeIdPropertyId, &v, PropertyConditionFlags_None);
    set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;

    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);
    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(cache_req_seq6, NULL);

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);
    VariantClear(&prop_cond.Value);

    /* UIAutomationType_Bool property condition tests. */
    prop_override.prop_id = UIA_IsControlElementPropertyId;
    V_VT(&prop_override.val) = VT_BOOL;
    V_BOOL(&prop_override.val) = VARIANT_FALSE;
    Provider.prop_override = &prop_override;
    Provider.prop_override_count = 1;

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    set_property_condition(&prop_cond, UIA_IsControlElementPropertyId, &v, PropertyConditionFlags_None);
    set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;

    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);
    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(cache_req_seq4, NULL);

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);
    VariantClear(&v);

    /*
     * Provider now returns VARIANT_TRUE for UIA_IsControlElementPropertyId,
     * conditional check will fail.
     */
    prop_override.prop_id = UIA_IsControlElementPropertyId;
    V_VT(&prop_override.val) = VT_BOOL;
    V_BOOL(&prop_override.val) = VARIANT_TRUE;
    Provider.prop_override = &prop_override;
    Provider.prop_override_count = 1;

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    set_property_condition(&prop_cond, UIA_IsControlElementPropertyId, &v, PropertyConditionFlags_None);
    set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;

    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_method_sequence(cache_req_seq5, NULL);
    ok(!out_req, "out_req != NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(!wcscmp(tree_struct, L""), "tree structure %s\n", debugstr_w(tree_struct));

    SysFreeString(tree_struct);
    VariantClear(&v);

    Provider.prop_override = NULL;
    Provider.prop_override_count = 0;

    /*
     * Tests for property value caching.
     */
    prop_ids[0] = UIA_RuntimeIdPropertyId;
    /* Invalid property ID, no work will be done. */
    prop_ids[1] = 1;
    tree_struct = NULL; out_req = NULL;
    set_cache_request(&cache_req, NULL, TreeScope_Element, prop_ids, ARRAY_SIZE(prop_ids), NULL, 0, AutomationElementMode_Full);
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_None, NULL, &out_req, &tree_struct);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!out_req, "out_req != NULL\n");
    ok(!tree_struct, "tree_struct != NULL\n");

    /*
     * Retrieve values for UIA_RuntimeIdPropertyId and
     * UIA_IsControlElementPropertyId in the returned cache.
     */
    prop_ids[0] = UIA_RuntimeIdPropertyId;
    prop_ids[1] = UIA_IsControlElementPropertyId;
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, FALSE);
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider", TRUE);

    tree_struct = NULL; out_req = NULL;
    set_cache_request(&cache_req, NULL, TreeScope_Element, prop_ids, ARRAY_SIZE(prop_ids), NULL, 0, AutomationElementMode_Full);
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_None, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 1;
    exp_elems[1] = 3;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);
    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));

    idx[0] = 0;
    for (i = 0; i < ARRAY_SIZE(prop_ids); i++)
    {
        idx[1] = 1 + i;
        VariantInit(&v_arr[i]);
        hr = SafeArrayGetElement(out_req, idx, &v_arr[i]);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    }

    ok(V_VT(&v_arr[0]) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(V_UNKNOWN(&v_arr[0]) == unk_ns, "unexpected IUnknown %p\n", V_UNKNOWN(&v_arr[0]));
    VariantClear(&v_arr[0]);

    ok(check_variant_bool(&v_arr[1], TRUE), "V_BOOL(&v) = %#x\n", V_BOOL(&v_arr[1]));
    VariantClear(&v_arr[1]);

    ok_method_sequence(cache_req_seq7, "cache_req_seq7");
    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    /*
     * Again, but return a valid runtime ID and a different value for
     * UIA_IsControlElementPropertyId.
     */
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    set_property_override(&prop_override, UIA_IsControlElementPropertyId, &v);
    set_provider_prop_override(&Provider, &prop_override, 1);
    Provider.runtime_id[0] = Provider.runtime_id[1] = 0xdeadbeef;
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider", TRUE);

    tree_struct = NULL; out_req = NULL;
    set_cache_request(&cache_req, NULL, TreeScope_Element, prop_ids, ARRAY_SIZE(prop_ids), NULL, 0, AutomationElementMode_Full);
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_None, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 1;
    exp_elems[1] = 3;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);
    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));

    idx[0] = 0;
    for (i = 0; i < ARRAY_SIZE(prop_ids); i++)
    {
        idx[1] = 1 + i;
        VariantInit(&v_arr[i]);
        hr = SafeArrayGetElement(out_req, idx, &v_arr[i]);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    }

    ok(V_VT(&v_arr[0]) == (VT_I4 | VT_ARRAY), "Unexpected vt %d\n", V_VT(&v_arr[0]));
    check_runtime_id(Provider.runtime_id, ARRAY_SIZE(Provider.runtime_id), V_ARRAY(&v_arr[0]));
    VariantClear(&v_arr[0]);

    ok(check_variant_bool(&v_arr[1], FALSE), "V_BOOL(&v) = %#x\n", V_BOOL(&v_arr[1]));
    VariantClear(&v_arr[1]);

    ok_method_sequence(cache_req_seq7, "cache_req_seq7");
    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    /* Normalization navigation tests. */
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, FALSE);
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, FALSE);

    hr = UiaNodeFromProvider(&Provider_child.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok_method_sequence(cache_req_seq8, "cache_req_seq8");

    /*
     * Neither Provider_child or Provider match this condition, return
     * nothing.
     */
    variant_init_bool(&v, FALSE);
    set_property_condition(&prop_cond, UIA_IsControlElementPropertyId, &v, PropertyConditionFlags_None);
    set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!out_req, "out_req != NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(!wcscmp(tree_struct, L""), "tree structure %s\n", debugstr_w(tree_struct));
    SysFreeString(tree_struct);
    ok_method_sequence(cache_req_seq9, "cache_req_seq9");

    /*
     * Provider now matches our condition, we'll get Provider in the cache
     * request.
     */
    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsControlElementPropertyId, &v);
    set_provider_prop_override(&Provider, &prop_override, 1);
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider", TRUE);

    set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;
    hr = UiaGetUpdatedCache(node, &cache_req, NormalizeState_View, NULL, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);
    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(cache_req_seq10, "cache_req_seq10");

    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);
    VariantClear(&v);

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, FALSE);
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, FALSE);

    IUnknown_Release(unk_ns);
    CoUninitialize();
}

static const struct prov_method_sequence nav_seq1[] = {
    NODE_CREATE_SEQ2(&Provider),
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Only done on Win10v1809+ */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Windows 10+ calls these. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq2[] = {
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_hwnd_child),
    { &Provider_hwnd_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq3[] = {
    { &Provider_hwnd_child, FRAG_NAVIGATE}, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_hwnd_child2),
    { &Provider_hwnd_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq4[] = {
    { &Provider_hwnd_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_hwnd_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS },
    /* All Windows versions use the NativeWindowHandle provider type check here. */
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    NODE_CREATE_SEQ2(&Provider),
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Only done on Win10v1809+ */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Windows 10+ calls these. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    { &Provider_nc_child, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider_nc_child, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_nc_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    { &Provider_nc_child, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_nc_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only called on Windows versions past Win10v1507. */
    { &Provider_nc_child, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq5[] = {
    { &Provider_nc_child, FRAG_NAVIGATE }, /* NavigateDirection_PreviousSibling */
    { &Provider_nc_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS },
    { &Provider_nc, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    NODE_CREATE_SEQ2(&Provider),
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Only done on Win10v1809+ */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Windows 10+ calls these. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_LastChild */
    NODE_CREATE_SEQ(&Provider_hwnd_child2),
    { &Provider_hwnd_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq6[] = {
    { &Provider_nc_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_nc_child2),
    { &Provider_nc_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq7[] = {
    { &Provider_nc_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_nc_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS },
    { &Provider_nc, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    NODE_CREATE_SEQ2(&Provider),
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Only done on Win10v1809+ */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Windows 10+ calls these. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq8[] = {
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq9[] = {
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    NODE_CREATE_SEQ2(&Provider),
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Only done on Win10v1809+ */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Windows 10+ calls these. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    /* Navigates to parent a second time. */
    { &Provider_child2, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_TODO },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Win10v1507 and below call this. */
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_TODO },
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    /* Only done on Win10v1809+ */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    { &Provider_hwnd, FRAG_NAVIGATE, METHOD_TODO }, /* NavigateDirection_Parent */
    /* Windows 10+ calls these. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
};

static const struct prov_method_sequence nav_seq10[] = {
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_LastChild */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq11[] = {
    { &Provider_hwnd, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    { &Provider_hwnd_child, PROV_GET_PROVIDER_OPTIONS },
    /* All Windows versions use the NativeWindowHandle provider type check here. */
    { &Provider_hwnd_child, PROV_GET_PROPERTY_VALUE, METHOD_TODO }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_hwnd_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    NODE_CREATE_SEQ2(&Provider),
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Only done on Win10v1809+ */
    { &Provider_hwnd_child, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Windows 10+ calls these. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd_child, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_hwnd_child2),
    { &Provider_hwnd_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq12[] = {
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_LastChild */
    { &Provider_child2, PROV_GET_PROVIDER_OPTIONS },
    { &Provider_child2, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_child2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER },
    NODE_CREATE_SEQ2(&Provider),
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Only done on Win10v1809+ */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Windows 10+ calls these. */
    { &Provider_child2, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_PreviousSibling */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq13[] = {
    NODE_CREATE_SEQ2(&Provider),
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* Only done on Win10v1809+ */
    { &Provider_hwnd, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Windows 10+ calls these. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider_hwnd, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_hwnd, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq14[] = {
    { &Provider_nc, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider2),
    { &Provider2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static const struct prov_method_sequence nav_seq15[] = {
    NODE_CREATE_SEQ(&Provider_child2_child_child),
    { &Provider_child2_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
};

static const struct prov_method_sequence nav_seq16[] = {
    { &Provider_child2_child_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child2_child),
    { &Provider_child2_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child2_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    /* Only done on Win10v1507 and below. */
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { 0 }
};

static const struct prov_method_sequence nav_seq17[] = {
    { &Provider_child2_child_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child2_child),
    { &Provider_child2_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child2_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

static void set_provider_nav_ifaces(struct Provider *prov, struct Provider *parent, struct Provider *frag_root,
    struct Provider *prev_sibling, struct Provider *next_sibling, struct Provider *first_child,
    struct Provider *last_child)
{
    prov->parent = NULL;
    prov->frag_root = NULL;
    prov->prev_sibling = NULL;
    prov->next_sibling = NULL;
    prov->first_child = NULL;
    prov->last_child = NULL;

    if (parent)
        prov->parent = &parent->IRawElementProviderFragment_iface;
    if (frag_root)
        prov->frag_root = &frag_root->IRawElementProviderFragmentRoot_iface;
    if (prev_sibling)
        prov->prev_sibling = &prev_sibling->IRawElementProviderFragment_iface;
    if (next_sibling)
        prov->next_sibling = &next_sibling->IRawElementProviderFragment_iface;
    if (first_child)
        prov->first_child = &first_child->IRawElementProviderFragment_iface;
    if (last_child)
        prov->last_child = &last_child->IRawElementProviderFragment_iface;
}

static void test_UiaNavigate(void)
{
    struct Provider_prop_override prop_override;
    LONG exp_lbound[2], exp_elems[2], idx[2], i;
    struct node_provider_desc exp_node_desc[4];
    struct UiaPropertyCondition prop_cond;
    struct UiaCacheRequest cache_req;
    HUIANODE node, node2, node3;
    SAFEARRAY *out_req;
    BSTR tree_struct;
    WNDCLASSA cls;
    HRESULT hr;
    VARIANT v;
    HWND hwnd;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "UiaNavigate class";

    RegisterClassA(&cls);

    hwnd = CreateWindowA("UiaNavigate class", "Test window", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);

    UiaRegisterProviderCallback(test_uia_provider_callback);
    base_hwnd_prov = &Provider_hwnd.IRawElementProviderSimple_iface;
    nc_prov = &Provider_nc.IRawElementProviderSimple_iface;

    set_provider_nav_ifaces(&Provider_nc, NULL, NULL, NULL, NULL, &Provider_nc_child, &Provider_nc_child2);
    set_provider_nav_ifaces(&Provider_nc_child, &Provider_nc, &Provider_nc, NULL,
            &Provider_nc_child2, NULL, NULL);
    set_provider_nav_ifaces(&Provider_nc_child2, &Provider_nc, &Provider_nc, &Provider_nc_child,
            NULL, NULL, NULL);

    set_provider_nav_ifaces(&Provider_hwnd, NULL, NULL, NULL, NULL, &Provider_hwnd_child, &Provider_hwnd_child2);
    set_provider_nav_ifaces(&Provider_hwnd_child, &Provider_hwnd, &Provider_hwnd, NULL,
            &Provider_hwnd_child2, NULL, NULL);
    set_provider_nav_ifaces(&Provider_hwnd_child2, &Provider_hwnd, &Provider_hwnd, &Provider_hwnd_child,
            NULL, NULL, NULL);

    /*
     * Show navigation behavior for multi-provider nodes. Navigation order is:
     * HWND provider children.
     * Non-Client provider children.
     * Main provider children.
     * Override provider children.
     */
    Provider.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.hwnd = Provider_hwnd.hwnd = Provider_nc.hwnd = hwnd;
    Provider.ignore_hwnd_prop = Provider_nc.ignore_hwnd_prop = TRUE;
    prov_root = &Provider.IRawElementProviderSimple_iface;
    node = (void *)0xdeadbeef;
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider_hwnd.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    ok(Provider_nc.ref == 2, "Unexpected refcnt %ld\n", Provider_nc.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", TRUE);
    check_node_provider_desc(V_BSTR(&v), L"Nonclient", L"Provider_nc", FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider_hwnd", FALSE);
    VariantClear(&v);

    ok_method_sequence(nav_seq1, "nav_seq1");

    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);
    idx[0] = idx[1] = 0;

    /* Navigate to Provider_hwnd_child. */
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_hwnd_child", TRUE);
    set_cache_request(&cache_req, NULL, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    tree_struct = NULL; out_req = NULL;
    hr = UiaNavigate(node, NavigateDirection_FirstChild, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider_hwnd_child.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd_child.ref);

    node2 = node3 = NULL;
    hr = SafeArrayGetElement(out_req, idx, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    IUnknown_AddRef((IUnknown *)node2);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(nav_seq2, "nav_seq2");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    /* Navigate to Provider_hwnd_child2. */
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_hwnd_child2", TRUE);
    hr = UiaNavigate(node2, NavigateDirection_NextSibling, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider_hwnd_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd_child2.ref);

    hr = SafeArrayGetElement(out_req, idx, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, &node3);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    IUnknown_AddRef((IUnknown *)node3);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(nav_seq3, "nav_seq3");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");
    ok(Provider_hwnd_child.ref == 1, "Unexpected refcnt %ld\n", Provider_hwnd_child.ref);
    node2 = node3;

    /* Navigate to Provider_nc_child. */
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_nc_child", TRUE);
    hr = UiaNavigate(node2, NavigateDirection_NextSibling, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider_nc_child.ref == 2, "Unexpected refcnt %ld\n", Provider_nc_child.ref);
    CHECK_CALLED(prov_callback_nonclient);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);

    hr = SafeArrayGetElement(out_req, idx, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, &node3);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    IUnknown_AddRef((IUnknown *)node3);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(nav_seq4, "nav_seq4");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");
    ok(Provider_hwnd_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_hwnd_child2.ref);
    node2 = node3;

    /* Navigate back to Provider_hwnd_child2. */
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_hwnd_child2", TRUE);
    hr = UiaNavigate(node2, NavigateDirection_PreviousSibling, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider_hwnd_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd_child2.ref);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);

    hr = SafeArrayGetElement(out_req, idx, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, &node3);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    IUnknown_AddRef((IUnknown *)node3);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(nav_seq5, "nav_seq5");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    ok(UiaNodeRelease(node3), "UiaNodeRelease returned FALSE\n");
    ok(Provider_hwnd_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_hwnd_child2.ref);

    /* Navigate to Provider_nc_child2. */
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_nc_child2", TRUE);
    hr = UiaNavigate(node2, NavigateDirection_NextSibling, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider_nc_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_nc_child2.ref);

    hr = SafeArrayGetElement(out_req, idx, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, &node3);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    IUnknown_AddRef((IUnknown *)node3);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(nav_seq6, "nav_seq6");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");
    ok(Provider_nc_child.ref == 1, "Unexpected refcnt %ld\n", Provider_nc_child.ref);
    node2 = node3;

    /* Navigate to Provider_child. */
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_child", TRUE);
    hr = UiaNavigate(node2, NavigateDirection_NextSibling, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);

    hr = SafeArrayGetElement(out_req, idx, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, &node3);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    IUnknown_AddRef((IUnknown *)node3);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(nav_seq7, "nav_seq7");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");
    ok(Provider_nc_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_nc_child2.ref);
    node2 = node3;

    /* Navigate to Provider_child2. */
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_child2", TRUE);
    hr = UiaNavigate(node2, NavigateDirection_NextSibling, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);

    hr = SafeArrayGetElement(out_req, idx, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, &node3);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    IUnknown_AddRef((IUnknown *)node3);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(nav_seq8, "nav_seq8");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);
    node2 = node3;

    /* Try navigating to next sibling on the final child of the node. */
    SET_EXPECT_MULTI(prov_callback_nonclient, 2);
    SET_EXPECT_MULTI(prov_callback_base_hwnd, 2);
    SET_EXPECT_MULTI(winproc_GETOBJECT_UiaRoot, 2);
    hr = UiaNavigate(node2, NavigateDirection_NextSibling, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok_method_sequence(nav_seq9, "nav_seq9");
    ok(!out_req, "out_req != NULL\n");
    ok(!tree_struct, "tree_struct != NULL\n");
    todo_wine CHECK_CALLED_MULTI(prov_callback_nonclient, 2);
    todo_wine CHECK_CALLED_MULTI(prov_callback_base_hwnd, 2);
    todo_wine CHECK_CALLED_MULTI(winproc_GETOBJECT_UiaRoot, 2);

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);
    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");
    ok(Provider_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_child2.ref);

    /* Navigate to Provider_child2, last child. */
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_child2", TRUE);
    set_cache_request(&cache_req, NULL, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    tree_struct = NULL;
    out_req = NULL;
    hr = UiaNavigate(node, NavigateDirection_LastChild, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd_child.ref);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(nav_seq10, "nav_seq10");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    /*
     * If the child we navigate to from a parent isn't considered the "parent
     * link" of it's HUIANODE, it is skipped. Here, we set Provider_hwnd_child
     * to an HWND provider, and set its main provider as Provider, which is
     * the parent link of the node.
     */
    Provider_hwnd_child.hwnd = hwnd;
    Provider_hwnd_child.prov_opts = ProviderOptions_ClientSideProvider;
    Provider.parent = &Provider2.IRawElementProviderFragment_iface;
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_hwnd_child2", TRUE);
    tree_struct = NULL;
    out_req = NULL;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_nonclient);
    hr = UiaNavigate(node, NavigateDirection_FirstChild, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider_hwnd_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd_child2.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_nonclient);

    hr = SafeArrayGetElement(out_req, idx, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    IUnknown_AddRef((IUnknown *)node2);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(nav_seq11, "nav_seq11");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");
    ok(Provider_hwnd_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_hwnd_child2.ref);

    /*
     * Same test as before, except with NavigateDirection_LastChild. This will
     * end up with Provider_nc as the parent link for the node instead of
     * Provider_child2.
     */
    Provider_child2.hwnd = hwnd;
    Provider_child2.ignore_hwnd_prop = TRUE;
    Provider_child2.parent = NULL;
    Provider_nc.parent = &Provider2.IRawElementProviderFragment_iface;
    Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_child", TRUE);
    tree_struct = NULL;
    out_req = NULL;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_base_hwnd);
    hr = UiaNavigate(node, NavigateDirection_LastChild, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd_child2.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_nonclient);
    CHECK_CALLED(prov_callback_base_hwnd);

    hr = SafeArrayGetElement(out_req, idx, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    IUnknown_AddRef((IUnknown *)node2);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(nav_seq12, "nav_seq12");

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    Provider_child2.hwnd = NULL;
    Provider_child2.ignore_hwnd_prop = FALSE;
    Provider_child2.parent = &Provider.IRawElementProviderFragment_iface;

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider_hwnd.ref == 1, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    ok(Provider_nc.ref == 1, "Unexpected refcnt %ld\n", Provider_nc.ref);

    /* Create a node with Provider_nc as the parent link. */
    Provider.parent = Provider_hwnd.parent = NULL;
    Provider_nc.parent = &Provider2.IRawElementProviderFragment_iface;
    Provider2.prov_opts = ProviderOptions_ServerSideProvider;
    Provider2.hwnd = NULL;

    node = (void *)0xdeadbeef;
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    hr = UiaNodeFromHandle(hwnd, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider_hwnd.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    ok(Provider_nc.ref == 2, "Unexpected refcnt %ld\n", Provider_nc.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Nonclient", L"Provider_nc", TRUE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider_hwnd", FALSE);
    VariantClear(&v);

    ok_method_sequence(nav_seq13, "nav_seq13");

    /* Navigate to Provider2, parent of Provider_nc. */
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider2", TRUE);
    set_cache_request(&cache_req, NULL, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    tree_struct = NULL;
    out_req = NULL;
    hr = UiaNavigate(node, NavigateDirection_Parent, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider2.ref == 2, "Unexpected refcnt %ld\n", Provider2.ref);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);
    ok(Provider2.ref == 1, "Unexpected refcnt %ld\n", Provider2.ref);
    ok_method_sequence(nav_seq14, "nav_seq14");

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider_hwnd.ref == 1, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    ok(Provider_nc.ref == 1, "Unexpected refcnt %ld\n", Provider_nc.ref);

    Provider_hwnd_child.hwnd = NULL;
    Provider_hwnd_child.prov_opts = ProviderOptions_ServerSideProvider;
    Provider.ignore_hwnd_prop = Provider_nc.ignore_hwnd_prop = FALSE;
    base_hwnd_prov = nc_prov = NULL;
    prov_root = NULL;
    UiaRegisterProviderCallback(NULL);

    /*
     * Conditional navigation for conditions other than ConditionType_True.
     */
    initialize_provider_tree(TRUE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    provider_add_child(&Provider, &Provider_child2);
    provider_add_child(&Provider_child2, &Provider_child2_child);
    provider_add_child(&Provider_child2_child, &Provider_child2_child_child);

    hr = UiaNodeFromProvider(&Provider_child2_child_child.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child2_child_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child2_child_child.ref);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child2_child_child", TRUE);
    VariantClear(&v);

    ok_method_sequence(nav_seq15, "nav_seq15");

    /*
     * Navigate from Provider_child2_child_child to a parent that has
     * UIA_IsControlElementPropertyId set to FALSE.
     */
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    set_property_condition(&prop_cond, UIA_IsControlElementPropertyId, &v, PropertyConditionFlags_None);

    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider", TRUE);
    set_cache_request(&cache_req, NULL, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    tree_struct = NULL;
    out_req = NULL;
    hr = UiaNavigate(node, NavigateDirection_Parent, (struct UiaCondition *)&prop_cond, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!out_req, "out_req != NULL\n");
    ok(!tree_struct, "tree_struct != NULL\n");
    ok_method_sequence(nav_seq16, "nav_seq16");

    /* Provider will now return FALSE for UIA_IsControlElementPropertyId. */
    set_property_override(&prop_override, UIA_IsControlElementPropertyId, &v);
    set_provider_prop_override(&Provider, &prop_override, 1);

    set_property_condition(&prop_cond, UIA_IsControlElementPropertyId, &v, PropertyConditionFlags_None);
    init_node_provider_desc(&exp_node_desc[0], GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider", TRUE);
    set_cache_request(&cache_req, NULL, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full);
    tree_struct = NULL;
    out_req = NULL;
    hr = UiaNavigate(node, NavigateDirection_Parent, (struct UiaCondition *)&prop_cond, &cache_req, &out_req, &tree_struct);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!out_req, "out_req == NULL\n");
    ok(!!tree_struct, "tree_struct == NULL\n");
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    ok_method_sequence(nav_seq17, "nav_seq17");
    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider_child2_child_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child2_child_child.ref);

    CoUninitialize();
    DestroyWindow(hwnd);
    UnregisterClassA("UiaNavigate class", NULL);
}

static void set_find_params(struct UiaFindParams *params, int max_depth, BOOL find_first, BOOL exclude_root,
        struct UiaCondition *find_cond)
{
    params->MaxDepth = max_depth;
    params->FindFirst = find_first;
    params->ExcludeRoot = exclude_root;
    params->pFindCondition = find_cond;
}

static void set_provider_prop_override(struct Provider *prov, struct Provider_prop_override *override, int count)
{
    prov->prop_override = override;
    prov->prop_override_count = count;
}

static void set_property_override(struct Provider_prop_override *override, int prop_id, VARIANT *val)
{
    override->prop_id = prop_id;
    override->val = *val;
}

static void set_provider_win_event_handler_respond_prov(struct Provider *prov, IRawElementProviderSimple *responder_prov,
        int responder_event)
{
    struct Provider_win_event_handler_data *data = &prov->win_event_handler_data;

    data->responder_prov = responder_prov;
    data->responder_event = responder_event;
}

static void set_provider_win_event_handler_win_event_expects(struct Provider *prov, DWORD exp_win_event_id,
        HWND exp_win_event_hwnd, LONG exp_win_event_obj_id, LONG exp_win_event_child_id)
{
    struct Provider_win_event_handler_data *data = &prov->win_event_handler_data;

    data->exp_win_event_id = exp_win_event_id;
    data->exp_win_event_hwnd = exp_win_event_hwnd;
    data->exp_win_event_obj_id = exp_win_event_obj_id;
    data->exp_win_event_child_id = exp_win_event_child_id;
}

static void set_provider_runtime_id(struct Provider *prov, int val, int val2)
{
    prov->runtime_id[0] = val;
    prov->runtime_id[1] = val2;
}

static void set_provider_method_event_data(struct Provider *prov, HANDLE event_handle, int method_id)
{
    prov->method_call_event_handle = event_handle;
    prov->method_call_event_method_id = method_id;
}

static void initialize_provider_advise_events_ids(struct Provider *prov)
{
    prov->advise_events_added_event_id = prov->advise_events_removed_event_id = 0;
}

static void initialize_provider(struct Provider *prov, int prov_opts, HWND hwnd, BOOL initialize_nav_links)
{
    prov->prov_opts = prov_opts;
    prov->hwnd = hwnd;
    prov->ret_invalid_prop_type = FALSE;
    prov->expected_tid = 0;
    prov->runtime_id[0] = prov->runtime_id[1] = 0;
    prov->last_call_tid = 0;
    prov->ignore_hwnd_prop = FALSE;
    prov->override_hwnd = NULL;
    prov->prop_override = NULL;
    prov->prop_override_count = 0;
    memset(&prov->bounds_rect, 0, sizeof(prov->bounds_rect));
    memset(&prov->value_pattern_data, 0, sizeof(prov->value_pattern_data));
    memset(&prov->legacy_acc_pattern_data, 0, sizeof(prov->legacy_acc_pattern_data));
    prov->focus_prov = NULL;
    prov->embedded_frag_roots = NULL;
    prov->embedded_frag_roots_count = 0;
    prov->advise_events_added_event_id = prov->advise_events_removed_event_id = 0;
    memset(&prov->win_event_handler_data, 0, sizeof(prov->win_event_handler_data));
    prov->method_call_event_handle = NULL;
    prov->method_call_event_method_id = -1;
    if (initialize_nav_links)
    {
        prov->frag_root = NULL;
        prov->parent = prov->prev_sibling = prov->next_sibling = prov->first_child = prov->last_child = NULL;
    }
}

static void initialize_provider_tree(BOOL initialize_nav_links)
{
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, initialize_nav_links);
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, initialize_nav_links);
    initialize_provider(&Provider_child_child, ProviderOptions_ServerSideProvider, NULL, initialize_nav_links);
    initialize_provider(&Provider_child_child2, ProviderOptions_ServerSideProvider, NULL, initialize_nav_links);
    initialize_provider(&Provider_child2, ProviderOptions_ServerSideProvider, NULL, initialize_nav_links);
    initialize_provider(&Provider_child2_child, ProviderOptions_ServerSideProvider, NULL, initialize_nav_links);
    initialize_provider(&Provider_child2_child_child, ProviderOptions_ServerSideProvider, NULL, initialize_nav_links);
}

static void provider_add_child(struct Provider *prov, struct Provider *child)
{
    if (!prov->first_child)
    {
        prov->first_child = prov->last_child = &child->IRawElementProviderFragment_iface;
        child->next_sibling = child->prev_sibling = NULL;
    }
    else
    {
        struct Provider *tmp = impl_from_ProviderFragment(prov->last_child);

        tmp->next_sibling = &child->IRawElementProviderFragment_iface;
        child->prev_sibling = prov->last_child;
        prov->last_child = &child->IRawElementProviderFragment_iface;
    }

    child->parent = &prov->IRawElementProviderFragment_iface;
    child->frag_root = prov->frag_root;
}

#define test_find_sa_results( tree_structs, offsets, exp_elems, exp_tree_struct, exp_offsets ) \
        test_find_sa_results_( (tree_structs), (offsets), (exp_elems), (exp_tree_struct), (exp_offsets), __FILE__, __LINE__)
static void test_find_sa_results_(SAFEARRAY *tree_structs, SAFEARRAY *offsets, LONG exp_elems,
        const WCHAR **exp_tree_struct, int *exp_offset, const char *file, int line)
{
    LONG lbound, ubound, elems;
    HRESULT hr;
    VARTYPE vt;
    UINT dims;
    LONG i;

    /* Tree structures SA. */
    hr = SafeArrayGetVartype(tree_structs, &vt);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(file, line)(vt == VT_BSTR, "Unexpected tree structures sa vt %d\n", vt);
    dims = SafeArrayGetDim(tree_structs);
    ok_(file, line)(dims == 1, "Unexpected tree structures sa dims %d\n", dims);

    lbound = ubound = elems = 0;
    hr = SafeArrayGetLBound(tree_structs, 1, &lbound);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx for SafeArrayGetLBound\n", hr);
    ok_(file, line)(!lbound, "Unexpected lbound %ld\n", lbound);

    hr = SafeArrayGetUBound(tree_structs, 1, &ubound);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx for SafeArrayGetUBound\n", hr);

    elems = (ubound - lbound) + 1;
    ok_(file, line)(exp_elems == elems, "Unexpected elems %ld\n", elems);

    /* Offsets SA. */
    hr = SafeArrayGetVartype(offsets, &vt);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(file, line)(vt == VT_I4, "Unexpected offsets sa vt %d\n", vt);
    dims = SafeArrayGetDim(offsets);
    ok_(file, line)(dims == 1, "Unexpected offsets sa dims %d\n", dims);

    lbound = ubound = elems = 0;
    hr = SafeArrayGetLBound(offsets, 1, &lbound);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx for SafeArrayGetLBound\n", hr);
    ok_(file, line)(!lbound, "Unexpected lbound %ld\n", lbound);

    hr = SafeArrayGetUBound(offsets, 1, &ubound);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx for SafeArrayGetUBound\n", hr);

    elems = (ubound - lbound) + 1;
    ok_(file, line)(exp_elems == elems, "Unexpected elems %ld\n", elems);

    for (i = 0; i < exp_elems; i++)
    {
        BSTR tree_struct;
        int offset;

        hr = SafeArrayGetElement(tree_structs, &i, &tree_struct);
        ok_(file, line)(hr == S_OK, "Unexpected hr %#lx\n", hr);
        ok_(file, line)(!wcscmp(tree_struct, exp_tree_struct[i]), "Unexpected tree structure %s\n", debugstr_w(tree_struct));
        SysFreeString(tree_struct);

        hr = SafeArrayGetElement(offsets, &i, &offset);
        ok_(file, line)(hr == S_OK, "Unexpected hr %#lx\n", hr);
        ok_(file, line)(exp_offset[i] == offset, "Unexpected offset %d\n", offset);
    }
}

static const struct prov_method_sequence find_seq1[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child_child),
    { &Provider_child_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    { &Provider_child_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child_child2),
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child2_child),
    { &Provider_child2_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child2_child_child),
    { &Provider_child2_child_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    { &Provider_child2_child_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2_child_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child2_child),
    { &Provider_child2_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only done on Win10v1507 and below. */
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child2_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child2_child_child, FRAG_GET_RUNTIME_ID },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child2_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child2_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

static const struct prov_method_sequence find_seq2[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only done on Win10v1507 and below. */
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child2, FRAG_GET_RUNTIME_ID },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

static const struct prov_method_sequence find_seq3[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only done on Win10v1507 and below. */
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

static const struct prov_method_sequence find_seq4[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

static const struct prov_method_sequence find_seq5[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

static const struct prov_method_sequence find_seq6[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

static const struct prov_method_sequence find_seq7[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child_child),
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child_child2),
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only done on Win10v1507 and below. */
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider_child_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child2, FRAG_GET_RUNTIME_ID },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

static const struct prov_method_sequence find_seq8[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child_child),
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child_child2),
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider_child_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child2, FRAG_GET_RUNTIME_ID },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

static const struct prov_method_sequence find_seq9[] = {
    { &Provider_child_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child2_child),
    { &Provider_child2_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child2_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child2_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only done on Win10v1507 and below. */
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider_child_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child2_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child2_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

static const struct prov_method_sequence find_seq10[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child_child),
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    { &Provider_child_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child_child2),
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child2_child),
    { &Provider_child2_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child2_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child2_child_child),
    { &Provider_child2_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child2_child_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    { &Provider_child2_child_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2_child_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child2_child),
    { &Provider_child2_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2_child, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only done on Win10v1507 and below. */
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

static const struct prov_method_sequence find_seq11[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child_child),
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    { &Provider_child_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child_child2),
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child_child2, FRAG_GET_RUNTIME_ID },
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

static void test_UiaFind(void)
{
    struct Provider_prop_override prop_override;
    LONG exp_lbound[2], exp_elems[2], idx[2], i;
    SAFEARRAY *out_req, *offsets, *tree_structs;
    struct node_provider_desc exp_node_desc[7];
    struct UiaPropertyCondition prop_cond[2];
    struct UiaCacheRequest cache_req;
    struct UiaFindParams find_params;
    const WCHAR *exp_tree_struct[7];
    int exp_offset[7], cache_prop;
    HUIANODE node, node2;
    HRESULT hr;
    VARIANT v;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    initialize_provider_tree(TRUE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    provider_add_child(&Provider, &Provider_child);
    provider_add_child(&Provider, &Provider_child2);
    provider_add_child(&Provider_child, &Provider_child_child);
    provider_add_child(&Provider_child, &Provider_child_child2);
    provider_add_child(&Provider_child2, &Provider_child2_child);
    provider_add_child(&Provider_child2_child, &Provider_child2_child_child);

    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);

    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", TRUE);
    VariantClear(&v);

    ok_method_sequence(node_from_prov2, NULL);

    /*
     * Maximum find depth of -1, find first is FALSE, exclude root is FALSE. A
     * maximum depth of -1 will search the entire tree.
     */
    out_req = offsets = tree_structs = NULL;
    cache_prop = UIA_RuntimeIdPropertyId;
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, &cache_prop, 1, NULL, 0,
            AutomationElementMode_Full);
    set_find_params(&find_params, -1, FALSE, FALSE, (struct UiaCondition *)&UiaTrueCondition);
    hr = UiaFind(node, &find_params, &cache_req, &out_req, &offsets, &tree_structs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(Provider_child_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child_child.ref);
    ok(Provider_child_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child_child2.ref);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);
    ok(Provider_child2_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child2_child.ref);
    ok(Provider_child2_child_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child2_child_child.ref);

    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider", TRUE);
    add_provider_desc(&exp_node_desc[1], L"Main", L"Provider_child", TRUE);
    add_provider_desc(&exp_node_desc[2], L"Main", L"Provider_child_child", TRUE);
    add_provider_desc(&exp_node_desc[3], L"Main", L"Provider_child_child2", TRUE);
    add_provider_desc(&exp_node_desc[4], L"Main", L"Provider_child2", TRUE);
    add_provider_desc(&exp_node_desc[5], L"Main", L"Provider_child2_child", TRUE);
    add_provider_desc(&exp_node_desc[6], L"Main", L"Provider_child2_child_child", TRUE);
    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 7;
    exp_elems[1] = 2;

    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    for (i = 0; i < exp_elems[0]; i++)
    {
        exp_offset[i] = i;
        exp_tree_struct[i] = L"P)";
    }
    test_find_sa_results(tree_structs, offsets, exp_elems[0], exp_tree_struct, exp_offset);
    ok_method_sequence(find_seq1, "find_seq1");

    SafeArrayDestroy(out_req);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(tree_structs);

    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);

    /*
     * Maximum find depth of 1, find first is FALSE, exclude root is FALSE.
     */
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, &cache_prop, 1, NULL, 0,
            AutomationElementMode_Full);
    set_find_params(&find_params, 1, FALSE, FALSE, (struct UiaCondition *)&UiaTrueCondition);
    hr = UiaFind(node, &find_params, &cache_req, &out_req, &offsets, &tree_structs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);

    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider", TRUE);
    add_provider_desc(&exp_node_desc[1], L"Main", L"Provider_child", TRUE);
    add_provider_desc(&exp_node_desc[2], L"Main", L"Provider_child2", TRUE);
    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 3;
    exp_elems[1] = 2;

    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    for (i = 0; i < exp_elems[0]; i++)
    {
        exp_offset[i] = i;
        exp_tree_struct[i] = L"P)";
    }

    test_find_sa_results(tree_structs, offsets, exp_elems[0], exp_tree_struct, exp_offset);
    ok_method_sequence(find_seq2, "find_seq2");

    SafeArrayDestroy(out_req);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(tree_structs);

    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);

    /*
     * Maximum find depth of 1, find first is FALSE, exclude root is TRUE.
     */
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, &cache_prop, 1, NULL, 0,
            AutomationElementMode_Full);
    set_find_params(&find_params, 1, FALSE, TRUE, (struct UiaCondition *)&UiaTrueCondition);
    hr = UiaFind(node, &find_params, &cache_req, &out_req, &offsets, &tree_structs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);

    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_child", TRUE);
    add_provider_desc(&exp_node_desc[1], L"Main", L"Provider_child2", TRUE);
    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 2;
    exp_elems[1] = 2;

    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    for (i = 0; i < exp_elems[0]; i++)
    {
        exp_offset[i] = i;
        exp_tree_struct[i] = L"P)";
    }
    test_find_sa_results(tree_structs, offsets, exp_elems[0], exp_tree_struct, exp_offset);
    ok_method_sequence(find_seq3, "find_seq3");

    SafeArrayDestroy(out_req);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(tree_structs);

    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);

    /*
     * Maximum find depth of 1, find first is TRUE, exclude root is TRUE. Will
     * retrieve only Provider_child.
     */
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, &cache_prop, 1, NULL, 0,
            AutomationElementMode_Full);
    set_find_params(&find_params, 1, TRUE, TRUE, (struct UiaCondition *)&UiaTrueCondition);
    hr = UiaFind(node, &find_params, &cache_req, &out_req, &offsets, &tree_structs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_child", TRUE);
    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 1;
    exp_elems[1] = 2;

    idx[0] = idx[1] = 0;
    hr = SafeArrayGetElement(out_req, idx, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    /* node2 is now set as Provider_child. */
    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    IUnknown_AddRef((IUnknown *)node2);

    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    for (i = 0; i < exp_elems[0]; i++)
    {
        exp_offset[i] = i;
        exp_tree_struct[i] = L"P)";
    }
    test_find_sa_results(tree_structs, offsets, exp_elems[0], exp_tree_struct, exp_offset);
    ok_method_sequence(find_seq4, "find_seq4");

    SafeArrayDestroy(out_req);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(tree_structs);

    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);

    /*
     * Maximum find depth of 0, find first is FALSE, exclude root is FALSE.
     * Provider_child doesn't have a runtime id for UI Automation to use as a
     * way to check if it has navigated back to the node that began the
     * search, so it will get siblings.
     */
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, &cache_prop, 1, NULL, 0,
            AutomationElementMode_Full);
    set_find_params(&find_params, 0, FALSE, FALSE, (struct UiaCondition *)&UiaTrueCondition);
    hr = UiaFind(node2, &find_params, &cache_req, &out_req, &offsets, &tree_structs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);

    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_child", TRUE);
    add_provider_desc(&exp_node_desc[1], L"Main", L"Provider_child2", TRUE);
    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 2;
    exp_elems[1] = 2;

    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    for (i = 0; i < exp_elems[0]; i++)
    {
        exp_offset[i] = i;
        exp_tree_struct[i] = L"P)";
    }
    test_find_sa_results(tree_structs, offsets, exp_elems[0], exp_tree_struct, exp_offset);
    ok_method_sequence(find_seq5, "find_seq5");

    SafeArrayDestroy(out_req);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(tree_structs);

    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);

    /*
     * Maximum find depth of 0, find first is FALSE, exclude root is FALSE.
     * Provider_child has a runtime id for UI Automation to use as a
     * way to check if it has navigated back to the node that began the
     * search, so it will stop at Provider_child.
     */
    Provider_child.runtime_id[0] = Provider_child.runtime_id[1] = 0xdeadbeef;
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, &cache_prop, 1, NULL, 0,
            AutomationElementMode_Full);
    set_find_params(&find_params, 0, FALSE, FALSE, (struct UiaCondition *)&UiaTrueCondition);
    hr = UiaFind(node2, &find_params, &cache_req, &out_req, &offsets, &tree_structs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_child", TRUE);
    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 1;
    exp_elems[1] = 2;

    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    for (i = 0; i < exp_elems[0]; i++)
    {
        exp_offset[i] = i;
        exp_tree_struct[i] = L"P)";
    }
    test_find_sa_results(tree_structs, offsets, exp_elems[0], exp_tree_struct, exp_offset);
    ok_method_sequence(find_seq6, "find_seq6");

    SafeArrayDestroy(out_req);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(tree_structs);

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    initialize_provider_tree(FALSE);
    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);

    /*
     * Maximum find depth of 1, find first is FALSE, exclude root is FALSE.
     * The cache request view condition is used to determine tree depth, if an
     * element matches the cache request view condition, depth is incremented.
     * Since Provider_child does not, Provider_child_child, Provider_child_child2,
     * and Provider_child2 are all considered to be at depth 1.
     */
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    set_property_condition(&prop_cond[0], UIA_IsContentElementPropertyId, &v, 0);
    set_property_override(&prop_override, UIA_IsContentElementPropertyId, &v);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_TRUE;
    set_property_condition(&prop_cond[1], UIA_IsControlElementPropertyId, &v, 0);

    set_provider_prop_override(&Provider, &prop_override, 1);
    set_provider_prop_override(&Provider_child2, &prop_override, 1);
    set_provider_prop_override(&Provider_child_child, &prop_override, 1);
    set_provider_prop_override(&Provider_child_child2, &prop_override, 1);

    set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond[0], TreeScope_Element, &cache_prop, 1, NULL, 0,
            AutomationElementMode_Full);
    set_find_params(&find_params, 1, FALSE, FALSE, (struct UiaCondition *)&prop_cond[1]);
    hr = UiaFind(node, &find_params, &cache_req, &out_req, &offsets, &tree_structs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child_child.ref);
    ok(Provider_child_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child_child2.ref);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);

    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider", TRUE);
    add_provider_desc(&exp_node_desc[1], L"Main", L"Provider_child_child", TRUE);
    add_provider_desc(&exp_node_desc[2], L"Main", L"Provider_child_child2", TRUE);
    add_provider_desc(&exp_node_desc[3], L"Main", L"Provider_child2", TRUE);
    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 4;
    exp_elems[1] = 2;

    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    for (i = 0; i < exp_elems[0]; i++)
    {
        exp_offset[i] = i;
        exp_tree_struct[i] = L"P)";
    }
    test_find_sa_results(tree_structs, offsets, exp_elems[0], exp_tree_struct, exp_offset);
    ok_method_sequence(find_seq7, "find_seq7");

    SafeArrayDestroy(out_req);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(tree_structs);

    initialize_provider_tree(FALSE);
    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);

    /*
     * Same test as before, except Provider has a runtime id.
     */
    Provider.runtime_id[0] = Provider.runtime_id[1] = 0xdeadbeef;
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    set_property_condition(&prop_cond[0], UIA_IsContentElementPropertyId, &v, 0);
    set_property_override(&prop_override, UIA_IsContentElementPropertyId, &v);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_TRUE;
    set_property_condition(&prop_cond[1], UIA_IsControlElementPropertyId, &v, 0);

    set_provider_prop_override(&Provider, &prop_override, 1);
    set_provider_prop_override(&Provider_child2, &prop_override, 1);
    set_provider_prop_override(&Provider_child_child, &prop_override, 1);
    set_provider_prop_override(&Provider_child_child2, &prop_override, 1);

    set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond[0], TreeScope_Element, &cache_prop, 1, NULL, 0,
            AutomationElementMode_Full);
    set_find_params(&find_params, 1, FALSE, FALSE, (struct UiaCondition *)&prop_cond[1]);
    hr = UiaFind(node, &find_params, &cache_req, &out_req, &offsets, &tree_structs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child_child.ref);
    ok(Provider_child_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child_child2.ref);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);

    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider", TRUE);
    add_provider_desc(&exp_node_desc[1], L"Main", L"Provider_child_child", TRUE);
    add_provider_desc(&exp_node_desc[2], L"Main", L"Provider_child_child2", TRUE);
    add_provider_desc(&exp_node_desc[3], L"Main", L"Provider_child2", TRUE);
    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 4;
    exp_elems[1] = 2;

    idx[0] = 2;
    idx[1] = 0;
    hr = SafeArrayGetElement(out_req, idx, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    /* node2 is now set as Provider_child_child2. */
    hr = UiaHUiaNodeFromVariant(&v, &node2);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    IUnknown_AddRef((IUnknown *)node2);

    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    for (i = 0; i < exp_elems[0]; i++)
    {
        exp_offset[i] = i;
        exp_tree_struct[i] = L"P)";
    }
    test_find_sa_results(tree_structs, offsets, exp_elems[0], exp_tree_struct, exp_offset);
    ok_method_sequence(find_seq8, "find_seq8");

    SafeArrayDestroy(out_req);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(tree_structs);

    initialize_provider_tree(FALSE);
    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);

    /*
     * Maximum find depth of 1, find first is FALSE, exclude root is FALSE.
     * Starting at Provider_child_child2, find will be able to traverse the
     * tree in the same order as it would if we had started at the tree root
     * Provider, retrieving Provider_child2 as a sibling and
     * Provider_child2_child as a node at depth 1.
     */
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    set_property_condition(&prop_cond[0], UIA_IsContentElementPropertyId, &v, 0);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_TRUE;
    set_property_condition(&prop_cond[1], UIA_IsControlElementPropertyId, &v, 0);

    prop_override.prop_id = UIA_IsContentElementPropertyId;
    V_VT(&prop_override.val) = VT_BOOL;
    V_BOOL(&prop_override.val) = VARIANT_FALSE;
    set_provider_prop_override(&Provider_child_child2, &prop_override, 1);
    set_provider_prop_override(&Provider_child2, &prop_override, 1);
    set_provider_prop_override(&Provider_child2_child, &prop_override, 1);

    set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond[0], TreeScope_Element, &cache_prop, 1, NULL, 0,
            AutomationElementMode_Full);
    set_find_params(&find_params, 1, FALSE, FALSE, (struct UiaCondition *)&prop_cond[1]);
    hr = UiaFind(node2, &find_params, &cache_req, &out_req, &offsets, &tree_structs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);
    ok(Provider_child2_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child2_child.ref);

    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_child_child2", TRUE);
    add_provider_desc(&exp_node_desc[1], L"Main", L"Provider_child2", TRUE);
    add_provider_desc(&exp_node_desc[2], L"Main", L"Provider_child2_child", TRUE);
    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 3;
    exp_elems[1] = 2;

    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    for (i = 0; i < exp_elems[0]; i++)
    {
        exp_offset[i] = i;
        exp_tree_struct[i] = L"P)";
    }
    test_find_sa_results(tree_structs, offsets, exp_elems[0], exp_tree_struct, exp_offset);
    ok_method_sequence(find_seq9, "find_seq9");

    SafeArrayDestroy(out_req);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(tree_structs);

    initialize_provider_tree(FALSE);
    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);

    ok(UiaNodeRelease(node2), "UiaNodeRelease returned FALSE\n");
    ok(Provider_child_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_child_child2.ref);

    /*
     * Maximum find depth of 1, find first is FALSE, exclude root is TRUE.
     * Exclude root applies to the first node that matches the view
     * condition, and not the node that is passed into UiaFind(). Since
     * Provider doesn't match our view condition here, Provider_child will be
     * excluded.
     */
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    set_property_condition(&prop_cond[0], UIA_IsContentElementPropertyId, &v, 0);
    set_property_override(&prop_override, UIA_IsContentElementPropertyId, &v);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_TRUE;
    set_property_condition(&prop_cond[1], UIA_IsControlElementPropertyId, &v, 0);

    set_provider_prop_override(&Provider_child, &prop_override, 1);
    set_provider_prop_override(&Provider_child2, &prop_override, 1);

    set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond[0], TreeScope_Element, &cache_prop, 1, NULL, 0,
            AutomationElementMode_Full);
    set_find_params(&find_params, 1, FALSE, TRUE, (struct UiaCondition *)&prop_cond[1]);
    hr = UiaFind(node, &find_params, &cache_req, &out_req, &offsets, &tree_structs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);

    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_child2", TRUE);
    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 1;
    exp_elems[1] = 2;

    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    for (i = 0; i < exp_elems[0]; i++)
    {
        exp_offset[i] = i;
        exp_tree_struct[i] = L"P)";
    }
    test_find_sa_results(tree_structs, offsets, exp_elems[0], exp_tree_struct, exp_offset);

    ok_method_sequence(find_seq10, "find_seq10");

    SafeArrayDestroy(out_req);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(tree_structs);

    initialize_provider_tree(FALSE);
    for (i = 0; i < ARRAY_SIZE(exp_node_desc); i++)
        init_node_provider_desc(&exp_node_desc[i], GetCurrentProcessId(), NULL);

    /*
     * Maximum find depth of -1, find first is TRUE, exclude root is FALSE.
     * Provider_child_child2 is the only element in the tree to match our
     * condition.
     */
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    set_property_condition(&prop_cond[0], UIA_IsContentElementPropertyId, &v, 0);
    set_property_override(&prop_override, UIA_IsContentElementPropertyId, &v);

    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_TRUE;
    set_property_condition(&prop_cond[1], UIA_IsControlElementPropertyId, &v, 0);

    set_provider_prop_override(&Provider_child_child2, &prop_override, 1);

    set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond[0], TreeScope_Element, &cache_prop, 1, NULL, 0,
            AutomationElementMode_Full);
    set_find_params(&find_params, -1, TRUE, FALSE, (struct UiaCondition *)&prop_cond[1]);
    hr = UiaFind(node, &find_params, &cache_req, &out_req, &offsets, &tree_structs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child_child2.ref);

    add_provider_desc(&exp_node_desc[0], L"Main", L"Provider_child_child2", TRUE);
    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = 1;
    exp_elems[1] = 2;

    test_cache_req_sa(out_req, exp_lbound, exp_elems, exp_node_desc);

    for (i = 0; i < exp_elems[0]; i++)
    {
        exp_offset[i] = i;
        exp_tree_struct[i] = L"P)";
    }
    test_find_sa_results(tree_structs, offsets, exp_elems[0], exp_tree_struct, exp_offset);
    ok_method_sequence(find_seq11, "find_seq11");

    SafeArrayDestroy(out_req);
    SafeArrayDestroy(offsets);
    SafeArrayDestroy(tree_structs);

    initialize_provider_tree(TRUE);

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    CoUninitialize();
}

struct marshal_thread_data {
    IUnknown *iface;
    const GUID *iface_iid;
    BOOL expect_proxy;
    const char *file;
    int line;

    IStream *marshal_stream;
};

static DWORD WINAPI interface_marshal_proxy_thread(LPVOID param)
{
    struct marshal_thread_data *data = (struct marshal_thread_data *)param;
    IUnknown *proxy_iface, *unk, *unk2;
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    proxy_iface = unk = unk2 = NULL;
    hr = CoGetInterfaceAndReleaseStream(data->marshal_stream, data->iface_iid, (void **)&proxy_iface);
    ok_(data->file, data->line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IUnknown_QueryInterface(data->iface, &IID_IUnknown, (void **)&unk);
    ok_(data->file, data->line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(data->file, data->line)(!!unk, "unk == NULL\n");

    hr = IUnknown_QueryInterface(proxy_iface, &IID_IUnknown, (void **)&unk2);
    ok_(data->file, data->line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(data->file, data->line)(!!unk2, "unk2 == NULL\n");

    if (data->expect_proxy)
        ok_(data->file, data->line)(unk != unk2, "unk == unk2\n");
    else
        ok_(data->file, data->line)(unk == unk2, "unk != unk2\n");

    IUnknown_Release(proxy_iface);
    IUnknown_Release(unk);
    IUnknown_Release(unk2);

    CoUninitialize();
    return 0;
}

static void check_interface_marshal_proxy_creation_(IUnknown *iface, REFIID iid, BOOL expect_proxy, const char *file,
        int line)
{
    struct marshal_thread_data data = { NULL, iid, expect_proxy, file, line };
    HANDLE thread;
    HRESULT hr;

    hr = IUnknown_QueryInterface(iface, data.iface_iid, (void **)&data.iface);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoMarshalInterThreadInterfaceInStream(data.iface_iid, data.iface, &data.marshal_stream);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    thread = CreateThread(NULL, 0, interface_marshal_proxy_thread, (void *)&data, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;
        while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);

    IUnknown_Release(data.iface);
}

static HWND create_test_hwnd(const char *class_name)
{
    WNDCLASSA cls = { 0 };

    cls.lpfnWndProc = test_wnd_proc;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpszClassName = class_name;
    RegisterClassA(&cls);

    return CreateWindowA(class_name, "Test window", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);
}

static HWND create_child_test_hwnd(const char *class_name, HWND parent)
{
    WNDCLASSA cls = { 0 };

    cls.lpfnWndProc = child_test_wnd_proc;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpszClassName = class_name;
    RegisterClassA(&cls);

    return CreateWindowA(class_name, "Test child window", WS_CHILD,
            0, 0, 50, 50, parent, NULL, NULL, NULL);
}

static void destroy_test_hwnd(HWND hwnd, const char *class_name, const char *child_class_name)
{
    DestroyWindow(hwnd);
    UnregisterClassA(class_name, NULL);
    if (child_class_name)
        UnregisterClassA(child_class_name, NULL);
}

static IUIAutomationElement *create_test_element_from_hwnd(IUIAutomation *uia_iface, HWND hwnd, BOOL block_hwnd_provs)
{
    IUIAutomationElement *element;
    HRESULT hr;
    VARIANT v;

    if (block_hwnd_provs)
    {
        SET_EXPECT(prov_callback_base_hwnd);
        SET_EXPECT(prov_callback_nonclient);
        base_hwnd_prov = proxy_prov = parent_proxy_prov = nc_prov = NULL;
        UiaRegisterProviderCallback(test_uia_provider_callback);
    }
    else
        UiaRegisterProviderCallback(NULL);

    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, hwnd, TRUE);
    prov_root = &Provider.IRawElementProviderSimple_iface;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    hr = IUIAutomation_ElementFromHandle(uia_iface, hwnd, &element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!element, "element == NULL\n");
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;
    if (block_hwnd_provs)
    {
        CHECK_CALLED(prov_callback_base_hwnd);
        CHECK_CALLED(prov_callback_nonclient);
    }

    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element, UIA_ProviderDescriptionPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);

    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    if (!block_hwnd_provs)
    {
        check_node_provider_desc_todo(V_BSTR(&v), L"Main", L"Provider", FALSE);
        check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
        check_node_provider_desc_todo(V_BSTR(&v), L"Hwnd", NULL, TRUE);
    }
    else
        check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", TRUE);

    VariantClear(&v);
    ok_method_sequence(node_from_hwnd2, "create_test_element");
    UiaRegisterProviderCallback(NULL);

    return element;
}

static void test_ElementFromHandle(IUIAutomation *uia_iface, BOOL is_cui8)
{
    HWND hwnd = create_test_hwnd("test_ElementFromHandle class");
    IUIAutomationElement2 *element_2;
    IUIAutomationElement *element;
    HRESULT hr;

    element = create_test_element_from_hwnd(uia_iface, hwnd, FALSE);
    hr = IUIAutomationElement_QueryInterface(element, &IID_IUIAutomationElement2, (void **)&element_2);
    if (is_cui8)
    {
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!!element_2, "element_2 == NULL\n");
        IUIAutomationElement2_Release(element_2);
    }
    else
        ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

    /*
     * The IUIAutomationElement interface uses the free threaded marshaler, so
     * no actual proxy interface will be created.
     */
    check_interface_marshal_proxy_creation((IUnknown *)element, &IID_IUIAutomationElement, FALSE);

    IUIAutomationElement_Release(element);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    DestroyWindow(hwnd);
    UnregisterClassA("test_ElementFromHandle class", NULL);
    prov_root = NULL;
}

static void test_Element_GetPropertyValue(IUIAutomation *uia_iface)
{
    HWND hwnd = create_test_hwnd("test_Element_GetPropertyValue class");
    const struct uia_element_property *elem_prop;
    struct Provider_prop_override prop_override;
    IUIAutomationElement *element, *element2;
    IUIAutomationElementArray *element_arr;
    int i, len, prop_id, tmp_int;
    struct UiaRect uia_rect;
    IUnknown *unk_ns;
    BSTR tmp_bstr;
    HRESULT hr;
    RECT rect;
    VARIANT v;

    element = create_test_element_from_hwnd(uia_iface, hwnd, TRUE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    provider_add_child(&Provider, &Provider_child);

    hr = UiaGetReservedNotSupportedValue(&unk_ns);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element, UIA_ControlTypePropertyId, TRUE, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    VariantInit(&v);
    V_VT(&v) = VT_BOOL;
    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element, 1, TRUE, &v);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_EMPTY, "Unexpected vt %d\n", V_VT(&v));

    for (i = 0; i < ARRAY_SIZE(element_properties); i++)
    {
        elem_prop = &element_properties[i];

        Provider.ret_invalid_prop_type = FALSE;
        VariantClear(&v);
        if (!(prop_id = UiaLookupId(AutomationIdentifierType_Property, elem_prop->prop_guid)))
        {
            win_skip("No propertyId for GUID %s, skipping further tests.\n", debugstr_guid(elem_prop->prop_guid));
            break;
        }

        winetest_push_context("Element prop_id %d", prop_id);
        hr = IUIAutomationElement_GetCurrentPropertyValueEx(element, prop_id, TRUE, &v);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_uia_prop_val(prop_id, elem_prop->type, &v, TRUE);

        /*
         * Some properties have special behavior if an invalid value is
         * returned, skip them here.
         */
        if (!elem_prop->skip_invalid)
        {
            Provider.ret_invalid_prop_type = TRUE;
            hr = IUIAutomationElement_GetCurrentPropertyValueEx(element, prop_id, TRUE, &v);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            if (SUCCEEDED(hr))
            {
                ok_method_sequence(get_prop_invalid_type_seq, NULL);
                ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
                ok(V_UNKNOWN(&v) == unk_ns, "unexpected IUnknown %p\n", V_UNKNOWN(&v));
                VariantClear(&v);
            }
        }

        winetest_pop_context();
    }

    /* Test IUIAutomationElementArray interface behavior. */
    Provider.ret_invalid_prop_type = FALSE;
    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element, UIA_ControllerForPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);

    element_arr = NULL;
    hr = IUnknown_QueryInterface(V_UNKNOWN(&v), &IID_IUIAutomationElementArray, (void **)&element_arr);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!element_arr, "element_arr == NULL\n");
    VariantClear(&v);

    hr = IUIAutomationElementArray_get_Length(element_arr, &len);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(len == ARRAY_SIZE(uia_unk_arr_prop_val), "Unexpected length %d\n", len);

    /* Invalid argument tests. */
    hr = IUIAutomationElementArray_get_Length(element_arr, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx\n", hr);

    element2 = (void *)0xdeadbeef;
    hr = IUIAutomationElementArray_GetElement(element_arr, len, &element2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx\n", hr);
    /* Pointer isn't cleared. */
    ok(!!element2, "element2 == NULL\n");

    hr = IUIAutomationElementArray_GetElement(element_arr, -1, &element2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx\n", hr);
    /* Pointer isn't cleared. */
    ok(!!element2, "element2 == NULL\n");

    hr = IUIAutomationElementArray_GetElement(element_arr, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx\n", hr);

    for (i = 0; i < len; i++)
    {
        element2 = NULL;
        hr = IUIAutomationElementArray_GetElement(element_arr, i, &element2);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
        ok(!!element2, "element2 == NULL\n");

        hr = IUIAutomationElement_GetCurrentPropertyValueEx(element2, UIA_ControlTypePropertyId, TRUE, &v);
        ok(hr == S_OK, "element[%d] Unexpected hr %#lx\n", i, hr);
        ok(V_VT(&v) == VT_I4, "element[%d] Unexpected VT %d\n", i, V_VT(&v));
        ok(V_I4(&v) == uia_i4_prop_val, "element[%d] Unexpected I4 %#lx\n", i, V_I4(&v));

        IUIAutomationElement_Release(element2);
        VariantClear(&v);
    }

    IUIAutomationElementArray_Release(element_arr);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(Provider_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_child2.ref);
    ok_method_sequence(get_elem_arr_prop_seq, NULL);

    /*
     * IUIAutomationElement_get_CurrentControlType tests. If the value
     * returned for UIA_ControlTypePropertyId is not a registered control
     * type ID, we'll get back UIA_CustomControlTypeId.
     */
    tmp_int = 0xdeadb33f;
    hr = IUIAutomationElement_get_CurrentControlType(element, &tmp_int);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /*
     * Win10v1507 and below don't check whether or not the returned control
     * type ID is valid.
     */
    ok(tmp_int == UIA_CustomControlTypeId || broken(tmp_int == 0xdeadbeef), "Unexpected control type %#x\n", tmp_int);
    ok_method_sequence(get_prop_seq, NULL);

    Provider.ret_invalid_prop_type = TRUE;
    tmp_int = 0xdeadbeef;
    hr = IUIAutomationElement_get_CurrentControlType(element, &tmp_int);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(tmp_int == UIA_CustomControlTypeId, "Unexpected control type %#x\n", tmp_int);
    Provider.ret_invalid_prop_type = FALSE;
    ok_method_sequence(get_prop_invalid_type_seq, NULL);

    /* Finally, a valid control type. */
    V_VT(&v) = VT_I4;
    V_I4(&v) = UIA_HyperlinkControlTypeId;
    set_property_override(&prop_override, UIA_ControlTypePropertyId, &v);
    set_provider_prop_override(&Provider, &prop_override, 1);
    hr = IUIAutomationElement_get_CurrentControlType(element, &tmp_int);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(tmp_int == UIA_HyperlinkControlTypeId, "Unexpected control type %#x\n", tmp_int);
    set_provider_prop_override(&Provider, NULL, 0);
    ok_method_sequence(get_prop_seq, NULL);

    /*
     * IUIAutomationElement_get_CurrentName tests.
     */
    tmp_bstr = NULL;
    hr = IUIAutomationElement_get_CurrentName(element, &tmp_bstr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(tmp_bstr, uia_bstr_prop_str), "Unexpected BSTR %s\n", wine_dbgstr_w(tmp_bstr));
    SysFreeString(tmp_bstr);
    ok_method_sequence(get_prop_seq, NULL);

    tmp_bstr = NULL;
    Provider.ret_invalid_prop_type = TRUE;
    hr = IUIAutomationElement_get_CurrentName(element, &tmp_bstr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(tmp_bstr, L""), "Unexpected BSTR %s\n", wine_dbgstr_w(tmp_bstr));
    SysFreeString(tmp_bstr);
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, FALSE);
    ok_method_sequence(get_prop_invalid_type_seq, NULL);

    /*
     * Windows 7 will call get_FragmentRoot in an endless loop until the fragment root returns an HWND.
     * It's the only version with this behavior.
     */
    if (!UiaLookupId(AutomationIdentifierType_Property, &OptimizeForVisualContent_Property_GUID))
    {
        win_skip("Skipping UIA_BoundingRectanglePropertyId tests for Win7\n");
        goto exit;
    }

    /*
     * IUIAutomationElement_get_CurrentBoundingRectangle/UIA_BoundRectanglePropertyId tests.
     */
    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element, UIA_LabeledByPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    hr = IUnknown_QueryInterface(V_UNKNOWN(&v), &IID_IUIAutomationElement, (void **)&element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!element2, "element2 == NULL\n");
    VariantClear(&v);

    /* Non-empty bounding rectangle, will return a VT_R8 SAFEARRAY. */
    set_uia_rect(&uia_rect, 0, 0, 50, 50);
    Provider_child.bounds_rect = uia_rect;
    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element2, UIA_BoundingRectanglePropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_rect_val(&v, &uia_rect);
    VariantClear(&v);
    ok_method_sequence(get_bounding_rect_seq2, "get_bounding_rect_seq2");

    hr = IUIAutomationElement_get_CurrentBoundingRectangle(element2, &rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_rect_rect_val(&rect, &uia_rect);
    memset(&rect, 0, sizeof(rect));
    ok_method_sequence(get_bounding_rect_seq3, "get_bounding_rect_seq3");

    /* Empty bounding rectangle will return ReservedNotSupportedValue. */
    set_uia_rect(&uia_rect, 0, 0, 0, 0);
    Provider_child.bounds_rect = uia_rect;
    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element2, UIA_BoundingRectanglePropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(V_UNKNOWN(&v) == unk_ns, "unexpected IUnknown %p\n", V_UNKNOWN(&v));
    VariantClear(&v);
    ok_method_sequence(get_empty_bounding_rect_seq, "get_empty_bounding_rect_seq");

    /* Returns an all 0 rect. */
    hr = IUIAutomationElement_get_CurrentBoundingRectangle(element2, &rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_rect_rect_val(&rect, &uia_rect);
    ok_method_sequence(get_empty_bounding_rect_seq, "get_empty_bounding_rect_seq");

    IUIAutomationElement_Release(element2);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, FALSE);

exit:
    IUIAutomationElement_Release(element);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    DestroyWindow(hwnd);
    UnregisterClassA("test_Element_GetPropertyValue class", NULL);
}

static void test_CUIAutomation_value_conversion(IUIAutomation *uia_iface)
{
    static const VARTYPE invalid_int_vts[] = { VT_I8, VT_INT };
    int in_arr[3] = { 0xdeadbeef, 0xfeedbeef, 0x1337b33f };
    int *out_arr, out_arr_count, i;
    LONG lbound, ubound;
    SAFEARRAY *sa;
    VARTYPE vt;
    HRESULT hr;
    UINT dims;

    /*
     * Conversion from VT_I4 SAFEARRAY to native int array.
     */
    for (i = 0; i < ARRAY_SIZE(invalid_int_vts); i++)
    {
        vt = invalid_int_vts[i];
        sa = SafeArrayCreateVector(vt, 0, 2);
        ok(!!sa, "sa == NULL\n");

        out_arr_count = 0xdeadbeef;
        out_arr = (int *)0xdeadbeef;
        hr = IUIAutomation_IntSafeArrayToNativeArray(uia_iface, sa, &out_arr, &out_arr_count);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        ok(!out_arr, "out_arr != NULL\n");
        ok(out_arr_count == 0xdeadbeef, "Unexpected out_arr_count %#x\n", out_arr_count);
        SafeArrayDestroy(sa);
    }

    /* Only accepts VT_I4 as an input array type. */
    sa = create_i4_safearray();
    hr = IUIAutomation_IntSafeArrayToNativeArray(uia_iface, sa, &out_arr, &out_arr_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(out_arr_count == ARRAY_SIZE(uia_i4_arr_prop_val), "Unexpected out_arr_count %#x\n", out_arr_count);
    for (i = 0; i < ARRAY_SIZE(uia_i4_arr_prop_val); i++)
        ok(out_arr[i] == uia_i4_arr_prop_val[i], "out_arr[%d]: Expected %ld, got %d\n", i, uia_i4_arr_prop_val[i], out_arr[i]);

    SafeArrayDestroy(sa);
    CoTaskMemFree(out_arr);

    /*
     * Conversion from native int array to VT_I4 SAFEARRAY.
     */
    sa = NULL;
    hr = IUIAutomation_IntNativeArrayToSafeArray(uia_iface, in_arr, ARRAY_SIZE(in_arr), &sa);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = SafeArrayGetVartype(sa, &vt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(vt == VT_I4, "Unexpected vt %d.\n", vt);

    dims = SafeArrayGetDim(sa);
    ok(dims == 1, "Unexpected array dims %d\n", dims);

    hr = SafeArrayGetLBound(sa, 1, &lbound);
    ok(hr == S_OK, "Failed to get LBound with hr %#lx\n", hr);
    ok(lbound == 0, "Unexpected LBound %#lx\n", lbound);

    hr = SafeArrayGetUBound(sa, 1, &ubound);
    ok(hr == S_OK, "Failed to get UBound with hr %#lx\n", hr);
    ok(((ubound - lbound) + 1) == ARRAY_SIZE(in_arr), "Unexpected array size %#lx\n", ((ubound - lbound) + 1));

    for (i = 0; i < ARRAY_SIZE(in_arr); i++)
    {
        LONG idx = lbound + i;
        int tmp_val;

        hr = SafeArrayGetElement(sa, &idx, &tmp_val);
        ok(hr == S_OK, "Failed to get element at idx %ld, hr %#lx\n", idx, hr);
        ok(tmp_val == in_arr[i], "Expected %d at idx %d, got %d\n", in_arr[i], i, tmp_val);
    }

    SafeArrayDestroy(sa);
}

static HRESULT WINAPI Object_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI Object_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI Object_Release(IUnknown *iface)
{
    return 1;
}

static IUnknownVtbl ObjectVtbl = {
    Object_QueryInterface,
    Object_AddRef,
    Object_Release
};
static IUnknown Object = {&ObjectVtbl};

static void test_CUIAutomation_condition_ifaces(IUIAutomation *uia_iface)
{
    IUIAutomationPropertyCondition *prop_cond, *prop_cond2;
    IUIAutomationCondition *cond, **cond_arr;
    enum PropertyConditionFlags prop_flags;
    IUIAutomationBoolCondition *bool_cond;
    IUIAutomationNotCondition *not_cond;
    IUIAutomationOrCondition *or_cond;
    PROPERTYID prop_id;
    int child_count;
    BOOL tmp_b;
    HRESULT hr;
    VARIANT v;
    ULONG ref;

    hr = IUIAutomation_CreateTrueCondition(uia_iface, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    cond = NULL;
    hr = IUIAutomation_CreateTrueCondition(uia_iface, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    hr = IUIAutomationCondition_QueryInterface(cond, &IID_IUIAutomationBoolCondition, (void **)&bool_cond);
    IUIAutomationCondition_Release(cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!bool_cond, "bool_cond == NULL\n");

    hr = IUIAutomationBoolCondition_get_BooleanValue(bool_cond, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    tmp_b = FALSE;
    hr = IUIAutomationBoolCondition_get_BooleanValue(bool_cond, &tmp_b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(tmp_b == TRUE, "tmp_b != TRUE\n");
    IUIAutomationBoolCondition_Release(bool_cond);

    hr = IUIAutomation_CreateFalseCondition(uia_iface, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    cond = NULL;
    hr = IUIAutomation_CreateFalseCondition(uia_iface, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    hr = IUIAutomationCondition_QueryInterface(cond, &IID_IUIAutomationBoolCondition, (void **)&bool_cond);
    IUIAutomationCondition_Release(cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!bool_cond, "bool_cond == NULL\n");

    hr = IUIAutomationBoolCondition_get_BooleanValue(bool_cond, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    tmp_b = TRUE;
    hr = IUIAutomationBoolCondition_get_BooleanValue(bool_cond, &tmp_b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(tmp_b == FALSE, "tmp_b != FALSE\n");
    IUIAutomationBoolCondition_Release(bool_cond);

    /*
     * IUIAutomationPropertyCondition tests.
     */
    cond = (void *)0xdeadbeef;
    VariantInit(&v);
    /* Invalid property ID. */
    hr = IUIAutomation_CreatePropertyCondition(uia_iface, 0, v, &cond);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!cond, "cond != NULL\n");

    /* Invalid variant type for property ID. */
    cond = (void *)0xdeadbeef;
    VariantInit(&v);
    hr = IUIAutomation_CreatePropertyCondition(uia_iface, UIA_RuntimeIdPropertyId, v, &cond);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!cond, "cond != NULL\n");

    /* NULL Condition argument. */
    V_VT(&v) = VT_I4 | VT_ARRAY;
    V_ARRAY(&v) = create_i4_safearray();
    hr = IUIAutomation_CreatePropertyCondition(uia_iface, UIA_RuntimeIdPropertyId, v, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* Finally, create property condition interface. */
    cond = NULL;
    hr = IUIAutomation_CreatePropertyCondition(uia_iface, UIA_RuntimeIdPropertyId, v, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    hr = IUIAutomationCondition_QueryInterface(cond, &IID_IUIAutomationPropertyCondition, (void **)&prop_cond);
    IUIAutomationCondition_Release(cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!prop_cond, "prop_cond == NULL\n");

    hr = IUIAutomationPropertyCondition_get_PropertyId(prop_cond, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomationPropertyCondition_get_PropertyId(prop_cond, &prop_id);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(prop_id == UIA_RuntimeIdPropertyId, "Unexpected prop_id %d.\n", prop_id);

    hr = IUIAutomationPropertyCondition_get_PropertyValue(prop_cond, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    VariantClear(&v);
    hr = IUIAutomationPropertyCondition_get_PropertyValue(prop_cond, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == (VT_I4 | VT_ARRAY), "Unexpected vt %d.\n", V_VT(&v));
    ok(!!V_ARRAY(&v), "V_ARRAY(&v) == NULL\n");
    VariantClear(&v);

    hr = IUIAutomationPropertyCondition_get_PropertyConditionFlags(prop_cond, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomationPropertyCondition_get_PropertyConditionFlags(prop_cond, &prop_flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(prop_flags == PropertyConditionFlags_None, "Unexpected flags %#x.\n", prop_flags);

    /*
     * IUIAutomationNotCondition tests.
     */
    cond = (void *)0xdeadbeef;

    /*
     * Passing in an interface that isn't a valid IUIAutomationCondition
     * interface.
     */
    hr = IUIAutomation_CreateNotCondition(uia_iface, (IUIAutomationCondition *)&Object, &cond);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(!cond, "cond != NULL\n");

    /* NULL input argument tests. */
    cond = (void *)0xdeadbeef;
    hr = IUIAutomation_CreateNotCondition(uia_iface, NULL, &cond);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(!cond, "cond != NULL\n");

    hr = IUIAutomation_CreateNotCondition(uia_iface, (IUIAutomationCondition *)prop_cond, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* Create the IUIAutomationNotCondition with our property condition. */
    cond = NULL;
    hr = IUIAutomation_CreateNotCondition(uia_iface, (IUIAutomationCondition *)prop_cond, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    /* IUIAutomationNotCondition holds a reference to the passed in condition. */
    ref = IUIAutomationPropertyCondition_Release(prop_cond);
    ok(ref == 1, "Unexpected ref %ld\n", ref);

    hr = IUIAutomationCondition_QueryInterface(cond, &IID_IUIAutomationNotCondition, (void **)&not_cond);
    IUIAutomationCondition_Release(cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!not_cond, "not_cond == NULL\n");

    hr = IUIAutomationNotCondition_GetChild(not_cond, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    cond = NULL;
    hr = IUIAutomationNotCondition_GetChild(not_cond, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(iface_cmp((IUnknown *)cond, (IUnknown *)prop_cond), "cond != prop_cond\n");
    IUIAutomationCondition_Release(cond);

    IUIAutomationNotCondition_Release(not_cond);

    /*
     * IUIAutomationOrCondition tests.
     */
    cond = NULL;
    VariantInit(&v);
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_FALSE;
    /* Create two condition interfaces to use for our IUIAutomationOrCondition. */
    hr = IUIAutomation_CreatePropertyCondition(uia_iface, UIA_IsControlElementPropertyId, v, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    hr = IUIAutomationCondition_QueryInterface(cond, &IID_IUIAutomationPropertyCondition, (void **)&prop_cond);
    IUIAutomationCondition_Release(cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!prop_cond, "prop_cond == NULL\n");

    cond = NULL;
    VariantInit(&v);
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_TRUE;
    hr = IUIAutomation_CreatePropertyCondition(uia_iface, UIA_IsContentElementPropertyId, v, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    hr = IUIAutomationCondition_QueryInterface(cond, &IID_IUIAutomationPropertyCondition, (void **)&prop_cond2);
    IUIAutomationCondition_Release(cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!prop_cond2, "prop_cond2 == NULL\n");

    /* NULL input argument tests. */
    hr = IUIAutomation_CreateOrCondition(uia_iface, (IUIAutomationCondition *)prop_cond,
            (IUIAutomationCondition *)prop_cond2, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    cond = (void *)0xdeadbeef;
    hr = IUIAutomation_CreateOrCondition(uia_iface, NULL, (IUIAutomationCondition *)prop_cond2, &cond);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(!cond, "cond != NULL\n");

    cond = (void *)0xdeadbeef;
    hr = IUIAutomation_CreateOrCondition(uia_iface, (IUIAutomationCondition *)prop_cond, NULL, &cond);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(!cond, "cond != NULL\n");

    /* One of the IUIAutomationCondition interfaces are invalid. */
    cond = (void *)0xdeadbeef;
    hr = IUIAutomation_CreateOrCondition(uia_iface, (IUIAutomationCondition *)prop_cond,
            (IUIAutomationCondition *)&Object, &cond);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
    ok(!cond, "cond != NULL\n");

    cond = NULL;
    hr = IUIAutomation_CreateOrCondition(uia_iface, (IUIAutomationCondition *)prop_cond,
            (IUIAutomationCondition *)prop_cond2, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    or_cond = NULL;
    hr = IUIAutomationCondition_QueryInterface(cond, &IID_IUIAutomationOrCondition, (void **)&or_cond);
    IUIAutomationCondition_Release(cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!or_cond, "or_cond == NULL\n");

    /* References held to both passed in interfaces. */
    ref = IUIAutomationPropertyCondition_Release(prop_cond);
    ok(ref == 1, "Unexpected ref %ld\n", ref);

    ref = IUIAutomationPropertyCondition_Release(prop_cond2);
    ok(ref == 1, "Unexpected ref %ld\n", ref);

    hr = IUIAutomationOrCondition_get_ChildCount(or_cond, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    child_count = 0;
    hr = IUIAutomationOrCondition_get_ChildCount(or_cond, &child_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(child_count == 2, "Unexpected child_count %d.\n", child_count);

    child_count = 10;
    hr = IUIAutomationOrCondition_GetChildrenAsNativeArray(or_cond, NULL, &child_count);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(child_count == 10, "Unexpected child_count %d.\n", child_count);

    cond_arr = (void *)0xdeadbeef;
    hr = IUIAutomationOrCondition_GetChildrenAsNativeArray(or_cond, &cond_arr, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(!cond_arr, "cond_arr != NULL\n");

    child_count = 0;
    cond_arr = NULL;
    hr = IUIAutomationOrCondition_GetChildrenAsNativeArray(or_cond, &cond_arr, &child_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(child_count == 2, "Unexpected child_count %d.\n", child_count);
    ok(!!cond_arr, "cond_arr == NULL\n");

    ok(iface_cmp((IUnknown *)cond_arr[0], (IUnknown *)prop_cond), "cond_arr[0] != prop_cond\n");
    IUIAutomationCondition_Release(cond_arr[0]);

    ok(iface_cmp((IUnknown *)cond_arr[1], (IUnknown *)prop_cond2), "cond_arr[1] != prop_cond2\n");
    IUIAutomationCondition_Release(cond_arr[1]);

    CoTaskMemFree(cond_arr);
    IUIAutomationOrCondition_Release(or_cond);

    /*
     * Condition used to get the control TreeView. Equivalent to:
     * if (!(UIA_IsControlElementPropertyId == VARIANT_FALSE))
     */
    hr = IUIAutomation_get_ControlViewCondition(uia_iface, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    cond = NULL;
    hr = IUIAutomation_get_ControlViewCondition(uia_iface, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    hr = IUIAutomationCondition_QueryInterface(cond, &IID_IUIAutomationNotCondition, (void **)&not_cond);
    IUIAutomationCondition_Release(cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!not_cond, "not_cond == NULL\n");

    cond = NULL;
    hr = IUIAutomationNotCondition_GetChild(not_cond, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    hr = IUIAutomationCondition_QueryInterface(cond, &IID_IUIAutomationPropertyCondition, (void **)&prop_cond);
    IUIAutomationCondition_Release(cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!prop_cond, "prop_cond == NULL\n");

    hr = IUIAutomationPropertyCondition_get_PropertyId(prop_cond, &prop_id);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(prop_id == UIA_IsControlElementPropertyId, "Unexpected prop_id %d.\n", prop_id);

    VariantInit(&v);
    hr = IUIAutomationPropertyCondition_get_PropertyValue(prop_cond, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(check_variant_bool(&v, FALSE), "Unexpected BOOL %#x\n", V_BOOL(&v));
    VariantClear(&v);

    hr = IUIAutomationPropertyCondition_get_PropertyConditionFlags(prop_cond, &prop_flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(prop_flags == PropertyConditionFlags_None, "Unexpected flags %#x.\n", prop_flags);

    IUIAutomationPropertyCondition_Release(prop_cond);
    IUIAutomationNotCondition_Release(not_cond);

    /*
     * Condition used to get the raw TreeView. Equivalent to:
     * if (1)
     */
    hr = IUIAutomation_get_RawViewCondition(uia_iface, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    cond = NULL;
    hr = IUIAutomation_get_RawViewCondition(uia_iface, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    hr = IUIAutomationCondition_QueryInterface(cond, &IID_IUIAutomationBoolCondition, (void **)&bool_cond);
    IUIAutomationCondition_Release(cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!bool_cond, "bool_cond == NULL\n");

    tmp_b = FALSE;
    hr = IUIAutomationBoolCondition_get_BooleanValue(bool_cond, &tmp_b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(tmp_b == TRUE, "tmp_b != TRUE\n");
    IUIAutomationBoolCondition_Release(bool_cond);
}

static void test_CUIAutomation_cache_request_iface(IUIAutomation *uia_iface)
{
    IUIAutomationPropertyCondition *prop_cond;
    enum PropertyConditionFlags prop_flags;
    IUIAutomationCacheRequest *cache_req;
    enum AutomationElementMode elem_mode;
    IUIAutomationCondition *cond, *cond2;
    IUIAutomationNotCondition *not_cond;
    enum TreeScope scope;
    PROPERTYID prop_id;
    HRESULT hr;
    VARIANT v;

    hr = IUIAutomation_CreateCacheRequest(uia_iface, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /*
     * CreateCacheRequest returns an IUIAutomationCacheRequest with the
     * default cache request values set.
     */
    cache_req = NULL;
    hr = IUIAutomation_CreateCacheRequest(uia_iface, &cache_req);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cache_req, "cache_req == NULL\n");

    /*
     * TreeScope tests.
     */
    hr = IUIAutomationCacheRequest_get_TreeScope(cache_req, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    scope = 0;
    hr = IUIAutomationCacheRequest_get_TreeScope(cache_req, &scope);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(scope == TreeScope_Element, "Unexpected scope %#x\n", scope);

    /* Set it to something invalid. */
    hr = IUIAutomationCacheRequest_put_TreeScope(cache_req, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomationCacheRequest_put_TreeScope(cache_req, TreeScope_Parent);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomationCacheRequest_put_TreeScope(cache_req, TreeScope_Ancestors);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomationCacheRequest_put_TreeScope(cache_req, TreeScope_Parent | TreeScope_Element);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomationCacheRequest_put_TreeScope(cache_req, TreeScope_Ancestors | TreeScope_Element);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomationCacheRequest_put_TreeScope(cache_req, ~(TreeScope_Subtree | TreeScope_Parent | TreeScope_Ancestors));
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Invalid values don't change anything. */
    scope = 0;
    hr = IUIAutomationCacheRequest_get_TreeScope(cache_req, &scope);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(scope == TreeScope_Element, "Unexpected scope %#x\n", scope);

    /* Now set it to TreeScope_Children. */
    hr = IUIAutomationCacheRequest_put_TreeScope(cache_req, TreeScope_Children);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    scope = 0;
    hr = IUIAutomationCacheRequest_get_TreeScope(cache_req, &scope);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(scope == TreeScope_Children, "Unexpected scope %#x\n", scope);

    /*
     * TreeFilter tests.
     */
    cond = NULL;
    hr = IUIAutomationCacheRequest_get_TreeFilter(cache_req, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    cond = NULL;
    hr = IUIAutomationCacheRequest_put_TreeFilter(cache_req, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    cond = NULL;
    hr = IUIAutomationCacheRequest_put_TreeFilter(cache_req, (IUIAutomationCondition *)&Object);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* Default IUIAutomationCacheRequest has the ControlView condition. */
    cond = NULL;
    hr = IUIAutomationCacheRequest_get_TreeFilter(cache_req, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    hr = IUIAutomationCondition_QueryInterface(cond, &IID_IUIAutomationNotCondition, (void **)&not_cond);
    IUIAutomationCondition_Release(cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!not_cond, "not_cond == NULL\n");

    cond = NULL;
    hr = IUIAutomationNotCondition_GetChild(not_cond, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    hr = IUIAutomationCondition_QueryInterface(cond, &IID_IUIAutomationPropertyCondition, (void **)&prop_cond);
    IUIAutomationCondition_Release(cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!prop_cond, "prop_cond == NULL\n");

    hr = IUIAutomationPropertyCondition_get_PropertyId(prop_cond, &prop_id);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(prop_id == UIA_IsControlElementPropertyId, "Unexpected prop_id %d.\n", prop_id);

    VariantInit(&v);
    hr = IUIAutomationPropertyCondition_get_PropertyValue(prop_cond, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(check_variant_bool(&v, FALSE), "Unexpected BOOL %#x\n", V_BOOL(&v));
    VariantClear(&v);

    hr = IUIAutomationPropertyCondition_get_PropertyConditionFlags(prop_cond, &prop_flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(prop_flags == PropertyConditionFlags_None, "Unexpected flags %#x.\n", prop_flags);

    IUIAutomationPropertyCondition_Release(prop_cond);
    IUIAutomationNotCondition_Release(not_cond);

    /* Set a new TreeFilter condition. */
    cond = NULL;
    hr = IUIAutomation_CreateTrueCondition(uia_iface, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    hr = IUIAutomationCacheRequest_put_TreeFilter(cache_req, cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomationCacheRequest_get_TreeFilter(cache_req, &cond2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond2, "cond2 == NULL\n");
    ok(iface_cmp((IUnknown *)cond, (IUnknown *)cond2), "cond != cond2\n");
    IUIAutomationCondition_Release(cond);
    IUIAutomationCondition_Release(cond2);

    /*
     * AutomationElementMode tests.
     */
    hr = IUIAutomationCacheRequest_get_AutomationElementMode(cache_req, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    elem_mode = 0;
    hr = IUIAutomationCacheRequest_get_AutomationElementMode(cache_req, &elem_mode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(elem_mode == AutomationElementMode_Full, "Unexpected element mode %#x\n", elem_mode);

    /* Invalid value - maximum is AutomationElementMode_Full, 0x01. */
    hr = IUIAutomationCacheRequest_put_AutomationElementMode(cache_req, 0x02);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomationCacheRequest_put_AutomationElementMode(cache_req, AutomationElementMode_None);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    elem_mode = 0;
    hr = IUIAutomationCacheRequest_get_AutomationElementMode(cache_req, &elem_mode);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(elem_mode == AutomationElementMode_None, "Unexpected element mode %#x\n", elem_mode);

    /*
     * AddProperty tests.
     */
    hr = IUIAutomationCacheRequest_AddProperty(cache_req, UIA_IsContentElementPropertyId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Invalid property ID. */
    hr = IUIAutomationCacheRequest_AddProperty(cache_req, 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IUIAutomationCacheRequest_Release(cache_req);
}

static const struct prov_method_sequence get_elem_cache_seq[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    NODE_CREATE_SEQ(&Provider_child),
    { 0 },
};

static const struct prov_method_sequence get_cached_prop_val_seq[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { 0 },
};

static const struct prov_method_sequence get_cached_prop_val_seq2[] = {
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { 0 },
};

static const struct prov_method_sequence get_cached_prop_val_seq3[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE },
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider, PROV_GET_PROPERTY_VALUE },
    NODE_CREATE_SEQ(&Provider_child),
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ControlTypePropertyId */
    { 0 },
};

static void test_Element_cache_methods(IUIAutomation *uia_iface)
{
    static const int cache_test_props[] = { UIA_IsKeyboardFocusablePropertyId, UIA_NamePropertyId, UIA_ControlTypePropertyId,
                                            UIA_BoundingRectanglePropertyId, UIA_HasKeyboardFocusPropertyId, };
    HWND hwnd = create_test_hwnd("test_Element_cache_methods class");
    IUIAutomationElement *element, *element2, *element3;
    struct Provider_prop_override prop_override;
    IUIAutomationCacheRequest *cache_req;
    IUIAutomationElementArray *elem_arr;
    int tmp_rt_id[2], i, len, tmp_int;
    IUnknown *unk_ns;
    BSTR tmp_bstr;
    BOOL tmp_bool;
    HRESULT hr;
    RECT rect;
    VARIANT v;

    element = create_test_element_from_hwnd(uia_iface, hwnd, TRUE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    provider_add_child(&Provider, &Provider_child);

    hr = UiaGetReservedNotSupportedValue(&unk_ns);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    cache_req = NULL;
    hr = IUIAutomation_CreateCacheRequest(uia_iface, &cache_req);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cache_req, "cache_req == NULL\n");

    /*
     * Run these tests on Provider_child, it doesn't have an HWND so it will
     * get UIA_RuntimeIdPropertyId from GetRuntimeId.
     */
    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element, UIA_LabeledByPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok_method_sequence(get_elem_cache_seq, "get_elem_cache_seq");

    IUIAutomationElement_Release(element);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    hr = IUnknown_QueryInterface(V_UNKNOWN(&v), &IID_IUIAutomationElement, (void **)&element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!element, "element == NULL\n");
    VariantClear(&v);

    /*
     * Passing in an invalid COM interface for IUIAutomationCacheRequest will
     * cause an access violation on Windows.
     */
    if (0)
    {
        IUIAutomationElement_BuildUpdatedCache(element, (IUIAutomationCacheRequest *)&Object, &element2);
    }

    hr = IUIAutomationElement_BuildUpdatedCache(element, cache_req, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    element2 = (void *)0xdeadbeef;
    hr = IUIAutomationElement_BuildUpdatedCache(element, NULL, &element2);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(!element2, "element2 != NULL\n");

    /*
     * Test cached property values. The default IUIAutomationCacheRequest
     * always caches UIA_RuntimeIdPropertyId.
     */
    element2 = NULL;
    hr = IUIAutomationElement_BuildUpdatedCache(element, cache_req, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!element2, "element2 == NULL\n");
    ok_method_sequence(get_cached_prop_val_seq, "get_cached_prop_val_seq");

    /* RuntimeId is currently unset, so we'll get the NotSupported value. */
    hr = IUIAutomationElement_GetCachedPropertyValueEx(element2, UIA_RuntimeIdPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(V_UNKNOWN(&v) == unk_ns, "unexpected IUnknown %p\n", V_UNKNOWN(&v));
    VariantClear(&v);

    /* Attempting to get a cached value for a non-cached property. */
    hr = IUIAutomationElement_GetCachedPropertyValueEx(element2, UIA_IsControlElementPropertyId, TRUE, &v);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_EMPTY, "Unexpected vt %d\n", V_VT(&v));
    VariantClear(&v);

    IUIAutomationElement_Release(element2);

    /* RuntimeId is now set. */
    Provider_child.runtime_id[0] = Provider_child.runtime_id[1] = 0xdeadbeef;
    element2 = NULL;
    hr = IUIAutomationElement_BuildUpdatedCache(element, cache_req, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!element2, "element2 == NULL\n");
    ok_method_sequence(get_cached_prop_val_seq, "get_cached_prop_val_seq");

    hr = IUIAutomationElement_GetCachedPropertyValueEx(element2, UIA_RuntimeIdPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == (VT_I4 | VT_ARRAY), "Unexpected vt %d\n", V_VT(&v));
    check_runtime_id(Provider_child.runtime_id, ARRAY_SIZE(Provider_child.runtime_id), V_ARRAY(&v));
    VariantClear(&v);
    IUIAutomationElement_Release(element2);

    /*
     * Add UIA_IsControlElementPropertyId to the list of cached property
     * values.
     */
    hr = IUIAutomationCacheRequest_AddProperty(cache_req, UIA_IsControlElementPropertyId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    Provider_child.runtime_id[0] = Provider_child.runtime_id[1] = 0xdeadb33f;
    element2 = NULL;
    hr = IUIAutomationElement_BuildUpdatedCache(element, cache_req, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!element2, "element2 == NULL\n");
    ok_method_sequence(get_cached_prop_val_seq2, "get_cached_prop_val_seq2");

    hr = IUIAutomationElement_GetCachedPropertyValueEx(element2, UIA_RuntimeIdPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == (VT_I4 | VT_ARRAY), "Unexpected vt %d\n", V_VT(&v));
    check_runtime_id(Provider_child.runtime_id, ARRAY_SIZE(Provider_child.runtime_id), V_ARRAY(&v));
    VariantClear(&v);

    hr = IUIAutomationElement_GetCachedPropertyValueEx(element2, UIA_IsControlElementPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(check_variant_bool(&v, TRUE), "V_BOOL(&v) = %#x\n", V_BOOL(&v));
    VariantClear(&v);

    IUIAutomationElement_Release(element2);
    IUIAutomationCacheRequest_Release(cache_req);

    IUIAutomationElement_Release(element);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    /* Test cached UIAutomationType_Element properties. */
    element = create_test_element_from_hwnd(uia_iface, hwnd, TRUE);

    cache_req = NULL;
    hr = IUIAutomation_CreateCacheRequest(uia_iface, &cache_req);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cache_req, "cache_req == NULL\n");

    /* UIAutomationType_Element property. */
    hr = IUIAutomationCacheRequest_AddProperty(cache_req, UIA_LabeledByPropertyId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* UIAutomationType_ElementArray property. */
    hr = IUIAutomationCacheRequest_AddProperty(cache_req, UIA_ControllerForPropertyId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    element2 = NULL;
    hr = IUIAutomationElement_BuildUpdatedCache(element, cache_req, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!element2, "element2 == NULL\n");
    ok(Provider_child.ref == 3, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);

    tmp_rt_id[0] = UIA_RUNTIME_ID_PREFIX;
    tmp_rt_id[1] = HandleToULong(hwnd);
    hr = IUIAutomationElement_GetCachedPropertyValueEx(element2, UIA_RuntimeIdPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == (VT_I4 | VT_ARRAY), "Unexpected vt %d\n", V_VT(&v));
    check_runtime_id(tmp_rt_id, ARRAY_SIZE(tmp_rt_id), V_ARRAY(&v));
    VariantClear(&v);

    /* Cached IUIAutomationElement. */
    hr = IUIAutomationElement_GetCachedPropertyValueEx(element2, UIA_LabeledByPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));

    element3 = NULL;
    hr = IUnknown_QueryInterface(V_UNKNOWN(&v), &IID_IUIAutomationElement, (void **)&element3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!element3, "element3 == NULL\n");
    VariantClear(&v);

    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element3, UIA_IsControlElementPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(check_variant_bool(&v, TRUE), "V_BOOL(&v) = %#x\n", V_BOOL(&v));
    IUIAutomationElement_Release(element3);
    VariantClear(&v);

    /* Cached IUIAutomationElementArray. */
    hr = IUIAutomationElement_GetCachedPropertyValueEx(element2, UIA_ControllerForPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));

    hr = IUnknown_QueryInterface(V_UNKNOWN(&v), &IID_IUIAutomationElementArray, (void **)&elem_arr);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(!!elem_arr, "elem_arr == NULL\n");
    VariantClear(&v);

    hr = IUIAutomationElementArray_get_Length(elem_arr, &len);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(len == ARRAY_SIZE(uia_unk_arr_prop_val), "Unexpected length %d\n", len);

    for (i = 0; i < ARRAY_SIZE(uia_unk_arr_prop_val); i++)
    {
        hr = IUIAutomationElementArray_GetElement(elem_arr, i, &element3);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
        ok(!!element3, "element3 == NULL\n");

        hr = IUIAutomationElement_GetCurrentPropertyValueEx(element3, UIA_ControlTypePropertyId, TRUE, &v);
        ok(hr == S_OK, "elem[%d] Unexpected hr %#lx\n", i, hr);
        ok(V_VT(&v) == VT_I4, "elem[%d] Unexpected VT %d\n", i, V_VT(&v));
        ok(V_I4(&v) == uia_i4_prop_val, "elem[%d] Unexpected I4 %#lx\n", i, V_I4(&v));

        IUIAutomationElement_Release(element3);
        VariantClear(&v);
    }

    IUIAutomationElementArray_Release(elem_arr);
    IUIAutomationCacheRequest_Release(cache_req);

    /*
     * Reference isn't released until the element holding the cache is
     * destroyed.
     */
    ok(Provider_child.ref == 3, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);

    IUIAutomationElement_Release(element2);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(Provider_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_child2.ref);
    ok_method_sequence(get_cached_prop_val_seq3, "get_cached_prop_val_seq3");

    IUIAutomationElement_Release(element);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    IUnknown_Release(unk_ns);

    /*
     * Windows 7 will call get_FragmentRoot in an endless loop until the fragment root returns an HWND.
     * It's the only version with this behavior.
     */
    if (!UiaLookupId(AutomationIdentifierType_Property, &OptimizeForVisualContent_Property_GUID))
    {
        win_skip("Skipping cached UIA_BoundingRectanglePropertyId tests for Win7\n");
        goto exit;
    }

    /*
     * Cached property value helper function tests.
     */
    element = create_test_element_from_hwnd(uia_iface, hwnd, TRUE);
    method_sequences_enabled = FALSE;

    /*
     * element has no cached values, element2 has cached values but they're
     * all the equivalent of VT_EMPTY, element3 has valid cached values.
     */
    cache_req = NULL;
    hr = IUIAutomation_CreateCacheRequest(uia_iface, &cache_req);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cache_req, "cache_req == NULL\n");

    for (i = 0; i < ARRAY_SIZE(cache_test_props); i++)
    {
        hr = IUIAutomationCacheRequest_AddProperty(cache_req, cache_test_props[i]);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }

    /* element2, invalid values for all cached properties. */
    element2 = NULL;
    Provider.ret_invalid_prop_type = TRUE;
    set_uia_rect(&Provider.bounds_rect, 0, 0, 0, 0);
    hr = IUIAutomationElement_BuildUpdatedCache(element, cache_req, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!element2, "element2 == NULL\n");
    Provider.ret_invalid_prop_type = FALSE;

    /* element3, valid values for all cached properties. */
    V_VT(&v) = VT_I4;
    V_I4(&v) = UIA_HyperlinkControlTypeId;
    set_property_override(&prop_override, UIA_ControlTypePropertyId, &v);
    set_provider_prop_override(&Provider, &prop_override, 1);
    set_uia_rect(&Provider.bounds_rect, 0, 0, 50, 50);

    element3 = NULL;
    hr = IUIAutomationElement_BuildUpdatedCache(element, cache_req, &element3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!element3, "element3 == NULL\n");
    set_provider_prop_override(&Provider, NULL, 0);

    IUIAutomationCacheRequest_Release(cache_req);

    /* Cached UIA_HasKeyboardFocusPropertyId helper. */
    hr = IUIAutomationElement_get_CachedHasKeyboardFocus(element, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    tmp_bool = 0xdeadbeef;
    hr = IUIAutomationElement_get_CachedHasKeyboardFocus(element, &tmp_bool);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(tmp_bool == 0xdeadbeef, "Unexpected tmp_bool %d\n", tmp_bool);

    tmp_bool = 0xdeadbeef;
    hr = IUIAutomationElement_get_CachedHasKeyboardFocus(element2, &tmp_bool);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!tmp_bool, "tmp_bool != FALSE\n");

    tmp_bool = FALSE;
    hr = IUIAutomationElement_get_CachedHasKeyboardFocus(element3, &tmp_bool);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!tmp_bool, "tmp_bool == FALSE\n");

    /* Cached UIA_IsKeyboardFocusablePropertyId helper. */
    hr = IUIAutomationElement_get_CachedIsKeyboardFocusable(element, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    tmp_bool = 0xdeadbeef;
    hr = IUIAutomationElement_get_CachedIsKeyboardFocusable(element, &tmp_bool);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(tmp_bool == 0xdeadbeef, "Unexpected tmp_bool %d\n", tmp_bool);

    tmp_bool = 0xdeadbeef;
    hr = IUIAutomationElement_get_CachedIsKeyboardFocusable(element2, &tmp_bool);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!tmp_bool, "tmp_bool != FALSE\n");

    tmp_bool = FALSE;
    hr = IUIAutomationElement_get_CachedIsKeyboardFocusable(element3, &tmp_bool);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!tmp_bool, "tmp_bool == FALSE\n");

    /* Cached UIA_NamePropertyId helper. */
    hr = IUIAutomationElement_get_CachedName(element, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    tmp_bstr = (void *)0xdeadbeef;
    hr = IUIAutomationElement_get_CachedName(element, &tmp_bstr);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(tmp_bstr == (void *)0xdeadbeef, "Unexpected BSTR ptr %p\n", tmp_bstr);

    tmp_bstr = NULL;
    hr = IUIAutomationElement_get_CachedName(element2, &tmp_bstr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(tmp_bstr, L""), "Unexpected BSTR %s\n", wine_dbgstr_w(tmp_bstr));
    SysFreeString(tmp_bstr);

    tmp_bstr = NULL;
    hr = IUIAutomationElement_get_CachedName(element3, &tmp_bstr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(tmp_bstr, uia_bstr_prop_str), "Unexpected BSTR %s\n", wine_dbgstr_w(tmp_bstr));
    SysFreeString(tmp_bstr);

    /* Cached UIA_ControlTypePropertyId. */
    hr = IUIAutomationElement_get_CachedControlType(element, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    tmp_int = 0xdeadbeef;
    hr = IUIAutomationElement_get_CachedControlType(element, &tmp_int);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(tmp_int == 0xdeadbeef, "Unexpected control type %#x\n", tmp_int);

    tmp_int = 0;
    hr = IUIAutomationElement_get_CachedControlType(element2, &tmp_int);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(tmp_int == UIA_CustomControlTypeId, "Unexpected control type %#x\n", tmp_int);

    tmp_int = 0;
    hr = IUIAutomationElement_get_CachedControlType(element3, &tmp_int);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(tmp_int == UIA_HyperlinkControlTypeId, "Unexpected control type %#x\n", tmp_int);

    /* Cached UIA_BoundingRectanglePropertyId helper. */
    hr = IUIAutomationElement_get_CachedBoundingRectangle(element, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    rect.left = rect.top = rect.bottom = rect.right = 1;
    hr = IUIAutomationElement_get_CachedBoundingRectangle(element, &rect);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(rect.left == 1, "Unexpected rect left %ld\n", rect.left);
    ok(rect.top == 1, "Unexpected rect top %ld\n", rect.top);
    ok(rect.right == 1, "Unexpected rect right %ld\n", rect.right);
    ok(rect.bottom == 1, "Unexpected rect bottom %ld\n", rect.bottom);

    rect.left = rect.top = rect.bottom = rect.right = 1;
    hr = IUIAutomationElement_get_CachedBoundingRectangle(element2, &rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!rect.left, "Unexpected rect left %ld\n", rect.left);
    ok(!rect.top, "Unexpected rect top %ld\n", rect.top);
    ok(!rect.right, "Unexpected rect right %ld\n", rect.right);
    ok(!rect.bottom, "Unexpected rect bottom %ld\n", rect.bottom);

    memset(&rect, 0, sizeof(rect));
    hr = IUIAutomationElement_get_CachedBoundingRectangle(element3, &rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_rect_rect_val(&rect, &Provider.bounds_rect);

    IUIAutomationElement_Release(element3);
    IUIAutomationElement_Release(element2);
    IUIAutomationElement_Release(element);

    set_uia_rect(&Provider.bounds_rect, 0, 0, 0, 0);
    method_sequences_enabled = TRUE;

exit:
    DestroyWindow(hwnd);
    UnregisterClassA("test_Element_cache_methods class", NULL);
}

static const struct prov_method_sequence element_find_start_seq[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_LabeledByPropertyId */
    NODE_CREATE_SEQ(&Provider),
    { 0 }
};

/*
 * Identical to find_seq7, except default cache request used by FindAll
 * doesn't cache UIA_RuntimeIdPropertyId.
 */
static const struct prov_method_sequence element_find_seq1[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child_child),
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child_child2),
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    /* Only done on Win10v1507 and below. */
    { &Provider, FRAG_NAVIGATE, METHOD_OPTIONAL }, /* NavigateDirection_Parent */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { &Provider_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

/*
 * Identical to find_seq11, except default cache request used by FindFirst
 * doesn't cache UIA_RuntimeIdPropertyId.
 */
static const struct prov_method_sequence element_find_seq2[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child_child),
    { &Provider_child_child, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    { &Provider_child_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child_child2),
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsContentElementPropertyId */
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_IsControlElementPropertyId */
    { &Provider_child_child2, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 },
};

struct exp_elem_desc {
    struct Provider *elem_prov;
    struct node_provider_desc prov_desc;
    ULONG exp_refcnt;
    ULONG exp_release_refcnt;
};

static void set_elem_desc(struct exp_elem_desc *desc, struct Provider *prov, HWND hwnd, DWORD pid, ULONG exp_refcnt,
        ULONG exp_release_refcnt)
{
    desc->elem_prov = prov;
    init_node_provider_desc(&desc->prov_desc, pid, hwnd);
    desc->exp_refcnt = exp_refcnt;
    desc->exp_release_refcnt = exp_release_refcnt;
}

#define test_uia_element_arr( elem_arr, exp_elems, exp_elems_count ) \
        test_uia_element_arr_( (elem_arr), (exp_elems), (exp_elems_count), __FILE__, __LINE__)
static void test_uia_element_arr_(IUIAutomationElementArray *elem_arr, struct exp_elem_desc *exp_elems, int exp_elems_count,
        const char *file, int line)
{
    int i, arr_length;
    HRESULT hr;

    hr = IUIAutomationElementArray_get_Length(elem_arr, &arr_length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_(file, line)(hr == S_OK, "IUIAutomationElementArray_get_Length: Unexpected hr %#lx\n", hr);
    ok_(file, line)(arr_length == exp_elems_count, "Unexpected arr_length %d.\n", arr_length);

    for (i = 0; i < arr_length; i++)
    {
        struct exp_elem_desc *desc = &exp_elems[i];
        IUIAutomationElement *element;
        VARIANT v;

        ok_(file, line)(desc->elem_prov->ref == desc->exp_refcnt, "elem[%d]: Unexpected refcnt %ld\n", i, exp_elems[i].elem_prov->ref);

        VariantInit(&v);
        element = NULL;
        hr = IUIAutomationElementArray_GetElement(elem_arr, i, &element);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(!!element, "element == NULL\n");

        hr = IUIAutomationElement_GetCurrentPropertyValueEx(element, UIA_ProviderDescriptionPropertyId, TRUE, &v);
        ok_(file, line)(hr == S_OK, "Unexpected hr %#lx\n", hr);
        test_node_provider_desc_(&exp_elems[i].prov_desc, V_BSTR(&v), file, line);
        VariantClear(&v);
        IUIAutomationElement_Release(element);
    }

    IUIAutomationElementArray_Release(elem_arr);

    for (i = 0; i < arr_length; i++)
    {
        struct exp_elem_desc *desc = &exp_elems[i];

        ok_(file, line)(desc->elem_prov->ref == desc->exp_release_refcnt, "elem[%d]: Unexpected refcnt %ld\n", i, exp_elems[i].elem_prov->ref);
    }
}

static void test_Element_Find(IUIAutomation *uia_iface)
{
    HWND hwnd = create_test_hwnd("test_Element_Find class");
    IUIAutomationCondition *condition, *condition2;
    struct Provider_prop_override prop_override;
    struct exp_elem_desc exp_elems[7] = { 0 };
    IUIAutomationElement *element, *element2;
    IUIAutomationElementArray *element_arr;
    IUIAutomationCacheRequest *cache_req;
    HRESULT hr;
    VARIANT v;

    element = create_test_element_from_hwnd(uia_iface, hwnd, TRUE);

    /*
     * The COM API has no equivalent to UiaNodeFromProvider, so the only way
     * we can get an initial element is with ElementFromHandle. This means our
     * element representing Provider will have an HWND associated, which
     * doesn't match our old UiaFind tests. To work around this, make Provider
     * return itself for the UIA_LabeledByPropertyId to get an element without
     * an HWND associated.
     */
    VariantInit(&v);
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown *)&Provider.IRawElementProviderSimple_iface;
    set_property_override(&prop_override, UIA_LabeledByPropertyId, &v);
    set_provider_prop_override(&Provider, &prop_override, 1);
    Provider.hwnd = NULL;

    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element, UIA_LabeledByPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(Provider.ref == 3, "Unexpected refcnt %ld\n", Provider.ref);

    hr = IUnknown_QueryInterface(V_UNKNOWN(&v), &IID_IUIAutomationElement, (void **)&element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!element2, "element2 == NULL\n");
    VariantClear(&v);
    ok_method_sequence(element_find_start_seq, "element_find_start_seq");
    set_provider_prop_override(&Provider, NULL, 0);

    IUIAutomationElement_Release(element);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    element = element2;
    element2 = NULL;

    initialize_provider_tree(TRUE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    provider_add_child(&Provider, &Provider_child);
    provider_add_child(&Provider, &Provider_child2);
    provider_add_child(&Provider_child, &Provider_child_child);
    provider_add_child(&Provider_child, &Provider_child_child2);
    provider_add_child(&Provider_child2, &Provider_child2_child);
    provider_add_child(&Provider_child2_child, &Provider_child2_child_child);

    /*
     * In order to match the tests from test_UiaFind(), we need to create a
     * custom IUIAutomationCacheRequest with a ConditionType_True treeview.
     * The default cache request also caches the RuntimeId property.
     */
    cache_req = NULL;
    hr = IUIAutomation_CreateCacheRequest(uia_iface, &cache_req);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cache_req, "cache_req == NULL\n");

    /* Set view condition to ConditionType_True. */
    condition = NULL;
    hr = IUIAutomation_CreateTrueCondition(uia_iface, &condition);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!condition, "condition == NULL\n");

    hr = IUIAutomationCacheRequest_put_TreeFilter(cache_req, condition);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /*
     * Equivalent to: Maximum find depth of -1, find first is FALSE, exclude
     * root is FALSE.
     */
    hr = IUIAutomationElement_FindAllBuildCache(element, TreeScope_Subtree, condition, cache_req, &element_arr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_elem_desc(&exp_elems[0], &Provider, NULL, GetCurrentProcessId(), 2, 2);
    add_provider_desc(&exp_elems[0].prov_desc, L"Main", L"Provider", TRUE);
    set_elem_desc(&exp_elems[1], &Provider_child, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[1].prov_desc, L"Main", L"Provider_child", TRUE);
    set_elem_desc(&exp_elems[2], &Provider_child_child, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[2].prov_desc, L"Main", L"Provider_child_child", TRUE);
    set_elem_desc(&exp_elems[3], &Provider_child_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[3].prov_desc, L"Main", L"Provider_child_child2", TRUE);
    set_elem_desc(&exp_elems[4], &Provider_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[4].prov_desc, L"Main", L"Provider_child2", TRUE);
    set_elem_desc(&exp_elems[5], &Provider_child2_child, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[5].prov_desc, L"Main", L"Provider_child2_child", TRUE);
    set_elem_desc(&exp_elems[6], &Provider_child2_child_child, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[6].prov_desc, L"Main", L"Provider_child2_child_child", TRUE);

    test_uia_element_arr(element_arr, exp_elems, 7);
    ok_method_sequence(find_seq1, "find_seq1");

    /*
     * Equivalent to: Maximum find depth of 1, find first is FALSE, exclude root
     * is FALSE.
     */
    hr = IUIAutomationElement_FindAllBuildCache(element, TreeScope_Element | TreeScope_Children, condition, cache_req, &element_arr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_elem_desc(&exp_elems[0], &Provider, NULL, GetCurrentProcessId(), 2, 2);
    add_provider_desc(&exp_elems[0].prov_desc, L"Main", L"Provider", TRUE);
    set_elem_desc(&exp_elems[1], &Provider_child, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[1].prov_desc, L"Main", L"Provider_child", TRUE);
    set_elem_desc(&exp_elems[2], &Provider_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[2].prov_desc, L"Main", L"Provider_child2", TRUE);

    test_uia_element_arr(element_arr, exp_elems, 3);
    ok_method_sequence(find_seq2, "find_seq2");

    /*
     * Equivalent to: Maximum find depth of 1, find first is FALSE, exclude root
     * is TRUE.
     */
    hr = IUIAutomationElement_FindAllBuildCache(element, TreeScope_Children, condition, cache_req, &element_arr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_elem_desc(&exp_elems[0], &Provider_child, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[0].prov_desc, L"Main", L"Provider_child", TRUE);
    set_elem_desc(&exp_elems[1], &Provider_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[1].prov_desc, L"Main", L"Provider_child2", TRUE);

    test_uia_element_arr(element_arr, exp_elems, 2);
    ok_method_sequence(find_seq3, "find_seq3");

    /*
     * Equivalent to: Maximum find depth of 1, find first is TRUE, exclude
     * root is TRUE. element2 now represents Provider_child.
     */
    hr = IUIAutomationElement_FindFirstBuildCache(element, TreeScope_Children, condition, cache_req, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element2, UIA_ProviderDescriptionPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child", TRUE);
    VariantClear(&v);

    ok_method_sequence(find_seq4, "find_seq4");

    /*
     * Equivalent to: Maximum find depth of 0, find first is FALSE, exclude
     * root is FALSE. Provider_child doesn't have a runtime id for UI
     * Automation to use as a way to check if it has navigated back to the
     * node that began the search, so it will get siblings.
     */
    hr = IUIAutomationElement_FindAllBuildCache(element2, TreeScope_Element, condition, cache_req, &element_arr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_elem_desc(&exp_elems[0], &Provider_child, NULL, GetCurrentProcessId(), 2, 2);
    add_provider_desc(&exp_elems[0].prov_desc, L"Main", L"Provider_child", TRUE);
    set_elem_desc(&exp_elems[1], &Provider_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[1].prov_desc, L"Main", L"Provider_child2", TRUE);

    test_uia_element_arr(element_arr, exp_elems, 2);
    ok_method_sequence(find_seq5, "find_seq5");

    /*
     * Equivalent to: Maximum find depth of 0, find first is FALSE, exclude
     * root is FALSE. Provider_child now has a runtime ID, so we don't get
     * its sibling.
     */
    Provider_child.runtime_id[0] = Provider_child.runtime_id[1] = 0xdeadbeef;
    hr = IUIAutomationElement_FindAllBuildCache(element2, TreeScope_Element, condition, cache_req, &element_arr);
    IUIAutomationElement_Release(element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_elem_desc(&exp_elems[0], &Provider_child, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[0].prov_desc, L"Main", L"Provider_child", TRUE);

    test_uia_element_arr(element_arr, exp_elems, 1);
    ok_method_sequence(find_seq6, "find_seq6");
    initialize_provider_tree(FALSE);

    IUIAutomationCondition_Release(condition);

    /* condition is now UIA_IsContentElementPropertyId == FALSE. */
    condition = NULL;
    variant_init_bool(&v, FALSE);
    hr = IUIAutomation_CreatePropertyCondition(uia_iface, UIA_IsContentElementPropertyId, v, &condition);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!condition, "condition == NULL\n");

    /* Set view condition to this property condition. */
    hr = IUIAutomationCacheRequest_put_TreeFilter(cache_req, condition);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* condition2 is UIA_IsControlElementPropertyId == TRUE. */
    condition2 = NULL;
    variant_init_bool(&v, TRUE);
    hr = IUIAutomation_CreatePropertyCondition(uia_iface, UIA_IsControlElementPropertyId, v, &condition2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!condition2, "condition2 == NULL\n");

    /* Override UIA_IsContentElementPropertyId, set it to FALSE. */
    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsContentElementPropertyId, &v);
    set_provider_prop_override(&Provider, &prop_override, 1);
    set_provider_prop_override(&Provider_child2, &prop_override, 1);
    set_provider_prop_override(&Provider_child_child, &prop_override, 1);
    set_provider_prop_override(&Provider_child_child2, &prop_override, 1);

    /*
     * Equivalent to: Maximum find depth of 1, find first is FALSE, exclude
     * root is FALSE. The cache request view condition is used to determine
     * tree depth, if an element matches the cache request view condition,
     * depth is incremented. Since Provider_child does not, Provider_child_child,
     * Provider_child_child2, and Provider_child2 are all considered to be at
     * depth 1.
     */
    hr = IUIAutomationElement_FindAllBuildCache(element, TreeScope_Element | TreeScope_Children, condition2, cache_req, &element_arr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_elem_desc(&exp_elems[0], &Provider, NULL, GetCurrentProcessId(), 2, 2);
    add_provider_desc(&exp_elems[0].prov_desc, L"Main", L"Provider", TRUE);
    set_elem_desc(&exp_elems[1], &Provider_child_child, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[1].prov_desc, L"Main", L"Provider_child_child", TRUE);
    set_elem_desc(&exp_elems[2], &Provider_child_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[2].prov_desc, L"Main", L"Provider_child_child2", TRUE);
    set_elem_desc(&exp_elems[3], &Provider_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[3].prov_desc, L"Main", L"Provider_child2", TRUE);

    test_uia_element_arr(element_arr, exp_elems, 4);
    ok_method_sequence(find_seq7, "find_seq7");
    initialize_provider_tree(FALSE);

    /*
     * Same test as before, except now Provider has a runtime ID.
     */
    Provider.runtime_id[0] = Provider.runtime_id[1] = 0xdeadbeef;
    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsContentElementPropertyId, &v);
    set_provider_prop_override(&Provider, &prop_override, 1);
    set_provider_prop_override(&Provider_child2, &prop_override, 1);
    set_provider_prop_override(&Provider_child_child, &prop_override, 1);
    set_provider_prop_override(&Provider_child_child2, &prop_override, 1);

    hr = IUIAutomationElement_FindAllBuildCache(element, TreeScope_Element | TreeScope_Children, condition2, cache_req, &element_arr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_elem_desc(&exp_elems[0], &Provider, NULL, GetCurrentProcessId(), 2, 2);
    add_provider_desc(&exp_elems[0].prov_desc, L"Main", L"Provider", TRUE);
    set_elem_desc(&exp_elems[1], &Provider_child_child, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[1].prov_desc, L"Main", L"Provider_child_child", TRUE);
    set_elem_desc(&exp_elems[2], &Provider_child_child2, NULL, GetCurrentProcessId(), 2, 2);
    add_provider_desc(&exp_elems[2].prov_desc, L"Main", L"Provider_child_child2", TRUE);
    set_elem_desc(&exp_elems[3], &Provider_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[3].prov_desc, L"Main", L"Provider_child2", TRUE);

    /* element2 now represents Provider_child_child2. */
    hr = IUIAutomationElementArray_GetElement(element_arr, 2, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    test_uia_element_arr(element_arr, exp_elems, 4);
    ok_method_sequence(find_seq8, "find_seq8");
    initialize_provider_tree(FALSE);

    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsContentElementPropertyId, &v);
    set_provider_prop_override(&Provider_child_child2, &prop_override, 1);
    set_provider_prop_override(&Provider_child2, &prop_override, 1);
    set_provider_prop_override(&Provider_child2_child, &prop_override, 1);

    /*
     * Equivalent to: Maximum find depth of 1, find first is FALSE,
     * exclude root is FALSE. Starting at Provider_child_child2, find
     * will be able to traverse the tree in the same order as it would
     * if we had started at the tree root Provider, retrieving
     * Provider_child2 as a sibling and Provider_child2_child as a node
     * at depth 1.
     */
    hr = IUIAutomationElement_FindAllBuildCache(element2, TreeScope_Element | TreeScope_Children, condition2, cache_req, &element_arr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUIAutomationElement_Release(element2);

    set_elem_desc(&exp_elems[0], &Provider_child_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[0].prov_desc, L"Main", L"Provider_child_child2", TRUE);
    set_elem_desc(&exp_elems[1], &Provider_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[1].prov_desc, L"Main", L"Provider_child2", TRUE);
    set_elem_desc(&exp_elems[2], &Provider_child2_child, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[2].prov_desc, L"Main", L"Provider_child2_child", TRUE);

    test_uia_element_arr(element_arr, exp_elems, 3);
    ok_method_sequence(find_seq9, "find_seq9");
    initialize_provider_tree(FALSE);

    /*
     * Equivalent to: Maximum find depth of 1, find first is FALSE, exclude
     * root is TRUE. Exclude root applies to the first node that matches the
     * view condition, and not the node that is passed into UiaFind(). Since
     * Provider doesn't match our view condition here, Provider_child will be
     * excluded.
     */
    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsContentElementPropertyId, &v);
    set_provider_prop_override(&Provider_child, &prop_override, 1);
    set_provider_prop_override(&Provider_child2, &prop_override, 1);
    hr = IUIAutomationElement_FindAllBuildCache(element, TreeScope_Children, condition2, cache_req, &element_arr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_elem_desc(&exp_elems[0], &Provider_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[0].prov_desc, L"Main", L"Provider_child2", TRUE);

    test_uia_element_arr(element_arr, exp_elems, 1);
    ok_method_sequence(find_seq10, "find_seq10");
    initialize_provider_tree(FALSE);

    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsContentElementPropertyId, &v);
    set_provider_prop_override(&Provider_child_child2, &prop_override, 1);

    /*
     * Equivalent to: Maximum find depth of -1, find first is TRUE, exclude
     * root is FALSE. Provider_child_child2 is the only element in the tree
     * to match our condition.
     */
    hr = IUIAutomationElement_FindFirstBuildCache(element, TreeScope_Subtree, condition, cache_req, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child_child2.ref);

    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element2, UIA_ProviderDescriptionPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child_child2", TRUE);
    VariantClear(&v);

    IUIAutomationElement_Release(element2);
    ok_method_sequence(find_seq11, "find_seq11");
    initialize_provider_tree(FALSE);

    IUIAutomationCondition_Release(condition);
    IUIAutomationCondition_Release(condition2);
    IUIAutomationCacheRequest_Release(cache_req);

    /*
     * Equivalent to: Maximum find depth of 1, find first is FALSE,
     * exclude root is FALSE. FindAll() uses the default
     * IUIAutomationCacheRequest, which uses the ControlView condition
     * as its view condition. Since Provider_child doesn't match this view
     * condition, it isn't a child of Provider in this treeview. No property
     * values are cached by the default internal cache request, so we need to
     * use a separate method sequence.
     */
    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsControlElementPropertyId, &v);
    set_provider_prop_override(&Provider_child, &prop_override, 1);

    /* condition is now UIA_IsContentElementPropertyId == TRUE. */
    variant_init_bool(&v, TRUE);
    hr = IUIAutomation_CreatePropertyCondition(uia_iface, UIA_IsContentElementPropertyId, v, &condition);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!condition, "cond == NULL\n");

    hr = IUIAutomationElement_FindAll(element, TreeScope_Element | TreeScope_Children, condition, &element_arr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_elem_desc(&exp_elems[0], &Provider, NULL, GetCurrentProcessId(), 2, 2);
    add_provider_desc(&exp_elems[0].prov_desc, L"Main", L"Provider", TRUE);
    set_elem_desc(&exp_elems[1], &Provider_child_child, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[1].prov_desc, L"Main", L"Provider_child_child", TRUE);
    set_elem_desc(&exp_elems[2], &Provider_child_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[2].prov_desc, L"Main", L"Provider_child_child2", TRUE);
    set_elem_desc(&exp_elems[3], &Provider_child2, NULL, GetCurrentProcessId(), 2, 1);
    add_provider_desc(&exp_elems[3].prov_desc, L"Main", L"Provider_child2", TRUE);

    test_uia_element_arr(element_arr, exp_elems, 4);
    ok_method_sequence(element_find_seq1, "element_find_seq1");
    initialize_provider_tree(FALSE);

    /*
     * Equivalent to: Maximum find depth of -1, find first is TRUE,
     * exclude root is FALSE. FindFirst() also uses the default cache request.
     * Provider_child_child2 will be the first provider in the tree to match
     * this view condition, so it will be returned.
     */
    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsControlElementPropertyId, &v);
    set_provider_prop_override(&Provider, &prop_override, 1);
    set_provider_prop_override(&Provider_child, &prop_override, 1);
    set_provider_prop_override(&Provider_child_child, &prop_override, 1);

    hr = IUIAutomationElement_FindFirst(element, TreeScope_Subtree, condition, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child_child2.ref);

    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element2, UIA_ProviderDescriptionPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_child_child2", TRUE);
    VariantClear(&v);

    IUIAutomationElement_Release(element2);
    ok_method_sequence(element_find_seq2, "element_find_seq2");
    initialize_provider_tree(TRUE);

    IUIAutomationCondition_Release(condition);
    IUIAutomationElement_Release(element);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    DestroyWindow(hwnd);
    UnregisterClassA("test_Element_Find class", NULL);
}

static const struct prov_method_sequence treewalker_seq1[] = {
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { 0 }
};

static const struct prov_method_sequence treewalker_seq2[] = {
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_FirstChild */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { 0 }
};

static const struct prov_method_sequence treewalker_seq3[] = {
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_LastChild */
    NODE_CREATE_SEQ(&Provider_child2),
    { 0 }
};

static const struct prov_method_sequence treewalker_seq4[] = {
    { &Provider, FRAG_NAVIGATE }, /* NavigateDirection_LastChild */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, FRAG_GET_RUNTIME_ID },
    { 0 }
};

static const struct prov_method_sequence treewalker_seq5[] = {
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { 0 }
};

static const struct prov_method_sequence treewalker_seq6[] = {
    { &Provider_child, FRAG_NAVIGATE }, /* NavigateDirection_NextSibling */
    NODE_CREATE_SEQ(&Provider_child2),
    { &Provider_child2, FRAG_GET_RUNTIME_ID },
    { 0 }
};

static const struct prov_method_sequence treewalker_seq7[] = {
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_PreviousSibling */
    NODE_CREATE_SEQ(&Provider_child),
    { 0 }
};

static const struct prov_method_sequence treewalker_seq8[] = {
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_PreviousSibling */
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { 0 }
};

static const struct prov_method_sequence treewalker_seq9[] = {
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { 0 }
};

static const struct prov_method_sequence treewalker_seq10[] = {
    { &Provider_child2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    NODE_CREATE_SEQ(&Provider),
    { &Provider, FRAG_GET_RUNTIME_ID },
    { 0 }
};

static void test_CUIAutomation_TreeWalker_ifaces(IUIAutomation *uia_iface)
{
    HWND hwnd = create_test_hwnd("test_CUIAutomation_TreeWalker_ifaces class");
    IUIAutomationElement *element, *element2, *element3;
    IUIAutomationCacheRequest *cache_req;
    IUIAutomationCondition *cond, *cond2;
    IUIAutomationTreeWalker *walker;
    HRESULT hr;

    cond = NULL;
    hr = IUIAutomation_CreateTrueCondition(uia_iface, &cond);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond, "cond == NULL\n");

    /* NULL input argument tests. */
    walker = (void *)0xdeadbeef;
    hr = IUIAutomation_CreateTreeWalker(uia_iface, NULL, &walker);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(!walker, "walker != NULL\n");

    hr = IUIAutomation_CreateTreeWalker(uia_iface, cond, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* Actually create TreeWalker. */
    walker = NULL;
    hr = IUIAutomation_CreateTreeWalker(uia_iface, cond, &walker);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!walker, "walker == NULL\n");

    hr = IUIAutomationTreeWalker_get_Condition(walker, &cond2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cond2, "cond2 == NULL\n");

    ok(iface_cmp((IUnknown *)cond, (IUnknown *)cond2), "cond != cond2\n");
    IUIAutomationCondition_Release(cond);
    IUIAutomationCondition_Release(cond2);

    cache_req = NULL;
    hr = IUIAutomation_CreateCacheRequest(uia_iface, &cache_req);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cache_req, "cache_req == NULL\n");

    element = create_test_element_from_hwnd(uia_iface, hwnd, TRUE);
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    initialize_provider(&Provider_child2, ProviderOptions_ServerSideProvider, NULL, TRUE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    provider_add_child(&Provider, &Provider_child);
    provider_add_child(&Provider, &Provider_child2);

    /* NULL input argument tests. */
    hr = IUIAutomationTreeWalker_GetFirstChildElement(walker, element, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    element2 = (void *)0xdeadbeef;
    hr = IUIAutomationTreeWalker_GetFirstChildElement(walker, NULL, &element2);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(!element2, "element2 != NULL\n");

    /* NavigateDirection_FirstChild. */
    element2 = NULL;
    hr = IUIAutomationTreeWalker_GetFirstChildElement(walker, element, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(!!element2, "element2 == NULL\n");
    ok_method_sequence(treewalker_seq1, "treewalker_seq1");

    IUIAutomationElement_Release(element2);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    element2 = NULL;
    hr = IUIAutomationTreeWalker_GetFirstChildElementBuildCache(walker, element, cache_req, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(!!element2, "element2 == NULL\n");
    ok_method_sequence(treewalker_seq2, "treewalker_seq2");

    /* NavigateDirection_NextSibling. */
    element3 = NULL;
    hr = IUIAutomationTreeWalker_GetNextSiblingElement(walker, element2, &element3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);
    ok(!!element3, "element3 == NULL\n");
    ok_method_sequence(treewalker_seq5, "treewalker_seq5");
    IUIAutomationElement_Release(element3);
    ok(Provider_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_child2.ref);

    element3 = NULL;
    hr = IUIAutomationTreeWalker_GetNextSiblingElementBuildCache(walker, element2, cache_req, &element3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);
    ok(!!element3, "element3 == NULL\n");
    ok_method_sequence(treewalker_seq6, "treewalker_seq6");
    IUIAutomationElement_Release(element3);
    ok(Provider_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_child2.ref);

    IUIAutomationElement_Release(element2);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    /* NavigateDirection_LastChild. */
    element2 = NULL;
    hr = IUIAutomationTreeWalker_GetLastChildElement(walker, element, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);
    ok(!!element2, "element2 == NULL\n");
    ok_method_sequence(treewalker_seq3, "treewalker_seq3");

    IUIAutomationElement_Release(element2);
    ok(Provider_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_child2.ref);

    element2 = NULL;
    hr = IUIAutomationTreeWalker_GetLastChildElementBuildCache(walker, element, cache_req, &element2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);
    ok(!!element2, "element2 == NULL\n");
    ok_method_sequence(treewalker_seq4, "treewalker_seq4");

    /* NavigateDirection_PreviousSibling. */
    element3 = NULL;
    hr = IUIAutomationTreeWalker_GetPreviousSiblingElement(walker, element2, &element3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(!!element3, "element3 == NULL\n");
    ok_method_sequence(treewalker_seq7, "treewalker_seq7");
    IUIAutomationElement_Release(element3);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    element3 = NULL;
    hr = IUIAutomationTreeWalker_GetPreviousSiblingElementBuildCache(walker, element2, cache_req, &element3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(!!element3, "element3 == NULL\n");
    ok_method_sequence(treewalker_seq8, "treewalker_seq8");
    IUIAutomationElement_Release(element3);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);

    /* NavigateDirection_Parent. */
    element3 = NULL;
    Provider.hwnd = NULL;
    hr = IUIAutomationTreeWalker_GetParentElement(walker, element2, &element3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 3, "Unexpected refcnt %ld\n", Provider.ref);
    ok(!!element3, "element3 == NULL\n");
    ok_method_sequence(treewalker_seq9, "treewalker_seq9");
    IUIAutomationElement_Release(element3);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    element3 = NULL;
    hr = IUIAutomationTreeWalker_GetParentElementBuildCache(walker, element2, cache_req, &element3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 3, "Unexpected refcnt %ld\n", Provider.ref);
    ok(!!element3, "element3 == NULL\n");
    ok_method_sequence(treewalker_seq10, "treewalker_seq10");
    IUIAutomationElement_Release(element3);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    IUIAutomationElement_Release(element2);
    ok(Provider_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_child2.ref);

    IUIAutomationElement_Release(element);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);

    IUIAutomationCacheRequest_Release(cache_req);
    IUIAutomationTreeWalker_Release(walker);

    DestroyWindow(hwnd);
    UnregisterClassA("test_CUIAutomation_TreeWalker_ifaces class", NULL);
}

static void set_clientside_providers_for_hwnd(struct Provider *proxy_prov, struct Provider *nc_prov,
        struct Provider *hwnd_prov, HWND hwnd)
{
    if (proxy_prov)
    {
        initialize_provider(proxy_prov, ProviderOptions_ClientSideProvider, hwnd, TRUE);
        proxy_prov->frag_root = &proxy_prov->IRawElementProviderFragmentRoot_iface;
        proxy_prov->ignore_hwnd_prop = TRUE;
    }

    initialize_provider(hwnd_prov, ProviderOptions_ClientSideProvider, hwnd, TRUE);
    initialize_provider(nc_prov, ProviderOptions_NonClientAreaProvider | ProviderOptions_ClientSideProvider, hwnd, TRUE);
    hwnd_prov->frag_root = &hwnd_prov->IRawElementProviderFragmentRoot_iface;
    nc_prov->frag_root = &nc_prov->IRawElementProviderFragmentRoot_iface;
    nc_prov->ignore_hwnd_prop = TRUE;
}

static void test_GetRootElement(IUIAutomation *uia_iface)
{
    IUIAutomationElement *element;
    HRESULT hr;
    VARIANT v;

    hr = IUIAutomation_GetRootElement(uia_iface, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    UiaRegisterProviderCallback(test_uia_provider_callback);

    set_clientside_providers_for_hwnd(&Provider_proxy, &Provider_nc, &Provider_hwnd, GetDesktopWindow());
    base_hwnd_prov = &Provider_hwnd.IRawElementProviderSimple_iface;
    proxy_prov = &Provider_proxy.IRawElementProviderSimple_iface;
    nc_prov = &Provider_nc.IRawElementProviderSimple_iface;

    /* Retrieve an element representing the desktop HWND. */
    method_sequences_enabled = FALSE;
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_proxy);
    hr = IUIAutomation_GetRootElement(uia_iface, &element);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!element, "Node == NULL.\n");
    ok(Provider_proxy.ref == 2, "Unexpected refcnt %ld\n", Provider_proxy.ref);
    ok(Provider_hwnd.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    ok(Provider_nc.ref == 2, "Unexpected refcnt %ld\n", Provider_nc.ref);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    CHECK_CALLED(prov_callback_proxy);

    hr = IUIAutomationElement_GetCurrentPropertyValueEx(element, UIA_ProviderDescriptionPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), GetDesktopWindow());
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_proxy", TRUE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider_hwnd", FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Nonclient", L"Provider_nc", FALSE);
    VariantClear(&v);

    IUIAutomationElement_Release(element);
    ok(Provider_proxy.ref == 1, "Unexpected refcnt %ld\n", Provider_proxy.ref);
    ok(Provider_hwnd.ref == 1, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    ok(Provider_nc.ref == 1, "Unexpected refcnt %ld\n", Provider_nc.ref);

    initialize_provider(&Provider_hwnd, ProviderOptions_ClientSideProvider, NULL, TRUE);
    initialize_provider(&Provider_nc, ProviderOptions_ClientSideProvider | ProviderOptions_NonClientAreaProvider, NULL,
            TRUE);
    initialize_provider(&Provider_proxy, ProviderOptions_ClientSideProvider, NULL, TRUE);
    base_hwnd_prov = proxy_prov = nc_prov = NULL;

    method_sequences_enabled = TRUE;
    UiaRegisterProviderCallback(NULL);
}

#define test_get_focused_elem( uia_iface, cache_req, exp_hr, exp_node_desc, proxy_cback_count, base_hwnd_cback_count, \
                              nc_cback_count, win_get_obj_count, child_win_get_obj_count, proxy_cback_todo, \
                              base_hwnd_cback_todo, nc_cback_todo, win_get_obj_todo, child_win_get_obj_todo ) \
        test_get_focused_elem_( (uia_iface), (cache_req), (exp_hr), (exp_node_desc), (proxy_cback_count), (base_hwnd_cback_count), \
                               (nc_cback_count), (win_get_obj_count), (child_win_get_obj_count), (proxy_cback_todo), \
                               (base_hwnd_cback_todo), (nc_cback_todo), (win_get_obj_todo), (child_win_get_obj_todo), __FILE__, __LINE__)
static void test_get_focused_elem_(IUIAutomation *uia_iface, IUIAutomationCacheRequest *cache_req, HRESULT exp_hr,
        struct node_provider_desc *exp_node_desc, int proxy_cback_count, int base_hwnd_cback_count, int nc_cback_count,
        int win_get_obj_count, int child_win_get_obj_count, BOOL proxy_cback_todo, BOOL base_hwnd_cback_todo,
        BOOL nc_cback_todo, BOOL win_get_obj_todo, BOOL child_win_get_obj_todo, const char *file, int line)
{
    IUIAutomationElement *element = NULL;
    HRESULT hr;
    VARIANT v;

    SET_EXPECT_MULTI(prov_callback_base_hwnd, base_hwnd_cback_count);
    SET_EXPECT_MULTI(prov_callback_nonclient, nc_cback_count);
    SET_EXPECT_MULTI(prov_callback_proxy, proxy_cback_count);
    SET_EXPECT_MULTI(winproc_GETOBJECT_UiaRoot, win_get_obj_count);
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, child_win_get_obj_count);
    if (cache_req)
        hr = IUIAutomation_GetFocusedElementBuildCache(uia_iface, cache_req, &element);
    else
        hr = IUIAutomation_GetFocusedElement(uia_iface, &element);
    ok_(file, line)(hr == exp_hr, "Unexpected hr %#lx.\n", hr);
    todo_wine_if(base_hwnd_cback_todo) CHECK_CALLED_MULTI(prov_callback_base_hwnd, base_hwnd_cback_count);
    todo_wine_if(proxy_cback_todo) CHECK_CALLED_MULTI(prov_callback_proxy, proxy_cback_count);
    todo_wine_if(nc_cback_todo) CHECK_CALLED_MULTI(prov_callback_nonclient, nc_cback_count);
    todo_wine_if(win_get_obj_todo) CHECK_CALLED_MULTI(winproc_GETOBJECT_UiaRoot, win_get_obj_count);
    todo_wine_if(child_win_get_obj_todo) CHECK_CALLED_MULTI(child_winproc_GETOBJECT_UiaRoot, child_win_get_obj_count);

    if (exp_node_desc->prov_count)
    {
        ok_(file, line)(!!element, "element == NULL\n");

        hr = IUIAutomationElement_GetCurrentPropertyValueEx(element, UIA_ProviderDescriptionPropertyId, TRUE, &v);
        ok_(file, line)(hr == S_OK, "Unexpected hr %#lx\n", hr);
        test_node_provider_desc_(exp_node_desc, V_BSTR(&v), file, line);
        VariantClear(&v);

        IUIAutomationElement_Release(element);
    }
    else
        ok_(file, line)(!element, "element != NULL\n");
}

static void test_GetFocusedElement(IUIAutomation *uia_iface)
{
    struct Provider_prop_override prop_override;
    struct node_provider_desc exp_node_desc;
    IUIAutomationCacheRequest *cache_req;
    IUIAutomationElement *element;
    HWND hwnd, hwnd_child;
    HRESULT hr;
    VARIANT v;

    hwnd = create_test_hwnd("test_GetFocusedElement class");
    hwnd_child = create_child_test_hwnd("test_GetFocusedElement child class", hwnd);
    UiaRegisterProviderCallback(test_uia_provider_callback);

    cache_req = NULL;
    hr = IUIAutomation_CreateCacheRequest(uia_iface, &cache_req);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!cache_req, "cache_req == NULL\n");

    /*
     * Set clientside providers for our test windows and the desktop. Same
     * tests as UiaNodeFromFocus, just with COM methods.
     */
    set_clientside_providers_for_hwnd(&Provider_proxy, &Provider_nc, &Provider_hwnd, GetDesktopWindow());
    base_hwnd_prov = &Provider_hwnd.IRawElementProviderSimple_iface;
    nc_prov = &Provider_nc.IRawElementProviderSimple_iface;
    proxy_prov = &Provider_proxy.IRawElementProviderSimple_iface;

    set_clientside_providers_for_hwnd(NULL, &Provider_nc2, &Provider_hwnd2, hwnd);
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, hwnd, TRUE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    prov_root = &Provider.IRawElementProviderSimple_iface;
    Provider.ignore_hwnd_prop = TRUE;

    set_clientside_providers_for_hwnd(NULL, &Provider_nc3, &Provider_hwnd3, hwnd_child);
    initialize_provider(&Provider2, ProviderOptions_ServerSideProvider, hwnd_child, TRUE);
    Provider2.frag_root = &Provider2.IRawElementProviderFragmentRoot_iface;
    child_win_prov_root = &Provider2.IRawElementProviderSimple_iface;
    Provider2.ignore_hwnd_prop = TRUE;

    /* NULL input argument tests. */
    hr = IUIAutomation_GetFocusedElement(uia_iface, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomation_GetFocusedElementBuildCache(uia_iface, cache_req, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    element = (void *)0xdeadbeef;
    hr = IUIAutomation_GetFocusedElementBuildCache(uia_iface, NULL, &element);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(!element, "element != NULL\n");

    /*
     * None of the providers for the desktop node return a provider from
     * IRawElementProviderFragmentRoot::GetFocus, so we just get the
     * desktop node.
     */
    method_sequences_enabled = FALSE;
    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), GetDesktopWindow());
    add_provider_desc(&exp_node_desc, L"Main", L"Provider_proxy", TRUE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc", FALSE);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd", FALSE);

    test_get_focused_elem(uia_iface, NULL, S_OK, &exp_node_desc, 1, 1, 1, 0, 0, FALSE, FALSE, FALSE, FALSE, FALSE);
    test_get_focused_elem(uia_iface, cache_req, S_OK, &exp_node_desc, 1, 1, 1, 0, 0, FALSE, FALSE, FALSE, FALSE, FALSE);

    /* Provider_hwnd returns Provider_hwnd2 from GetFocus. */
    Provider_hwnd.focus_prov = &Provider_hwnd2.IRawElementProviderFragment_iface;

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), hwnd);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider", TRUE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc2", FALSE);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd2", FALSE);

    test_get_focused_elem(uia_iface, NULL, S_OK, &exp_node_desc, 2, 1, 2, 1, 0, TRUE, FALSE, FALSE, FALSE, FALSE);
    test_get_focused_elem(uia_iface, cache_req, S_OK, &exp_node_desc, 2, 1, 2, 1, 0, TRUE, FALSE, FALSE, FALSE, FALSE);

    /*
     * Provider_proxy returns Provider from GetFocus. The provider that
     * creates the node will not have GetFocus called on it to avoid returning
     * the same provider twice. Similarly, on nodes other than the desktop
     * node, the HWND provider will not have GetFocus called on it.
     */
    Provider_hwnd.focus_prov = NULL;
    Provider_proxy.focus_prov = &Provider.IRawElementProviderFragment_iface;
    Provider.focus_prov = Provider_hwnd2.focus_prov = &Provider2.IRawElementProviderFragment_iface;

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), hwnd);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider", TRUE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc2", FALSE);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd2", FALSE);

    test_get_focused_elem(uia_iface, NULL, S_OK, &exp_node_desc, 2, 2, 2, 1, 0, TRUE, FALSE, FALSE, FALSE, FALSE);
    test_get_focused_elem(uia_iface, cache_req, S_OK, &exp_node_desc, 2, 2, 2, 1, 0, TRUE, FALSE, FALSE, FALSE, FALSE);

    /*
     * Provider_nc returns Provider_nc2 from GetFocus, Provider returns
     * Provider2, Provider_nc3 returns Provider_child.
     */
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    Provider_proxy.focus_prov = Provider_hwnd.focus_prov = NULL;
    Provider_nc.focus_prov = &Provider_nc2.IRawElementProviderFragment_iface;
    Provider.focus_prov = &Provider2.IRawElementProviderFragment_iface;
    Provider_nc3.focus_prov = &Provider_child.IRawElementProviderFragment_iface;

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider_child", TRUE);

    test_get_focused_elem(uia_iface, NULL, S_OK,  &exp_node_desc, 2, 3, 2, 2, 1, TRUE, FALSE, FALSE, TRUE, FALSE);
    test_get_focused_elem(uia_iface, cache_req, S_OK,  &exp_node_desc, 2, 3, 2, 2, 1, TRUE, FALSE, FALSE, TRUE, FALSE);

    /*
     * Provider_proxy returns Provider_child_child from GetFocus. The focus
     * provider is normalized against the cache request view condition.
     * Provider_child_child and its ancestors don't match the cache request
     * view condition, so we'll get no provider.
     */
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    initialize_provider(&Provider_child_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    provider_add_child(&Provider, &Provider_child);
    provider_add_child(&Provider_child, &Provider_child_child);
    Provider_proxy.focus_prov = &Provider_child_child.IRawElementProviderFragment_iface;
    Provider_nc.focus_prov = Provider_hwnd.focus_prov = NULL;

    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsControlElementPropertyId, &v);
    set_provider_prop_override(&Provider_child_child, &prop_override, 1);
    set_provider_prop_override(&Provider_child, &prop_override, 1);
    set_provider_prop_override(&Provider, &prop_override, 1);

    /*
     * GetFocusedElement returns UIA_E_ELEMENTNOTAVAILABLE when no provider
     * matches our view condition, GetFocusedElementBuildCache returns E_FAIL.
     */
    init_node_provider_desc(&exp_node_desc, 0, NULL);
    test_get_focused_elem(uia_iface, NULL, UIA_E_ELEMENTNOTAVAILABLE,
            &exp_node_desc, 2, 2, 2, 1, 0, TRUE, FALSE, FALSE, FALSE, FALSE);
    test_get_focused_elem(uia_iface, cache_req, E_FAIL,
            &exp_node_desc, 2, 2, 2, 1, 0, TRUE, FALSE, FALSE, FALSE, FALSE);

    /* This time, Provider_child matches our view condition. */
    set_provider_prop_override(&Provider_child, NULL, 0);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider_child", TRUE);

    test_get_focused_elem(uia_iface, NULL, S_OK, &exp_node_desc, 1, 1, 1, 0, 0, FALSE, FALSE, FALSE, FALSE, FALSE);

    method_sequences_enabled = TRUE;
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, TRUE);
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    initialize_provider(&Provider_child_child, ProviderOptions_ServerSideProvider, NULL, TRUE);

    base_hwnd_prov = nc_prov = proxy_prov = prov_root = NULL;
    IUIAutomationCacheRequest_Release(cache_req);
    UiaRegisterProviderCallback(NULL);
    DestroyWindow(hwnd);
    UnregisterClassA("test_GetFocusedElement class", NULL);
    UnregisterClassA("test_GetFocusedElement child class", NULL);
}

static void set_uia_hwnd_expects(int proxy_cback_count, int base_hwnd_cback_count, int nc_cback_count,
        int win_get_uia_obj_count, int win_get_client_obj_count)
{
    SET_EXPECT_MULTI(prov_callback_base_hwnd, base_hwnd_cback_count);
    SET_EXPECT_MULTI(prov_callback_nonclient, nc_cback_count);
    SET_EXPECT_MULTI(prov_callback_proxy, proxy_cback_count);
    SET_EXPECT_MULTI(winproc_GETOBJECT_UiaRoot, win_get_uia_obj_count);
    SET_EXPECT_MULTI(winproc_GETOBJECT_CLIENT, win_get_client_obj_count);
}

static void check_uia_hwnd_expects(int proxy_cback_count, BOOL proxy_cback_todo,
        int base_hwnd_cback_count, BOOL base_hwnd_cback_todo, int nc_cback_count, BOOL nc_cback_todo,
        int win_get_uia_obj_count, BOOL win_get_uia_obj_todo, int win_get_client_obj_count, BOOL win_get_client_obj_todo)
{
    todo_wine_if(proxy_cback_todo) CHECK_CALLED_MULTI(prov_callback_proxy, proxy_cback_count);
    todo_wine_if(base_hwnd_cback_todo) CHECK_CALLED_MULTI(prov_callback_base_hwnd, base_hwnd_cback_count);
    todo_wine_if(nc_cback_todo) CHECK_CALLED_MULTI(prov_callback_nonclient, nc_cback_count);
    todo_wine_if(win_get_uia_obj_todo) CHECK_CALLED_MULTI(winproc_GETOBJECT_UiaRoot, win_get_uia_obj_count);
    if (win_get_client_obj_count)
        todo_wine_if(win_get_client_obj_todo) CHECK_CALLED_MULTI(winproc_GETOBJECT_CLIENT, win_get_client_obj_count);
}

static void check_uia_hwnd_expects_at_most(int proxy_cback_count, int base_hwnd_cback_count, int nc_cback_count,
        int win_get_uia_obj_count, int win_get_client_obj_count)
{
    CHECK_CALLED_AT_MOST(prov_callback_proxy, proxy_cback_count);
    CHECK_CALLED_AT_MOST(prov_callback_base_hwnd, base_hwnd_cback_count);
    CHECK_CALLED_AT_MOST(prov_callback_nonclient, nc_cback_count);
    CHECK_CALLED_AT_MOST(winproc_GETOBJECT_UiaRoot, win_get_uia_obj_count);
    CHECK_CALLED_AT_MOST(winproc_GETOBJECT_CLIENT, win_get_client_obj_count);
}

static void check_uia_hwnd_expects_at_least(int proxy_cback_count, BOOL proxy_cback_todo,
        int base_hwnd_cback_count, BOOL base_hwnd_cback_todo, int nc_cback_count, BOOL nc_cback_todo,
        int win_get_uia_obj_count, BOOL win_get_uia_obj_todo, int win_get_client_obj_count, BOOL win_get_client_obj_todo)
{
    todo_wine_if(proxy_cback_todo) CHECK_CALLED_AT_LEAST(prov_callback_proxy, proxy_cback_count);
    todo_wine_if(base_hwnd_cback_todo) CHECK_CALLED_AT_LEAST(prov_callback_base_hwnd, base_hwnd_cback_count);
    todo_wine_if(nc_cback_todo) CHECK_CALLED_AT_LEAST(prov_callback_nonclient, nc_cback_count);
    todo_wine_if(win_get_uia_obj_todo) CHECK_CALLED_AT_LEAST(winproc_GETOBJECT_UiaRoot, win_get_uia_obj_count);
    todo_wine_if(win_get_client_obj_todo) CHECK_CALLED_AT_LEAST(winproc_GETOBJECT_CLIENT, win_get_client_obj_count);
}

#define MAX_EVENT_QUEUE_COUNT 4
struct ExpectedEventQueue {
    struct node_provider_desc exp_node_desc[MAX_EVENT_QUEUE_COUNT];
    struct node_provider_desc exp_nested_node_desc[MAX_EVENT_QUEUE_COUNT];
    int exp_event_count;
    int exp_event_pos;
};

static void push_event_queue_event(struct ExpectedEventQueue *queue, struct node_provider_desc *exp_node_desc)
{
    const int idx = queue->exp_event_count;

    assert(idx < MAX_EVENT_QUEUE_COUNT);

    if (exp_node_desc)
    {
        int i;

        queue->exp_node_desc[idx] = *exp_node_desc;
        for (i = 0; i < exp_node_desc->prov_count; i++)
        {
            if (exp_node_desc->nested_desc[i])
            {
                queue->exp_nested_node_desc[idx] = *exp_node_desc->nested_desc[i];
                queue->exp_node_desc[idx].nested_desc[i] = &queue->exp_nested_node_desc[idx];
                break;
            }
        }
    }
    else
        memset(&queue->exp_node_desc[idx], 0, sizeof(queue->exp_node_desc[idx]));
    queue->exp_event_count++;
}

static struct node_provider_desc *pop_event_queue_event(struct ExpectedEventQueue *queue)
{
    if (!queue->exp_event_count || queue->exp_event_pos >= queue->exp_event_count)
    {
        ok(0, "Failed to pop expected event from queue\n");
        return NULL;
    }

    return &queue->exp_node_desc[queue->exp_event_pos++];
}

static struct ComEventData {
    struct ExpectedEventQueue exp_events;

    HWND event_hwnd;
    DWORD last_call_tid;
    HANDLE event_handle;
} ComEventData;

static void set_com_event_data(struct node_provider_desc *exp_node_desc)
{
    memset(&ComEventData.exp_events, 0, sizeof(ComEventData.exp_events));
    push_event_queue_event(&ComEventData.exp_events, exp_node_desc);

    ComEventData.last_call_tid = 0;
    SET_EXPECT(uia_com_event_callback);
}

static void push_expected_com_event(struct node_provider_desc *exp_node_desc)
{
    push_event_queue_event(&ComEventData.exp_events, exp_node_desc);
    SET_EXPECT_MULTI(uia_com_event_callback, ComEventData.exp_events.exp_event_count);
}

#define test_com_event_data( sender ) \
        test_com_event_data_( (sender), __FILE__, __LINE__)
static void test_com_event_data_(IUIAutomationElement *sender, const char *file, int line)
{
    struct node_provider_desc *exp_desc = pop_event_queue_event(&ComEventData.exp_events);
    HRESULT hr;
    VARIANT v;

    CHECK_EXPECT(uia_com_event_callback);

    VariantInit(&v);
    hr = IUIAutomationElement_GetCurrentPropertyValueEx(sender, UIA_ProviderDescriptionPropertyId, TRUE, &v);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx\n", hr);
    test_node_provider_desc_(exp_desc, V_BSTR(&v), file, line);
    VariantClear(&v);

    ComEventData.last_call_tid = GetCurrentThreadId();
    if (ComEventData.event_handle && (ComEventData.exp_events.exp_event_count == ComEventData.exp_events.exp_event_pos))
        SetEvent(ComEventData.event_handle);
}

/*
 * IUIAutomationEventHandler.
 */
static struct AutomationEventHandler
{
    IUIAutomationEventHandler IUIAutomationEventHandler_iface;
    LONG ref;
} AutomationEventHandler;

static inline struct AutomationEventHandler *impl_from_AutomationEventHandler(IUIAutomationEventHandler *iface)
{
    return CONTAINING_RECORD(iface, struct AutomationEventHandler, IUIAutomationEventHandler_iface);
}

static HRESULT WINAPI AutomationEventHandler_QueryInterface(IUIAutomationEventHandler *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUIAutomationEventHandler) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUIAutomationEventHandler_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI AutomationEventHandler_AddRef(IUIAutomationEventHandler* iface)
{
    struct AutomationEventHandler *handler = impl_from_AutomationEventHandler(iface);
    return InterlockedIncrement(&handler->ref);
}

static ULONG WINAPI AutomationEventHandler_Release(IUIAutomationEventHandler* iface)
{
    struct AutomationEventHandler *handler = impl_from_AutomationEventHandler(iface);
    return InterlockedDecrement(&handler->ref);
}

static HRESULT WINAPI AutomationEventHandler_HandleAutomationEvent(IUIAutomationEventHandler *iface,
        IUIAutomationElement *sender, EVENTID event_id)
{
    test_com_event_data(sender);

    return S_OK;
}

static const IUIAutomationEventHandlerVtbl AutomationEventHandlerVtbl = {
    AutomationEventHandler_QueryInterface,
    AutomationEventHandler_AddRef,
    AutomationEventHandler_Release,
    AutomationEventHandler_HandleAutomationEvent,
};

static struct AutomationEventHandler AutomationEventHandler =
{
    { &AutomationEventHandlerVtbl },
    1,
};

/*
 * IUIAutomationFocusChangedEventHandler.
 */
static struct FocusChangedHandler
{
    IUIAutomationFocusChangedEventHandler IUIAutomationFocusChangedEventHandler_iface;
    LONG ref;

    BOOL event_handler_added;
} FocusChangedHandler;

static inline struct FocusChangedHandler *impl_from_FocusChangedHandler(IUIAutomationFocusChangedEventHandler *iface)
{
    return CONTAINING_RECORD(iface, struct FocusChangedHandler, IUIAutomationFocusChangedEventHandler_iface);
}

static HRESULT WINAPI FocusChangedHandler_QueryInterface(IUIAutomationFocusChangedEventHandler *iface,
        REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUIAutomationFocusChangedEventHandler) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUIAutomationFocusChangedEventHandler_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI FocusChangedHandler_AddRef(IUIAutomationFocusChangedEventHandler* iface)
{
    struct FocusChangedHandler *handler = impl_from_FocusChangedHandler(iface);
    return InterlockedIncrement(&handler->ref);
}

static ULONG WINAPI FocusChangedHandler_Release(IUIAutomationFocusChangedEventHandler* iface)
{
    struct FocusChangedHandler *handler = impl_from_FocusChangedHandler(iface);
    return InterlockedDecrement(&handler->ref);
}

static HRESULT WINAPI FocusChangedHandler_HandleFocusChangedEvent(IUIAutomationFocusChangedEventHandler *iface,
        IUIAutomationElement *sender)
{
    struct FocusChangedHandler *handler = impl_from_FocusChangedHandler(iface);

    if (handler->event_handler_added)
        test_com_event_data(sender);

    return S_OK;
}

static const IUIAutomationFocusChangedEventHandlerVtbl FocusChangedHandlerVtbl = {
    FocusChangedHandler_QueryInterface,
    FocusChangedHandler_AddRef,
    FocusChangedHandler_Release,
    FocusChangedHandler_HandleFocusChangedEvent,
};

static struct FocusChangedHandler FocusChangedHandler =
{
    { &FocusChangedHandlerVtbl },
    1,
};

static DWORD WINAPI uia_com_event_handler_test_thread(LPVOID param)
{
    struct node_provider_desc exp_node_desc;
    HRESULT hr;

    /*
     * Raise an event from inside of an MTA - the event handler proxy will be
     * called from the current thread because it was registered in an MTA.
     */
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider2", TRUE);
    set_com_event_data(&exp_node_desc);
    hr = UiaRaiseAutomationEvent(&Provider2.IRawElementProviderSimple_iface, UIA_LiveRegionChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(uia_com_event_callback);
    ok(ComEventData.last_call_tid == GetCurrentThreadId(), "Event handler called on unexpected thread %ld\n",
            ComEventData.last_call_tid);
    CoUninitialize();

    /*
     * Raise an event from inside of an STA - an event handler proxy will be
     * created, and our handler will be invoked from another thread.
     */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    set_com_event_data(&exp_node_desc);
    hr = UiaRaiseAutomationEvent(&Provider2.IRawElementProviderSimple_iface, UIA_LiveRegionChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(uia_com_event_callback);
    ok(ComEventData.last_call_tid != GetCurrentThreadId(), "Event handler called on unexpected thread %ld\n",
            ComEventData.last_call_tid);
    CoUninitialize();

    return 0;
}

static void test_IUIAutomationEventHandler(IUIAutomation *uia_iface, IUIAutomationElement *elem)
{
    struct Provider_prop_override prop_override;
    struct node_provider_desc exp_node_desc;
    IUIAutomationElement *elem2;
    HANDLE thread;
    HRESULT hr;
    VARIANT v;

    /*
     * Invalid input argument tests.
     */
    hr = IUIAutomation_AddAutomationEventHandler(uia_iface, UIA_LiveRegionChangedEventId, NULL, TreeScope_Subtree, NULL,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomation_AddAutomationEventHandler(uia_iface, UIA_LiveRegionChangedEventId, elem, TreeScope_Subtree, NULL,
            NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /*
     * Passing in a NULL element to this method results in an access violation
     * on Windows.
     */
    if (0)
    {
        IUIAutomation_RemoveAutomationEventHandler(uia_iface, 1, NULL, &AutomationEventHandler.IUIAutomationEventHandler_iface);
    }

    hr = IUIAutomation_RemoveAutomationEventHandler(uia_iface, 1, elem, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomation_RemoveAutomationEventHandler(uia_iface, UIA_AutomationFocusChangedEventId, elem,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /*
     * UIA_AutomationFocusChangedEventId can only be listened for with
     * AddFocusChangedEventHandler. Trying to register it on a regular event
     * handler returns E_INVALIDARG.
     */
    hr = IUIAutomation_AddAutomationEventHandler(uia_iface, UIA_AutomationFocusChangedEventId, elem, TreeScope_Subtree, NULL,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Windows 11 queries the HWND for the element when adding a new handler. */
    set_uia_hwnd_expects(3, 2, 2, 3, 0);
    /* All other event IDs are fine, only focus events are blocked. */
    hr = IUIAutomation_AddAutomationEventHandler(uia_iface, 1, elem, TreeScope_Subtree, NULL,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(AutomationEventHandler.ref > 1, "Unexpected refcnt %ld\n", AutomationEventHandler.ref);
    check_uia_hwnd_expects_at_most(3, 2, 2, 3, 0);

    hr = IUIAutomation_RemoveAutomationEventHandler(uia_iface, 1, elem,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(AutomationEventHandler.ref == 1, "Unexpected refcnt %ld\n", AutomationEventHandler.ref);

    /*
     * Test event raising behavior.
     */
    set_uia_hwnd_expects(3, 2, 2, 3, 0);
    hr = IUIAutomation_AddAutomationEventHandler(uia_iface, UIA_LiveRegionChangedEventId, elem, TreeScope_Subtree, NULL,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(AutomationEventHandler.ref > 1, "Unexpected refcnt %ld\n", AutomationEventHandler.ref);
    check_uia_hwnd_expects_at_most(3, 2, 2, 3, 0);

    /* Same behavior as HUIAEVENTs, events are matched by runtime ID. */
    initialize_provider(&Provider2, ProviderOptions_ServerSideProvider, NULL, TRUE);
    Provider2.runtime_id[0] = 0x2a;
    Provider2.runtime_id[1] = HandleToUlong(ComEventData.event_hwnd);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider2", TRUE);
    set_com_event_data(&exp_node_desc);
    hr = UiaRaiseAutomationEvent(&Provider2.IRawElementProviderSimple_iface, UIA_LiveRegionChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(uia_com_event_callback);

    /*
     * If no cache request is provided by the user in
     * AddAutomationEventHandler, the default cache request is used. If no
     * elements match the view condition, the event handler isn't invoked.
     */
    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsControlElementPropertyId, &v);
    set_provider_prop_override(&Provider2, &prop_override, 1);
    hr = UiaRaiseAutomationEvent(&Provider2.IRawElementProviderSimple_iface, UIA_LiveRegionChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    set_provider_prop_override(&Provider2, NULL, 0);
    thread = CreateThread(NULL, 0, uia_com_event_handler_test_thread, NULL, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;

        while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);

    hr = IUIAutomation_RemoveAutomationEventHandler(uia_iface, UIA_LiveRegionChangedEventId, elem,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(AutomationEventHandler.ref == 1, "Unexpected refcnt %ld\n", AutomationEventHandler.ref);

    VariantInit(&v);
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    hr = IUIAutomationElement_GetCurrentPropertyValueEx(elem, UIA_LabeledByPropertyId, TRUE, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);

    hr = IUnknown_QueryInterface(V_UNKNOWN(&v), &IID_IUIAutomationElement, (void **)&elem2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elem2, "elem2 == NULL\n");
    VariantClear(&v);

    /*
     * Register an event on an element that has no runtime ID. The only way to
     * remove an individual event handler is by matching a combination of
     * runtime-id, property ID, and event handler interface pointer. Without a
     * runtime-id, the only way to unregister the event handler is to call
     * RemoveAllEventHandlers().
     */
    hr = IUIAutomation_AddAutomationEventHandler(uia_iface, UIA_LiveRegionChangedEventId, elem2, TreeScope_Subtree, NULL,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(AutomationEventHandler.ref > 1, "Unexpected refcnt %ld\n", AutomationEventHandler.ref);

    /* No removal will occur due to a lack of a runtime ID to match. */
    hr = IUIAutomation_RemoveAutomationEventHandler(uia_iface, UIA_LiveRegionChangedEventId, elem2,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(AutomationEventHandler.ref > 1, "Unexpected refcnt %ld\n", AutomationEventHandler.ref);

    hr = IUIAutomation_RemoveAllEventHandlers(uia_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(AutomationEventHandler.ref == 1, "Unexpected refcnt %ld\n", AutomationEventHandler.ref);

    IUIAutomationElement_Release(elem2);
}

static void test_IUIAutomationFocusChangedEventHandler(IUIAutomation *uia_iface)
{
    struct node_provider_desc exp_node_desc;
    IUIAutomationElement *elem;
    HRESULT hr;

    SetFocus(ComEventData.event_hwnd);

    /*
     * FocusChangedEventHandlers are always registered on the desktop node
     * with a scope of the entire desktop.
     *
     * All versions of Windows query the currently focused HWND when adding a
     * new focus changed event handler, but behavior differs between versions:
     *
     * Win7-Win10v1507 queries for the focused provider and raises an event
     * while also advising of events.
     *
     * Win10v1809+ will query the focused HWND, but doesn't advise of events
     * or raise a focus event. Windows 11 will advise the provider of the
     * focused HWND of events, but not any clientside providers.
     */
    set_uia_hwnd_expects(6, 6, 6, 3, 0);
    hr = IUIAutomation_AddFocusChangedEventHandler(uia_iface, NULL,
            &FocusChangedHandler.IUIAutomationFocusChangedEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(FocusChangedHandler.ref > 1, "Unexpected refcnt %ld\n", FocusChangedHandler.ref);
    check_uia_hwnd_expects_at_most(6, 6, 6, 3, 0);
    FocusChangedHandler.event_handler_added = TRUE;

    /*
     * Focus changed event handlers are registered on the desktop with a scope
     * of all elements, so all elements match regardless of runtime ID.
     */
    initialize_provider(&Provider2, ProviderOptions_ServerSideProvider, NULL, TRUE);
    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider2", TRUE);
    set_com_event_data(&exp_node_desc);
    hr = UiaRaiseAutomationEvent(&Provider2.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(uia_com_event_callback);

    /*
     * Removing the focus changed event handler creates a desktop node -
     * presumably to use to get a runtime ID for removal.
     */
    set_uia_hwnd_expects(1, 1, 1, 0, 0);
    hr = IUIAutomation_RemoveFocusChangedEventHandler(uia_iface,
            &FocusChangedHandler.IUIAutomationFocusChangedEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(FocusChangedHandler.ref == 1, "Unexpected refcnt %ld\n", FocusChangedHandler.ref);
    FocusChangedHandler.event_handler_added = FALSE;
    check_uia_hwnd_expects(1, FALSE, 1, FALSE, 1, FALSE, 0, FALSE, 0, FALSE);

    /*
     * The focus changed event handler can also be removed by called
     * RemoveAutomationEventHandler, which isn't documented.
     */
    set_uia_hwnd_expects(6, 6, 6, 3, 0);
    hr = IUIAutomation_AddFocusChangedEventHandler(uia_iface, NULL,
            &FocusChangedHandler.IUIAutomationFocusChangedEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(FocusChangedHandler.ref > 1, "Unexpected refcnt %ld\n", FocusChangedHandler.ref);
    check_uia_hwnd_expects_at_most(6, 6, 6, 3, 0);
    FocusChangedHandler.event_handler_added = TRUE;

    set_uia_hwnd_expects(1, 1, 1, 0, 0);
    hr = IUIAutomation_GetRootElement(uia_iface, &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_hwnd_expects(1, FALSE, 1, FALSE, 1, FALSE, 0, FALSE, 0, FALSE);

    hr = IUIAutomation_RemoveAutomationEventHandler(uia_iface, UIA_AutomationFocusChangedEventId, elem,
            (IUIAutomationEventHandler *)&FocusChangedHandler.IUIAutomationFocusChangedEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(FocusChangedHandler.ref == 1, "Unexpected refcnt %ld\n", FocusChangedHandler.ref);

    IUIAutomationElement_Release(elem);
}

struct com_win_event_test_thread_data
{
    IUIAutomation *uia_iface;
    HWND test_hwnd;
    HWND test_child_hwnd;
};

static void set_method_event_handle_for_providers(struct Provider *main, struct Provider *hwnd, struct Provider *nc,
        HANDLE *handles, int method)
{
    if (!handles || method < 0)
    {
        set_provider_method_event_data(main, NULL, -1);
        set_provider_method_event_data(hwnd, NULL, -1);
        set_provider_method_event_data(nc, NULL, -1);
    }
    else
    {
        set_provider_method_event_data(main, handles[0], method);
        set_provider_method_event_data(hwnd, handles[1], method);
        set_provider_method_event_data(nc, handles[2], method);
    }
}

static void reset_event_advise_values_for_hwnd_providers(struct Provider *main, struct Provider *hwnd, struct Provider *nc)
{
    initialize_provider_advise_events_ids(main);
    initialize_provider_advise_events_ids(hwnd);
    initialize_provider_advise_events_ids(nc);
}

#define test_hwnd_providers_event_advise_added( main, hwnd, nc, event_id, todo) \
        test_hwnd_providers_event_advise_added_( (main), (hwnd), (nc), (event_id), (todo), __FILE__, __LINE__)
static void test_hwnd_providers_event_advise_added_(struct Provider *main, struct Provider *hwnd, struct Provider *nc,
        int event_id, BOOL todo, const char *file, int line)
{
    test_provider_event_advise_added_(main, event_id, todo, file, line);
    test_provider_event_advise_added_(hwnd, event_id, todo, file, line);
    test_provider_event_advise_added_(nc, event_id, todo, file, line);
}

static void test_uia_com_event_handler_event_advisement(IUIAutomation *uia_iface, HWND test_hwnd, HWND test_child_hwnd)
{
    GUITHREADINFO info = { .cbSize = sizeof(info) };
    IUIAutomationElement *elem;
    HANDLE method_event[4];
    int event_handle_count;
    DWORD wait_res;
    BOOL is_win11;
    HRESULT hr;
    int i;

    for (i = 0; i < ARRAY_SIZE(method_event); i++)
        method_event[i] = CreateEventW(NULL, FALSE, FALSE, NULL);

    /* Only sends WM_GETOBJECT twice on Win11. */
    set_uia_hwnd_expects(0, 1, 1, 2, 0);
    hr = IUIAutomation_ElementFromHandle(uia_iface, test_hwnd, &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elem, "elem == NULL\n");
    ok(Provider.ref >= 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider_hwnd2.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd2.ref);
    ok(Provider_nc2.ref == 2, "Unexpected refcnt %ld\n", Provider_nc2.ref);
    check_uia_hwnd_expects_at_least(0, FALSE, 1, FALSE, 1, FALSE, 1, FALSE, 0, FALSE);

    /*
     * The COM API has no equivalent to UiaEventAddWindow, which means all
     * event advisement has to be done by the COM API itself. It does this by
     * using EVENT_OBJECT_SHOW as a way to find HWNDs that need to be advised.
     */
    set_uia_hwnd_expects(0, 1, 1, 4, 0); /* Only done on Win11. */
    hr = IUIAutomation_AddAutomationEventHandler(uia_iface, UIA_LiveRegionChangedEventId, elem, TreeScope_Subtree, NULL,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(AutomationEventHandler.ref > 1, "Unexpected refcnt %ld\n", AutomationEventHandler.ref);
    is_win11 = !!CALLED_COUNT(prov_callback_base_hwnd);
    check_uia_hwnd_expects_at_most(0, 1, 1, 4, 0);

    set_provider_method_event_data(&Provider_hwnd2, method_event[0], ADVISE_EVENTS_EVENT_ADDED);
    set_provider_method_event_data(&Provider_nc2, method_event[1], ADVISE_EVENTS_EVENT_ADDED);
    set_provider_method_event_data(&Provider, method_event[2], ADVISE_EVENTS_EVENT_ADDED);
    event_handle_count = 3;

    /*
     * Raise EVENT_OBJECT_SHOW on a non-visible HWND. Its providers will not
     * be advised of events being listened for.
     */
    ok(!IsWindowVisible(test_hwnd), "Test HWND is visible\n");
    NotifyWinEvent(EVENT_OBJECT_SHOW, test_hwnd, OBJID_WINDOW, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 500) == WAIT_TIMEOUT, "Wait for method_event(s) didn't timeout.\n");

    /*
     * This fires off EVENT_OBJECT_SHOW, our providers will be advised of
     * events.
     */
    initialize_provider_advise_events_ids(&Provider);
    initialize_provider_advise_events_ids(&Provider_nc2);
    initialize_provider_advise_events_ids(&Provider_hwnd2);

    set_uia_hwnd_expects(0, 2, 2, 6, 0); /* Only done more than one of each on Win11. */
    ShowWindow(test_hwnd, SW_SHOW);
    wait_res = msg_wait_for_all_events(method_event, event_handle_count, 3000);
    if ((wait_res == WAIT_TIMEOUT) && (Provider_nc2.advise_events_added_event_id && Provider_hwnd2.advise_events_added_event_id) &&
            !Provider.advise_events_added_event_id)
    {
        /*
         * Windows 7 won't advise a nested node provider from the current
         * process of events being listened for.
         */
        win_skip("Windows 7 only advises clientside providers of events, skipping further tests.\n");
        hr = IUIAutomation_RemoveAutomationEventHandler(uia_iface, UIA_LiveRegionChangedEventId, elem,
                &AutomationEventHandler.IUIAutomationEventHandler_iface);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(AutomationEventHandler.ref == 1, "Unexpected refcnt %ld\n", AutomationEventHandler.ref);
        IUIAutomationElement_Release(elem);

        set_provider_method_event_data(&Provider_hwnd2, NULL, -1);
        set_provider_method_event_data(&Provider_nc2, NULL, -1);
        set_provider_method_event_data(&Provider, NULL, -1);
        goto exit;
    }
    ok(wait_res != WAIT_TIMEOUT, "Wait for method_event(s) timed out.\n");
    check_uia_hwnd_expects_at_least(0, FALSE, 1, FALSE, 1, FALSE, 1, FALSE, 0, FALSE);

    /*
     * Manually fire off EVENT_OBJECT_SHOW, providers will be advised of
     * events being added again.
     */
    set_uia_hwnd_expects(0, 2, 2, 6, 0); /* Only done more than one of each on Win11. */
    NotifyWinEvent(EVENT_OBJECT_SHOW, test_hwnd, OBJID_WINDOW, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 3000) != WAIT_TIMEOUT, "Wait for method_event(s) timed out.\n");
    check_uia_hwnd_expects_at_least(0, FALSE, 1, FALSE, 1, FALSE, 1, FALSE, 0, FALSE);

    /*
     * Providers are only advised of events being listened for if an event is
     * raised with an objid of OBJID_WINDOW.
     */
    NotifyWinEvent(EVENT_OBJECT_SHOW, test_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 500) == WAIT_TIMEOUT, "Wait for method_event(s) didn't timeout.\n");

    set_provider_method_event_data(&Provider_hwnd2, NULL, -1);
    set_provider_method_event_data(&Provider_nc2, NULL, -1);
    set_provider_method_event_data(&Provider, NULL, -1);

    /*
     * Show our child HWND. Navigation is done to confirm it is within the
     * scope of our event handler, and it is only advised if it is.
     */
    set_provider_method_event_data(&Provider_hwnd3, method_event[0], ADVISE_EVENTS_EVENT_ADDED);
    set_provider_method_event_data(&Provider_nc3, method_event[1], ADVISE_EVENTS_EVENT_ADDED);
    set_provider_method_event_data(&Provider2, method_event[2], ADVISE_EVENTS_EVENT_ADDED);

    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 6); /* Only done more than once on Win11. */
    set_uia_hwnd_expects(0, 2, 3, 5, 0); /* Only done more than one of each on Win11. */
    ShowWindow(test_child_hwnd, SW_SHOW);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 3000) != WAIT_TIMEOUT, "Wait for method_event(s) timed out.\n");
    check_uia_hwnd_expects_at_least(0, FALSE, 1, FALSE, 1, FALSE, 1, FALSE, 0, FALSE);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);

    /* Same deal as before, it will advise multiple times. */
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 6); /* Only done more than once on Win11. */
    set_uia_hwnd_expects(0, 2, 3, 5, 0); /* Only done more than one of each on Win11. */
    NotifyWinEvent(EVENT_OBJECT_SHOW, test_child_hwnd, OBJID_WINDOW, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 3000) != WAIT_TIMEOUT, "Wait for method_event(s) timed out.\n");
    check_uia_hwnd_expects_at_least(0, FALSE, 1, FALSE, 1, FALSE, 1, FALSE, 0, FALSE);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);

    /* Break navigation chain, can't reach our test element so no advisement. */
    Provider_hwnd3.parent = NULL;
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 2); /* Only done more than once on Win11. */
    set_uia_hwnd_expects(0, 1, 1, 1, 0);
    NotifyWinEvent(EVENT_OBJECT_SHOW, test_child_hwnd, OBJID_WINDOW, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 2000) == WAIT_TIMEOUT, "Wait for method_event(s) didn't timeout.\n");
    check_uia_hwnd_expects(0, FALSE, 1, FALSE, 1, FALSE, 1, TRUE, 0, FALSE);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);

    set_provider_method_event_data(&Provider_hwnd3, NULL, -1);
    set_provider_method_event_data(&Provider_nc3, NULL, -1);
    set_provider_method_event_data(&Provider2, NULL, -1);

    hr = IUIAutomation_RemoveAutomationEventHandler(uia_iface, UIA_LiveRegionChangedEventId, elem,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IUIAutomationElement_Release(elem);

    /*
     * Register event handler on desktop element with a scope of
     * TreeScope_Subtree. All EVENT_OBJECT_SHOW events will result in event
     * advisement regardless of navigation.
     */
    set_uia_hwnd_expects(0, 1, 1, 0, 0);
    hr = IUIAutomation_GetRootElement(uia_iface, &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_hwnd_expects(0, FALSE, 1, FALSE, 1, FALSE, 0, FALSE, 0, FALSE);

    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot); /* Only done on Win11. */
    set_uia_hwnd_expects(0, 2, 2, 3, 0); /* Only done on Win11. */
    hr = IUIAutomation_AddAutomationEventHandler(uia_iface, UIA_LiveRegionChangedEventId, elem, TreeScope_Subtree, NULL,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(AutomationEventHandler.ref > 1, "Unexpected refcnt %ld\n", AutomationEventHandler.ref);
    CHECK_CALLED_AT_MOST(child_winproc_GETOBJECT_UiaRoot, 1);
    check_uia_hwnd_expects_at_most(0, 2, 2, 3, 0);

    set_provider_method_event_data(&Provider_hwnd, method_event[0], ADVISE_EVENTS_EVENT_ADDED);
    set_provider_method_event_data(&Provider_nc, method_event[1], ADVISE_EVENTS_EVENT_ADDED);
    set_provider_method_event_data(&Provider_proxy, method_event[2], ADVISE_EVENTS_EVENT_ADDED);

    /*
     * Windows 11 always advises all HWNDs on the desktop, so we wait for our
     * child window provider to be advised as well.
     */
    if (is_win11)
    {
        set_provider_method_event_data(&Provider2, method_event[3], ADVISE_EVENTS_EVENT_ADDED);
        event_handle_count++;
    }

    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot); /* Only done on Win11. */
    set_uia_hwnd_expects(0, 3, 3, 1, 0); /* Only done more than once on Win11. */
    NotifyWinEvent(EVENT_OBJECT_SHOW, GetDesktopWindow(), OBJID_WINDOW, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 2000) != WAIT_TIMEOUT, "Wait for method_event(s) timed out.\n");
    CHECK_CALLED_AT_MOST(winproc_GETOBJECT_UiaRoot, 1);
    CHECK_CALLED_AT_MOST(child_winproc_GETOBJECT_UiaRoot, 1);
    check_uia_hwnd_expects_at_least(0, FALSE, 1, FALSE, 1, FALSE, 0, FALSE, 0, FALSE);

    set_provider_method_event_data(&Provider_hwnd, NULL, -1);
    set_provider_method_event_data(&Provider_nc, NULL, -1);
    set_provider_method_event_data(&Provider_proxy, NULL, -1);

    /*
     * Test window isn't connected to desktop element through navigation, but
     * still gets advised of events on a desktop HWND event.
     */
    set_provider_method_event_data(&Provider_hwnd2, method_event[0], ADVISE_EVENTS_EVENT_ADDED);
    set_provider_method_event_data(&Provider_nc2, method_event[1], ADVISE_EVENTS_EVENT_ADDED);
    set_provider_method_event_data(&Provider, method_event[2], ADVISE_EVENTS_EVENT_ADDED);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot); /* Only done on Win11. */
    set_uia_hwnd_expects(0, 2, 2, 7, 0); /* Only done more than once on Win11. */
    NotifyWinEvent(EVENT_OBJECT_SHOW, test_hwnd, OBJID_WINDOW, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 2000) != WAIT_TIMEOUT, "Wait for method_event(s) timed out.\n");
    CHECK_CALLED_AT_MOST(child_winproc_GETOBJECT_UiaRoot, 1);
    check_uia_hwnd_expects_at_most(0, 2, 2, 7, 0);

    hr = IUIAutomation_RemoveAutomationEventHandler(uia_iface, UIA_LiveRegionChangedEventId, elem,
            &AutomationEventHandler.IUIAutomationEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IUIAutomationElement_Release(elem);

    set_provider_method_event_data(&Provider_hwnd2, NULL, -1);
    set_provider_method_event_data(&Provider_nc2, NULL, -1);
    set_provider_method_event_data(&Provider, NULL, -1);
    if (is_win11)
        set_provider_method_event_data(&Provider2, NULL, -1);

    /*
     * IUIAutomationFocusChangedEventHandler is treated differently than all
     * other event handlers - it only advises providers of events the first
     * time it encounters an EVENT_OBJECT_FOCUS WinEvent for an HWND.
     */
    reset_event_advise_values_for_hwnd_providers(&Provider_proxy, &Provider_hwnd, &Provider_nc);
    reset_event_advise_values_for_hwnd_providers(&Provider, &Provider_hwnd2, &Provider_nc2);
    reset_event_advise_values_for_hwnd_providers(&Provider2, &Provider_hwnd3, &Provider_nc3);

    set_uia_hwnd_expects(0, 6, 6, 5, 0);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot); /* Only sent on Win11. */
    FocusChangedHandler.event_handler_added = FALSE;
    hr = IUIAutomation_AddFocusChangedEventHandler(uia_iface, NULL,
            &FocusChangedHandler.IUIAutomationFocusChangedEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(FocusChangedHandler.ref > 1, "Unexpected refcnt %ld\n", FocusChangedHandler.ref);
    check_uia_hwnd_expects_at_most(0, 6, 6, 5, 0);
    CHECK_CALLED_AT_MOST(child_winproc_GETOBJECT_UiaRoot, 1);

    /*
     * Windows 10 version 1507 and below only advise the currently focused
     * HWNDs providers of events being added - newer versions instead advise
     * the desktop HWND's providers of events being added. We're matching the
     * newer behavior, so skip the tests on older versions.
     */
    if (!Provider_nc.advise_events_added_event_id && !Provider_hwnd.advise_events_added_event_id &&
            !Provider_proxy.advise_events_added_event_id)
    {
        win_skip("Win10v1507 and below advise the currently focused HWND and not the desktop HWND, skipping tests.\n");

        test_hwnd_providers_event_advise_added(&Provider, &Provider_hwnd2, &Provider_nc2, UIA_AutomationFocusChangedEventId, FALSE);
        set_uia_hwnd_expects(0, 1, 1, 0, 0);
        hr = IUIAutomation_RemoveFocusChangedEventHandler(uia_iface,
                &FocusChangedHandler.IUIAutomationFocusChangedEventHandler_iface);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        check_uia_hwnd_expects(0, FALSE, 1, FALSE, 1, FALSE, 0, FALSE, 0, FALSE);
        goto exit;
    }

    /*
     * Currently focused HWND is not advised, it's expected that the desktop
     * providers will advise it using GetEmbeddedFragmentRoots.
     */
    test_provider_event_advise_added(&Provider_hwnd2, 0, FALSE);
    test_provider_event_advise_added(&Provider_nc2, 0, FALSE);
    test_hwnd_providers_event_advise_added(&Provider_proxy, &Provider_hwnd, &Provider_nc, UIA_AutomationFocusChangedEventId, FALSE);

    /*
     * EVENT_OBJECT_SHOW doesn't advise events on anything other than the
     * desktop HWND for focus changed event handlers.
     */
    event_handle_count = 3;
    set_uia_hwnd_expects(0, 1, 1, 2, 0); /* Win11 sends WM_GETOBJECT twice. */
    set_method_event_handle_for_providers(&Provider, &Provider_hwnd2, &Provider_nc2, method_event, ADVISE_EVENTS_EVENT_ADDED);
    NotifyWinEvent(EVENT_OBJECT_SHOW, test_hwnd, OBJID_WINDOW, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 750) == WAIT_TIMEOUT, "Wait for method_event(s) didn't time out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");
    check_uia_hwnd_expects_at_most(0, 1, 1, 2, 0);
    set_method_event_handle_for_providers(&Provider, &Provider_hwnd2, &Provider_nc2, NULL, -1);

    set_method_event_handle_for_providers(&Provider2, &Provider_hwnd3, &Provider_nc3, method_event, ADVISE_EVENTS_EVENT_ADDED);
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 2); /* Only done twice on Win11. */
    set_uia_hwnd_expects(0, 1, 1, 1, 0);
    NotifyWinEvent(EVENT_OBJECT_SHOW, test_child_hwnd, OBJID_WINDOW, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 750) == WAIT_TIMEOUT, "Wait for method_event(s) didn't time out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");
    set_method_event_handle_for_providers(&Provider2, &Provider_hwnd3, &Provider_nc3, NULL, -1);
    check_uia_hwnd_expects(0, FALSE, 1, FALSE, 1, FALSE, 1, TRUE, 0, FALSE);
    CHECK_CALLED_AT_MOST(child_winproc_GETOBJECT_UiaRoot, 2);

    /*
     * This will advise events.
     */
    set_method_event_handle_for_providers(&Provider_proxy, &Provider_hwnd, &Provider_nc, method_event, ADVISE_EVENTS_EVENT_ADDED);
    reset_event_advise_values_for_hwnd_providers(&Provider_proxy, &Provider_hwnd, &Provider_nc);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot); /* Only sent on Win11. */
    set_uia_hwnd_expects(0, 3, 3, 1, 0); /* Only Win11 sends WM_GETOBJECT. */

    NotifyWinEvent(EVENT_OBJECT_SHOW, GetDesktopWindow(), OBJID_WINDOW, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 2000) != WAIT_TIMEOUT, "Wait for method_event(s) timed out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");

    test_hwnd_providers_event_advise_added(&Provider_proxy, &Provider_hwnd, &Provider_nc, UIA_AutomationFocusChangedEventId, FALSE);
    set_method_event_handle_for_providers(&Provider_proxy, &Provider_hwnd, &Provider_nc, NULL, -1);
    check_uia_hwnd_expects_at_most(0, 3, 3, 1, 0);
    CHECK_CALLED_AT_MOST(child_winproc_GETOBJECT_UiaRoot, 1);

    /*
     * The HWND that was focused when adding the event handler is ignored,
     * EVENT_OBJECT_FOCUS only advises of events the first time
     * EVENT_OBJECT_FOCUS is raised on an HWND, and it tracks if it has
     * encountered a particular HWND before.
     */
    set_method_event_handle_for_providers(&Provider, &Provider_hwnd2, &Provider_nc2, method_event, ADVISE_EVENTS_EVENT_ADDED);
    set_uia_hwnd_expects(0, 1, 1, 3, 0); /* Only Win11 sends WM_GETOBJECT 3 times. */

    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 750) == WAIT_TIMEOUT, "Wait for method_event(s) didn't time out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");

    set_method_event_handle_for_providers(&Provider, &Provider_hwnd2, &Provider_nc2, NULL, -1);
    check_uia_hwnd_expects_at_most(0, 1, 1, 3, 0);

    /* Only OBJID_CLIENT is listened for, all other OBJIDs are ignored. */
    set_method_event_handle_for_providers(&Provider2, &Provider_hwnd3, &Provider_nc3, method_event, ADVISE_EVENTS_EVENT_ADDED);

    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_child_hwnd, OBJID_WINDOW, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 750) == WAIT_TIMEOUT, "Wait for method_event(s) didn't time out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");

    /*
     * First time EVENT_OBJECT_FOCUS is fired for this HWND, it will be
     * advised of UIA_AutomationFocusChangedEventId being listened for.
     * But only the serverside provider.
     */
    set_method_event_handle_for_providers(&Provider2, &Provider_hwnd3, &Provider_nc3, NULL, -1);
    reset_event_advise_values_for_hwnd_providers(&Provider2, &Provider_hwnd3, &Provider_nc3);
    set_provider_method_event_data(&Provider2, method_event[0], ADVISE_EVENTS_EVENT_ADDED);
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 3); /* Only sent 3 times on Win11. */
    set_uia_hwnd_expects(0, 1, 1, 2, 0); /* Only Win11 sends WM_GETOBJECT 2 times. */

    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_child_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, 1, 2000) != WAIT_TIMEOUT, "Wait for method_event(s) timed out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");

    check_uia_hwnd_expects_at_most(0, 1, 1, 2, 0);
    CHECK_CALLED_AT_MOST(child_winproc_GETOBJECT_UiaRoot, 3);
    test_provider_event_advise_added(&Provider2, UIA_AutomationFocusChangedEventId, FALSE);
    test_provider_event_advise_added(&Provider_hwnd3, 0, FALSE);
    test_provider_event_advise_added(&Provider_nc3, 0, FALSE);

    /* Doing it again has no effect, it has already been advised. */
    reset_event_advise_values_for_hwnd_providers(&Provider2, &Provider_hwnd3, &Provider_nc3);
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 3); /* Only sent 3 times on Win11. */
    set_uia_hwnd_expects(0, 1, 1, 2, 0); /* Only Win11 sends WM_GETOBJECT 2 times. */

    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_child_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, 1, 750) == WAIT_TIMEOUT, "Wait for method_event(s) didn't time out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");

    set_provider_method_event_data(&Provider2, NULL, -1);
    check_uia_hwnd_expects_at_most(0, 1, 1, 2, 0);
    CHECK_CALLED_AT_MOST(child_winproc_GETOBJECT_UiaRoot, 3);
    test_hwnd_providers_event_advise_added(&Provider2, &Provider_hwnd3, &Provider_nc3, 0, FALSE);

    /*
     * Same deal here - we've already encountered this HWND and advised it
     * when the event handler was added initially, so it too is ignored.
     */
    set_method_event_handle_for_providers(&Provider, &Provider_hwnd2, &Provider_nc2, method_event, ADVISE_EVENTS_EVENT_ADDED);
    reset_event_advise_values_for_hwnd_providers(&Provider, &Provider_hwnd2, &Provider_nc2);
    set_uia_hwnd_expects(0, 1, 1, 3, 0); /* Only Win11 sends WM_GETOBJECT 3 times. */

    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(method_event, event_handle_count, 750) == WAIT_TIMEOUT, "Wait for method_event(s) didn't time out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");

    set_method_event_handle_for_providers(&Provider, &Provider_hwnd2, &Provider_nc2, NULL, -1);
    check_uia_hwnd_expects_at_most(0, 1, 1, 3, 0);
    test_hwnd_providers_event_advise_added(&Provider, &Provider_hwnd2, &Provider_nc2, 0, FALSE);

    /* HWND destruction is tracked with EVENT_OBJECT_DESTROY/OBJID_WINDOW. */
    NotifyWinEvent(EVENT_OBJECT_DESTROY, test_child_hwnd, OBJID_WINDOW, CHILDID_SELF);
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");

    /*
     * EVENT_OBJECT_DESTROY removed this HWND, EVENT_OBJECT_FOCUS will now
     * advise it again.
     */
    reset_event_advise_values_for_hwnd_providers(&Provider2, &Provider_hwnd3, &Provider_nc3);
    set_provider_method_event_data(&Provider2, method_event[0], ADVISE_EVENTS_EVENT_ADDED);
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 3); /* Only sent 3 times on Win11. */
    set_uia_hwnd_expects(0, 1, 1, 2, 0); /* Only Win11 sends WM_GETOBJECT 2 times. */

    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_child_hwnd, OBJID_CLIENT, CHILDID_SELF);
    wait_res = msg_wait_for_all_events(method_event, 1, 2000);
    ok(wait_res != WAIT_TIMEOUT || broken(wait_res == WAIT_TIMEOUT), /* Win10v1709 */
            "Wait for method_event(s) timed out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");

    set_provider_method_event_data(&Provider2, NULL, -1);
    check_uia_hwnd_expects_at_most(0, 1, 1, 2, 0);
    CHECK_CALLED_AT_MOST(child_winproc_GETOBJECT_UiaRoot, 3);
    if (!winetest_platform_is_wine && (wait_res == WAIT_TIMEOUT))  /* Win10v1709 */
        test_provider_event_advise_added(&Provider2, 0, FALSE);
    else
        test_provider_event_advise_added(&Provider2, UIA_AutomationFocusChangedEventId, FALSE);
    test_provider_event_advise_added(&Provider_hwnd3, 0, FALSE);
    test_provider_event_advise_added(&Provider_nc3, 0, FALSE);

    set_uia_hwnd_expects(0, 1, 1, 0, 0);
    hr = IUIAutomation_RemoveFocusChangedEventHandler(uia_iface,
            &FocusChangedHandler.IUIAutomationFocusChangedEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_hwnd_expects(0, FALSE, 1, FALSE, 1, FALSE, 0, FALSE, 0, FALSE);

exit:
    Provider_hwnd3.parent = &Provider_hwnd2.IRawElementProviderFragment_iface;
    for (i = 0; i < ARRAY_SIZE(method_event); i++)
        CloseHandle(method_event[i]);
}

static struct MultiEventData {
    struct ExpectedEventQueue exp_events;

    HANDLE event_handle;
    BOOL ignore_callback;
} MultiEventData;

static void set_multi_event_data(struct node_provider_desc *exp_node_desc)
{
    memset(&MultiEventData.exp_events, 0, sizeof(MultiEventData.exp_events));
    push_event_queue_event(&MultiEventData.exp_events, exp_node_desc);

    SET_EXPECT(uia_event_callback);
}

static void push_expected_event(struct node_provider_desc *exp_node_desc)
{
    push_event_queue_event(&MultiEventData.exp_events, exp_node_desc);
    SET_EXPECT_MULTI(uia_event_callback, MultiEventData.exp_events.exp_event_count);
}

static void WINAPI multi_uia_event_callback(struct UiaEventArgs *args, SAFEARRAY *req_data, BSTR tree_struct)
{
    struct node_provider_desc *exp_desc;
    LONG exp_lbound[2], exp_elems[2];

    if (MultiEventData.ignore_callback)
        return;

    CHECK_EXPECT(uia_event_callback);

    exp_lbound[0] = exp_lbound[1] = 0;
    exp_elems[0] = exp_elems[1] = 1;
    exp_desc = pop_event_queue_event(&MultiEventData.exp_events);
    test_cache_req_sa(req_data, exp_lbound, exp_elems, exp_desc);

    ok(!wcscmp(tree_struct, L"P)"), "tree structure %s\n", debugstr_w(tree_struct));
    if (MultiEventData.event_handle && (MultiEventData.exp_events.exp_event_count == MultiEventData.exp_events.exp_event_pos))
        SetEvent(MultiEventData.event_handle);
}

static void test_uia_com_focus_change_event_handler_win_event_handling(IUIAutomation *uia_iface, HWND test_hwnd,
        HWND test_child_hwnd)
{
    struct UiaCacheRequest cache_req = { (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, NULL, 0, NULL, 0,
                                         AutomationElementMode_Full };
    struct node_provider_desc exp_node_desc, exp_nested_node_desc;
    struct Provider_prop_override prop_override;
    HANDLE event_handles[2];
    HUIAEVENT event;
    HUIANODE node;
    HRESULT hr;
    VARIANT v;
    int i;

    for (i = 0; i < ARRAY_SIZE(event_handles); i++)
        event_handles[i] = CreateEventW(NULL, FALSE, FALSE, NULL);

    set_uia_hwnd_expects(0, 1, 1, 0, 0);
    hr = UiaGetRootNode(&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!node, "Node == NULL.\n");
    check_uia_hwnd_expects(0, FALSE, 1, FALSE, 1, FALSE, 0, FALSE, 0, FALSE);

    set_uia_hwnd_expects(0, 6, 6, 5, 0);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot); /* Only sent on Win11. */
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, multi_uia_event_callback, TreeScope_Subtree, NULL, 0, &cache_req,
            &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL.\n");
    check_uia_hwnd_expects_at_most(0, 6, 6, 5, 0);
    CHECK_CALLED_AT_MOST(child_winproc_GETOBJECT_UiaRoot, 1);

    UiaNodeRelease(node);

    MultiEventData.event_handle = event_handles[0];
    ComEventData.event_handle = event_handles[1];

    /*
     * No IUIAutomationFocusChangedEventHandler is installed, and no
     * IProxyProviderWinEventHandler interfaces were returned from our
     * clientside desktop providers. This WinEvent will be ignored.
     */
    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(event_handles, 1, 750) == WAIT_TIMEOUT, "Wait for event_handle(s) didn't time out.\n");

    /* Add our IUIAutomationFocusChangedEventHandler. */
    set_uia_hwnd_expects(0, 6, 6, 5, 0);
    SET_EXPECT(child_winproc_GETOBJECT_UiaRoot); /* Only sent on Win11. */
    MultiEventData.ignore_callback = TRUE;
    FocusChangedHandler.event_handler_added = FALSE;
    hr = IUIAutomation_AddFocusChangedEventHandler(uia_iface, NULL,
            &FocusChangedHandler.IUIAutomationFocusChangedEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(FocusChangedHandler.ref > 1, "Unexpected refcnt %ld\n", FocusChangedHandler.ref);
    check_uia_hwnd_expects_at_most(0, 6, 6, 5, 0);
    CHECK_CALLED_AT_MOST(child_winproc_GETOBJECT_UiaRoot, 1);

    /*
     * Now that we have an IUIAutomationFocusChangedEventHandler installed,
     * EVENT_OBJECT_FOCUS events will be translated into native UIA events on
     * our serverside provider. This is done for both HUIAEVENTs and COM
     * events, unlike event advisement which only applies to COM event
     * handlers.
     */
    MultiEventData.ignore_callback = FALSE;
    FocusChangedHandler.event_handler_added = TRUE;
    init_node_provider_desc(&exp_nested_node_desc, GetCurrentProcessId(), test_hwnd);
    add_provider_desc(&exp_nested_node_desc, L"Main", L"Provider", TRUE);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), test_hwnd);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd2", FALSE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc2", FALSE);
    add_nested_provider_desc(&exp_node_desc, L"Main", NULL, TRUE, &exp_nested_node_desc);

    set_multi_event_data(&exp_node_desc);
    set_com_event_data(&exp_node_desc);

    set_uia_hwnd_expects(0, 2, 2, 4, 0); /* Win11 sends 4 WM_GETOBJECT messages, normally only 3. */
    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(event_handles, 2, 5000) != WAIT_TIMEOUT, "Wait for event_handle(s) timed out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");
    check_uia_hwnd_expects_at_least(0, FALSE, 2, FALSE, 2, FALSE, 3, FALSE, 0, FALSE);
    CHECK_CALLED(uia_com_event_callback);
    CHECK_CALLED(uia_event_callback);

    /*
     * Child ID is ignored when translating EVENT_OBJECT_FOCUS events into
     * native UIA events on our serverside provider.
     */
    set_multi_event_data(&exp_node_desc);
    set_com_event_data(&exp_node_desc);
    set_uia_hwnd_expects(0, 2, 2, 4, 0); /* Win11 sends 4 WM_GETOBJECT messages, normally only 3. */

    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_hwnd, OBJID_CLIENT, -1);
    ok(msg_wait_for_all_events(event_handles, 2, 5000) != WAIT_TIMEOUT, "Wait for event_handle(s) timed out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");
    check_uia_hwnd_expects_at_least(0, FALSE, 2, FALSE, 2, FALSE, 3, FALSE, 0, FALSE);
    CHECK_CALLED(uia_com_event_callback);
    CHECK_CALLED(uia_event_callback);

    /*
     * UIA queries the serverside provider for UIA_HasKeyboardFocusPropertyId.
     * If this is anything other than TRUE, it won't raise an event for our
     * serverside provider.
     */
    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_HasKeyboardFocusPropertyId, &v);
    set_provider_prop_override(&Provider, &prop_override, 1);

    SET_EXPECT_MULTI(winproc_GETOBJECT_UiaRoot, 2); /* Only done twice on Win11. */
    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(event_handles, 2, 750) == WAIT_TIMEOUT, "Wait for event_handle(s) didn't time out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    set_provider_prop_override(&Provider, NULL, 0);

    /*
     * The first time EVENT_OBJECT_FOCUS is raised for an HWND with a
     * serverside provider UIA will query for the currently focused provider
     * and raise a focus change event for it, alongside advising the root
     * provider of focus change events being listened for. All subsequent
     * EVENT_OBJECT_FOCUS events on the same HWND only query the root
     * provider.
     */
    initialize_provider(&Provider_child2, ProviderOptions_ServerSideProvider, NULL, TRUE);
    Provider_child2.parent = &Provider2.IRawElementProviderFragment_iface;
    Provider_child2.frag_root = &Provider2.IRawElementProviderFragmentRoot_iface;
    Provider2.focus_prov = &Provider_child2.IRawElementProviderFragment_iface;
    set_provider_runtime_id(&Provider_child2, UiaAppendRuntimeId, 2);
    initialize_provider_advise_events_ids(&Provider2);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider_child2", TRUE);

    set_multi_event_data(&exp_node_desc);
    set_com_event_data(&exp_node_desc);

    /* Second event. */
    init_node_provider_desc(&exp_nested_node_desc, GetCurrentProcessId(), test_child_hwnd);
    add_provider_desc(&exp_nested_node_desc, L"Main", L"Provider2", TRUE);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), test_child_hwnd);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd3", TRUE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc3", FALSE);
    add_nested_provider_desc(&exp_node_desc, L"Main", NULL, FALSE, &exp_nested_node_desc);

    push_expected_event(&exp_node_desc);
    push_expected_com_event(&exp_node_desc);
    set_uia_hwnd_expects(0, 2, 2, 3, 0); /* Win11 sends 3 WM_GETOBJECT messages, normally only 2. */
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 4); /* Win11 sends 4 WM_GETOBJECT messages, normally only 3. */
    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_child_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(event_handles, 2, 5000) != WAIT_TIMEOUT, "Wait for event_handle(s) timed out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");
    check_uia_hwnd_expects_at_least(0, FALSE, 2, FALSE, 2, FALSE, 2, TRUE, 0, FALSE);
    CHECK_CALLED_AT_LEAST(child_winproc_GETOBJECT_UiaRoot, 3);
    CHECK_CALLED_MULTI(uia_com_event_callback, 2);
    CHECK_CALLED_MULTI(uia_event_callback, 2);

    /*
     * Second time EVENT_OBJECT_FOCUS is raised for this HWND, only the root
     * provider will have an event raised.
     */
    init_node_provider_desc(&exp_nested_node_desc, GetCurrentProcessId(), test_child_hwnd);
    add_provider_desc(&exp_nested_node_desc, L"Main", L"Provider2", TRUE);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), test_child_hwnd);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd3", TRUE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc3", FALSE);
    add_nested_provider_desc(&exp_node_desc, L"Main", NULL, FALSE, &exp_nested_node_desc);

    set_multi_event_data(&exp_node_desc);
    set_com_event_data(&exp_node_desc);
    set_uia_hwnd_expects(0, 2, 2, 3, 0); /* Win11 sends 3 WM_GETOBJECT messages, normally only 2. */
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 4); /* Win11 sends 4 WM_GETOBJECT messages, normally only 3. */
    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_child_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(event_handles, 2, 5000) != WAIT_TIMEOUT, "Wait for event_handle(s) timed out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");
    check_uia_hwnd_expects_at_least(0, FALSE, 2, FALSE, 2, FALSE, 2, TRUE, 0, FALSE);
    CHECK_CALLED_AT_LEAST(child_winproc_GETOBJECT_UiaRoot, 3);
    CHECK_CALLED(uia_com_event_callback);
    CHECK_CALLED(uia_event_callback);

    /*
     * Windows 7 has quirky behavior around MSAA proxy creation, skip tests.
     */
    if (!UiaLookupId(AutomationIdentifierType_Property, &OptimizeForVisualContent_Property_GUID))
    {
        win_skip("Skipping focus MSAA proxy tests for Win7\n");
        goto exit;
    }

    /*
     * Creates an MSAA proxy, raises event on that.
     */
    prov_root = NULL;
    set_accessible_props(&Accessible, ROLE_SYSTEM_CLIENT, STATE_SYSTEM_FOCUSED, 0, L"acc_name", 0, 0, 0, 0);
    Accessible.ow_hwnd = test_hwnd;
    Accessible.focus_child_id = CHILDID_SELF;
    acc_client = &Accessible.IAccessible_iface;

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), test_hwnd);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd2", FALSE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc2", FALSE);
    add_provider_desc(&exp_node_desc, L"Main", NULL, TRUE); /* MSAA Proxy provider. */

    set_multi_event_data(&exp_node_desc);
    set_com_event_data(&exp_node_desc);

    set_uia_hwnd_expects(0, 1, 1, 4, 4); /* Win11 sends 4 WM_GETOBJECT messages, normally only 2. */
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, QI_IAccIdentity, 3);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accParent, 3);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(event_handles, 2, 5000) != WAIT_TIMEOUT, "Wait for event_handle(s) timed out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");
    check_uia_hwnd_expects_at_least(0, FALSE, 1, FALSE, 1, FALSE, 1, FALSE, 1, FALSE);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible, QI_IAccIdentity);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible, get_accParent);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    CHECK_CALLED(uia_com_event_callback);
    CHECK_CALLED(uia_event_callback);

    /*
     * Return Accessible_child2 from get_accFocus.
     */
    set_accessible_props(&Accessible, ROLE_SYSTEM_CLIENT, STATE_SYSTEM_FOCUSED, 0, L"acc_name", 0, 0, 0, 0);
    Accessible.focus_child_id = 4; /* 4 gets us Accessible_child2. */
    set_accessible_props(&Accessible_child2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_FOCUSED, 0, L"acc_child2", 0, 0, 0, 0);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", NULL, TRUE); /* MSAA Proxy provider. */

    set_multi_event_data(&exp_node_desc);
    set_com_event_data(&exp_node_desc);
    set_uia_hwnd_expects(0, 0, 0, 2, 3); /* Win11 sends 2/3 WM_GETOBJECT messages, normally only 1/2. */
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, QI_IAccIdentity, 4); /* Only done 4 times on Win11, normally 3. */
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accParent, 3); /* Only done 3 times on Win11, normally 2. */
    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accRole);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, accNavigate); /* Wine only, Windows doesn't pass this through the DA wrapper. */
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible_child2, get_accParent, 2);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible_child2, get_accState, 2);
    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(event_handles, 2, 5000) != WAIT_TIMEOUT, "Wait for event_handle(s) timed out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");
    check_uia_hwnd_expects_at_least(0, FALSE, 0, FALSE, 0, FALSE, 1, FALSE, 2, FALSE);
    todo_wine CHECK_ACC_METHOD_CALLED_AT_LEAST(&Accessible, QI_IAccIdentity, 3);
    todo_wine CHECK_ACC_METHOD_CALLED_AT_LEAST(&Accessible, get_accParent, 2);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible_child2, QI_IAccIdentity);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accRole);
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible_child2, get_accParent, 2);
    CHECK_ACC_METHOD_CALLED_MULTI(&Accessible_child2, get_accState, 2);
    CHECK_ACC_METHOD_CALLED_AT_MOST(&Accessible_child2, accNavigate, 1);
    CHECK_CALLED(uia_com_event_callback);
    CHECK_CALLED(uia_event_callback);

    /*
     * accFocus returns Accessible_child2, however it has
     * STATE_SYSTEM_INVISIBLE set. Falls back to Accessible.
     */
    set_accessible_props(&Accessible_child2, ROLE_SYSTEM_DOCUMENT, STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_FOCUSED, 0, L"acc_child2", 0, 0, 0, 0);
    set_accessible_props(&Accessible, ROLE_SYSTEM_CLIENT, STATE_SYSTEM_FOCUSED, 0, L"acc_name", 0, 0, 0, 0);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), test_hwnd);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd2", FALSE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc2", FALSE);
    add_provider_desc(&exp_node_desc, L"Main", NULL, TRUE); /* MSAA Proxy provider. */

    set_multi_event_data(&exp_node_desc);
    set_com_event_data(&exp_node_desc);
    set_uia_hwnd_expects(0, 1, 1, 4, 4); /* Win11 sends 4 WM_GETOBJECT messages, normally only 2. */
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, QI_IAccIdentity, 4); /* Only done 4 times on Win11, normally 2. */
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, get_accParent, 3); /* Only done 3 times on Win11, normally 1. */
    SET_ACC_METHOD_EXPECT(&Accessible, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accState);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, accNavigate); /* Wine only, Windows doesn't pass this through the DA wrapper. */
    SET_ACC_METHOD_EXPECT(&Accessible_child2, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accParent);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accState);
    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_hwnd, OBJID_CLIENT, CHILDID_SELF);
    ok(msg_wait_for_all_events(event_handles, 2, 5000) != WAIT_TIMEOUT, "Wait for event_handle(s) timed out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");
    check_uia_hwnd_expects_at_least(0, FALSE, 1, FALSE, 1, FALSE, 1, FALSE, 1, FALSE);
    todo_wine CHECK_ACC_METHOD_CALLED_AT_LEAST(&Accessible, QI_IAccIdentity, 2);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible, get_accParent);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accState);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible_child2, QI_IAccIdentity);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accParent);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accState);
    CHECK_ACC_METHOD_CALLED_AT_MOST(&Accessible_child2, accNavigate, 1);
    CHECK_CALLED(uia_com_event_callback);
    CHECK_CALLED(uia_event_callback);

    /*
     * Get Accessible_child2 by raising an event with its child ID directly.
     * It will have its get_accFocus method called.
     */
    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", NULL, TRUE); /* MSAA Proxy provider. */

    set_multi_event_data(&exp_node_desc);
    set_com_event_data(&exp_node_desc);
    set_uia_hwnd_expects(0, 0, 0, 2, 2); /* Win11 sends 2/2 WM_GETOBJECT messages, normally only 1/1. */
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible, QI_IAccIdentity, 2); /* Only done 2 times on Win11, normally 1. */
    SET_ACC_METHOD_EXPECT(&Accessible, get_accParent); /* Only done on Win11. */
    SET_ACC_METHOD_EXPECT(&Accessible, get_accChild);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accFocus);
    SET_ACC_METHOD_EXPECT(&Accessible_child2, get_accState);
    SET_ACC_METHOD_EXPECT_MULTI(&Accessible_child2, get_accParent, 2);
    NotifyWinEvent(EVENT_OBJECT_FOCUS, test_hwnd, OBJID_CLIENT, 4);
    ok(msg_wait_for_all_events(event_handles, 2, 5000) != WAIT_TIMEOUT, "Wait for event_handle(s) timed out.\n");
    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");
    check_uia_hwnd_expects_at_least(0, FALSE, 0, FALSE, 0, FALSE, 1, FALSE, 1, FALSE);
    CHECK_ACC_METHOD_CALLED_AT_MOST(&Accessible, get_accParent, 1); /* Only done on Win11. */
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible, QI_IAccIdentity);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accChild);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible_child2, QI_IAccIdentity);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accFocus);
    CHECK_ACC_METHOD_CALLED(&Accessible_child2, get_accState);
    CHECK_ACC_METHOD_CALLED_AT_MOST(&Accessible_child2, get_accParent, 2);
    CHECK_CALLED(uia_com_event_callback);
    CHECK_CALLED(uia_event_callback);
    acc_client = NULL;

exit:
    set_uia_hwnd_expects(0, 1, 1, 0, 0);
    hr = IUIAutomation_RemoveFocusChangedEventHandler(uia_iface,
            &FocusChangedHandler.IUIAutomationFocusChangedEventHandler_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_hwnd_expects(0, FALSE, 1, FALSE, 1, FALSE, 0, FALSE, 0, FALSE);

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ComEventData.event_handle = MultiEventData.event_handle = NULL;
    for (i = 0; i < ARRAY_SIZE(event_handles); i++)
        CloseHandle(event_handles[i]);
}

static DWORD WINAPI uia_com_event_handler_win_event_test_thread(LPVOID param)
{
    struct com_win_event_test_thread_data *test_data = (struct com_win_event_test_thread_data *)param;
    IUIAutomation *uia_iface = test_data->uia_iface;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    UiaRegisterProviderCallback(uia_com_win_event_clientside_provider_callback);

    test_uia_com_event_handler_event_advisement(uia_iface, test_data->test_hwnd, test_data->test_child_hwnd);
    test_uia_com_focus_change_event_handler_win_event_handling(uia_iface, test_data->test_hwnd, test_data->test_child_hwnd);

    if (wait_for_clientside_callbacks(2000)) trace("Kept getting callbacks up until timeout\n");
    UiaRegisterProviderCallback(NULL);
    CoUninitialize();

    return 0;
}

static void test_CUIAutomation_event_handlers(IUIAutomation *uia_iface)
{
    struct com_win_event_test_thread_data test_data = { uia_iface, NULL };
    IUIAutomationElement *elem;
    HANDLE thread;
    HRESULT hr;
    HWND hwnd;

    test_data.test_hwnd = ComEventData.event_hwnd = hwnd = create_test_hwnd("test_CUIAutomation_event_handlers class");

    /* Set up providers for the desktop window and our test HWND. */
    set_clientside_providers_for_hwnd(&Provider_proxy, &Provider_nc, &Provider_hwnd, GetDesktopWindow());
    base_hwnd_prov = &Provider_hwnd.IRawElementProviderSimple_iface;
    proxy_prov = &Provider_proxy.IRawElementProviderSimple_iface;
    nc_prov = &Provider_nc.IRawElementProviderSimple_iface;

    set_clientside_providers_for_hwnd(NULL, &Provider_nc2, &Provider_hwnd2, hwnd);
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, hwnd, TRUE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    Provider.ignore_hwnd_prop = TRUE;
    prov_root = &Provider.IRawElementProviderSimple_iface;

    method_sequences_enabled = FALSE;
    set_uia_hwnd_expects(1, 1, 1, 1, 1);
    UiaRegisterProviderCallback(test_uia_provider_callback);
    hr = IUIAutomation_ElementFromHandle(uia_iface, hwnd, &elem);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!elem, "elem == NULL\n");
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider_hwnd2.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd2.ref);
    ok(Provider_nc2.ref == 2, "Unexpected refcnt %ld\n", Provider_nc2.ref);
    check_uia_hwnd_expects(1, TRUE, 1, FALSE, 1, FALSE, 1, FALSE, 0, FALSE);

    test_IUIAutomationEventHandler(uia_iface, elem);
    test_IUIAutomationFocusChangedEventHandler(uia_iface);
    IUIAutomationElement_Release(elem);

    /* Create a test child window. */
    test_data.test_child_hwnd = create_child_test_hwnd("test_CUIAutomation_event_handlers child class", hwnd);
    set_clientside_providers_for_hwnd(NULL, &Provider_nc3, &Provider_hwnd3, test_data.test_child_hwnd);
    initialize_provider(&Provider2, ProviderOptions_ServerSideProvider, test_data.test_child_hwnd, TRUE);
    Provider2.frag_root = &Provider2.IRawElementProviderFragmentRoot_iface;
    Provider2.ignore_hwnd_prop = TRUE;
    Provider_hwnd3.parent = &Provider_hwnd2.IRawElementProviderFragment_iface;
    child_win_prov_root = &Provider2.IRawElementProviderSimple_iface;

    hr = IUIAutomation_RemoveAllEventHandlers(uia_iface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /*
     * Particular versions of Windows 7 trigger access violations when doing
     * WinEvent tests, just skip them for Windows 7.
     */
    if (!UiaLookupId(AutomationIdentifierType_Property, &OptimizeForVisualContent_Property_GUID))
    {
        win_skip("Skipping COM API WinEvent translation tests for Win7\n");
        goto exit;
    }

    thread = CreateThread(NULL, 0, uia_com_event_handler_win_event_test_thread, (void *)&test_data, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;

        while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);

exit:
    UiaRegisterProviderCallback(NULL);
    DestroyWindow(hwnd);
    UnregisterClassA("test_CUIAutomation_event_handlers class", NULL);
    UnregisterClassA("test_CUIAutomation_event_handlers child class", NULL);
    method_sequences_enabled = TRUE;
}

struct uia_com_classes {
    const GUID *clsid;
    const GUID *iid;
};

static const struct uia_com_classes com_classes[] = {
    { &CLSID_CUIAutomation,  &IID_IUnknown },
    { &CLSID_CUIAutomation,  &IID_IUIAutomation },
    { &CLSID_CUIAutomation8, &IID_IUnknown },
    { &CLSID_CUIAutomation8, &IID_IUIAutomation },
    { &CLSID_CUIAutomation8, &IID_IUIAutomation2 },
    { &CLSID_CUIAutomation8, &IID_IUIAutomation3 },
    { &CLSID_CUIAutomation8, &IID_IUIAutomation4 },
    { &CLSID_CUIAutomation8, &IID_IUIAutomation5 },
    { &CLSID_CUIAutomation8, &IID_IUIAutomation6 },
};

static void test_CUIAutomation(void)
{
    BOOL has_cui8 = TRUE, tmp_b;
    IUIAutomation *uia_iface;
    IUnknown *unk1, *unk2;
    HRESULT hr;
    VARIANT v;
    int i;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    for (i = 0; i < ARRAY_SIZE(com_classes); i++)
    {
        IUnknown *iface = NULL;

        hr = CoCreateInstance(com_classes[i].clsid, NULL, CLSCTX_INPROC_SERVER, com_classes[i].iid,
                (void **)&iface);

        if ((com_classes[i].clsid == &CLSID_CUIAutomation8) && (hr == REGDB_E_CLASSNOTREG))
        {
            win_skip("CLSID_CUIAutomation8 class not registered, skipping further tests.\n");
            has_cui8 = FALSE;
            break;
        }
        else if ((com_classes[i].clsid == &CLSID_CUIAutomation8) && (hr == E_NOINTERFACE) &&
                (com_classes[i].iid != &IID_IUIAutomation2) && (com_classes[i].iid != &IID_IUIAutomation) &&
                (com_classes[i].iid != &IID_IUnknown))
        {
            win_skip("No object for clsid %s, iid %s, skipping further tests.\n", debugstr_guid(com_classes[i].clsid),
                debugstr_guid(com_classes[i].iid));
            break;
        }

        ok(hr == S_OK, "Failed to create interface for clsid %s, iid %s, hr %#lx\n",
                debugstr_guid(com_classes[i].clsid), debugstr_guid(com_classes[i].iid), hr);
        ok(!!iface, "iface == NULL\n");
        IUnknown_Release(iface);
    }

    if (has_cui8)
        hr = CoCreateInstance(&CLSID_CUIAutomation8, NULL, CLSCTX_INPROC_SERVER, &IID_IUIAutomation,
                (void **)&uia_iface);
    else
        hr = CoCreateInstance(&CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER, &IID_IUIAutomation,
                (void **)&uia_iface);
    ok(hr == S_OK, "Failed to create IUIAutomation interface, hr %#lx\n", hr);
    ok(!!uia_iface, "uia_iface == NULL\n");

    /* Reserved value retrieval methods. */
    hr = UiaGetReservedNotSupportedValue(&unk1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomation_get_ReservedNotSupportedValue(uia_iface, &unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk1 == unk2, "unk1 != unk2\n");

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = unk1;
    hr = IUIAutomation_CheckNotSupported(uia_iface, v, &tmp_b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(tmp_b == TRUE, "tmp_b != TRUE\n");

    IUnknown_Release(unk1);
    IUnknown_Release(unk2);

    hr = UiaGetReservedMixedAttributeValue(&unk1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IUIAutomation_get_ReservedMixedAttributeValue(uia_iface, &unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk1 == unk2, "unk1 != unk2\n");

    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = unk1;
    hr = IUIAutomation_CheckNotSupported(uia_iface, v, &tmp_b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(tmp_b == FALSE, "tmp_b != FALSE\n");

    IUnknown_Release(unk1);
    IUnknown_Release(unk2);

    test_CUIAutomation_condition_ifaces(uia_iface);
    test_CUIAutomation_value_conversion(uia_iface);
    test_CUIAutomation_cache_request_iface(uia_iface);
    test_CUIAutomation_TreeWalker_ifaces(uia_iface);
    test_ElementFromHandle(uia_iface, has_cui8);
    test_Element_GetPropertyValue(uia_iface);
    test_Element_cache_methods(uia_iface);
    test_Element_Find(uia_iface);
    test_GetRootElement(uia_iface);
    test_GetFocusedElement(uia_iface);
    test_CUIAutomation_event_handlers(uia_iface);

    IUIAutomation_Release(uia_iface);
    CoUninitialize();
}

static const struct prov_method_sequence default_hwnd_prov_props_seq[] = {
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Windows 7. */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Windows 7. */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_NamePropertyId */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Windows 7. */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_NamePropertyId */
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Windows 7. */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ClassNamePropertyId */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Windows 7. */
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ClassNamePropertyId */
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Windows 7. */
    { &Provider, PROV_GET_PROPERTY_VALUE }, /* UIA_ProcessIdPropertyId */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { &Provider_nc, PROV_GET_PROPERTY_VALUE }, /* UIA_ProcessIdPropertyId */
    { &Provider_nc, PROV_GET_PROVIDER_OPTIONS, METHOD_TODO },
    { 0 }
};

#define test_node_hwnd_provider( node, hwnd ) \
        test_node_hwnd_provider_( (node), (hwnd), __FILE__, __LINE__)
static void test_node_hwnd_provider_(HUIANODE node, HWND hwnd, const char *file, int line)
{
    WCHAR buf[1024] = { 0 };
    HRESULT hr;
    VARIANT v;
    DWORD pid;

    winetest_push_context("UIA_NativeWindowHandlePropertyId");
    hr = UiaGetPropertyValue(node, UIA_NativeWindowHandlePropertyId, &v);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok_(file, line)(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
    ok_(file, line)(V_I4(&v) == HandleToUlong(hwnd), "V_I4(&v) = %#lx, expected %#lx\n", V_I4(&v), HandleToUlong(hwnd));
    VariantClear(&v);
    winetest_pop_context();

    winetest_push_context("UIA_NamePropertyId");
    SendMessageW(hwnd, WM_GETTEXT, ARRAY_SIZE(buf), (LPARAM)buf);
    hr = UiaGetPropertyValue(node, UIA_NamePropertyId, &v);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok_(file, line)(V_VT(&v) == VT_BSTR, "Unexpected VT %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), buf), "Unexpected BSTR %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    winetest_pop_context();

    winetest_push_context("UIA_ClassNamePropertyId");
    memset(buf, 0, sizeof(buf));
    GetClassNameW(hwnd, buf, ARRAY_SIZE(buf));
    hr = UiaGetPropertyValue(node, UIA_ClassNamePropertyId, &v);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok_(file, line)(V_VT(&v) == VT_BSTR, "Unexpected VT %d\n", V_VT(&v));
    ok(!lstrcmpW(V_BSTR(&v), buf), "Unexpected BSTR %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    winetest_pop_context();

    GetWindowThreadProcessId(hwnd, &pid);
    winetest_push_context("UIA_ProcessIdPropertyId");
    hr = UiaGetPropertyValue(node, UIA_ProcessIdPropertyId, &v);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok_(file, line)(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
    ok_(file, line)(V_I4(&v) == pid, "V_I4(&v) = %#lx, expected %#lx\n", V_I4(&v), pid);
    VariantClear(&v);
    winetest_pop_context();
}

enum {
    PARENT_HWND_NULL,
    PARENT_HWND_HWND,
    PARENT_HWND_DESKTOP,
};

struct uia_hwnd_control_type_test {
    DWORD style;
    DWORD style_ex;
    int exp_control_type;
    int parent_hwnd_type;
};

static const struct uia_hwnd_control_type_test hwnd_control_type_test[] = {
    { WS_OVERLAPPEDWINDOW,   0,                UIA_WindowControlTypeId, PARENT_HWND_NULL },
    /* Top-level window (parent is desktop window) is always a window control. */
    { WS_CHILD,              0,                UIA_WindowControlTypeId, PARENT_HWND_DESKTOP },
    /* Not a top-level window, considered a pane. */
    { WS_CHILD,              0,                UIA_PaneControlTypeId,   PARENT_HWND_HWND },
    /* Not a top-level window, but WS_EX_APPWINDOW is always considered a window. */
    { WS_CHILD,              WS_EX_APPWINDOW,  UIA_WindowControlTypeId, PARENT_HWND_HWND },
    /*
     * WS_POPUP is always a pane regardless of being a top level window,
     * unless WS_CAPTION is set.
     */
    { WS_CAPTION | WS_POPUP, 0,                UIA_WindowControlTypeId, PARENT_HWND_DESKTOP },
    { WS_BORDER | WS_POPUP,  0,                UIA_PaneControlTypeId,   PARENT_HWND_DESKTOP },
    { WS_POPUP,              0,                UIA_PaneControlTypeId,   PARENT_HWND_DESKTOP },
    /*
     * Top level window with WS_EX_TOOLWINDOW and without WS_CAPTION is
     * considered a pane.
     */
    { WS_CHILD,              WS_EX_TOOLWINDOW, UIA_PaneControlTypeId,   PARENT_HWND_DESKTOP },
    { WS_CHILD | WS_CAPTION, WS_EX_TOOLWINDOW, UIA_WindowControlTypeId, PARENT_HWND_DESKTOP },
};

static void create_base_hwnd_test_node(HWND hwnd, BOOL child_hwnd, struct Provider *main, struct Provider *nc,
        HUIANODE *ret_node)
{
    ULONG main_ref, nc_ref;
    HRESULT hr;

    initialize_provider(nc, ProviderOptions_ClientSideProvider | ProviderOptions_NonClientAreaProvider, hwnd, TRUE);
    initialize_provider(main, ProviderOptions_ClientSideProvider, hwnd, TRUE);
    nc->ignore_hwnd_prop = main->ignore_hwnd_prop = TRUE;
    main_ref = main->ref;
    nc_ref = nc->ref;

    if (!child_hwnd)
    {
        SET_EXPECT(winproc_GETOBJECT_UiaRoot);
        SET_EXPECT(winproc_GETOBJECT_CLIENT);
        prov_root = &main->IRawElementProviderSimple_iface;
    }
    else
    {
        SET_EXPECT(winproc_GETOBJECT_UiaRoot);
        SET_EXPECT(winproc_GETOBJECT_CLIENT);
        SET_EXPECT(child_winproc_GETOBJECT_UiaRoot);
        child_win_prov_root = &main->IRawElementProviderSimple_iface;
    }

    hr = UiaNodeFromProvider(&nc->IRawElementProviderSimple_iface, ret_node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(main->ref == (main_ref + 1), "Unexpected refcnt %ld\n", main->ref);
    ok(nc->ref == (nc_ref + 1), "Unexpected refcnt %ld\n", nc->ref);
    if (child_hwnd)
    {
        /* Called while trying to get override provider. */
        todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
        CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);
    }
    else
        CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;
    Provider.ret_invalid_prop_type = Provider_nc.ret_invalid_prop_type = TRUE;
}

#define test_node_hwnd_provider_navigation( node, dir, exp_dest_hwnd ) \
        test_node_hwnd_provider_navigation_( (node), (dir), (exp_dest_hwnd), __FILE__, __LINE__)
static void test_node_hwnd_provider_navigation_(HUIANODE node, int nav_dir, HWND exp_dest_hwnd, const char *file,
        int line)
{
    struct UiaCacheRequest cache_req = { NULL, TreeScope_Element, NULL, 0, NULL, 0, AutomationElementMode_Full };
    const WCHAR *exp_tree_struct = exp_dest_hwnd ? L"P)" : L"";
    SAFEARRAY *out_req = NULL;
    BSTR tree_struct = NULL;
    LONG idx[2] = { 0 };
    HUIANODE tmp_node;
    HRESULT hr;
    VARIANT v;
    int i;

    hr = UiaNavigate(node, nav_dir, (struct UiaCondition *)&UiaTrueCondition, &cache_req, &out_req, &tree_struct);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok_(file, line)(!!out_req, "out_req == NULL\n");
    ok_(file, line)(!!tree_struct, "tree_struct == NULL\n");
    if (!exp_dest_hwnd)
        goto exit;

    for (i = 0; i < 2; i++)
    {
        hr = SafeArrayGetLBound(out_req, 1 + i, &idx[i]);
        ok_(file, line)(hr == S_OK, "SafeArrayGetLBound unexpected hr %#lx\n", hr);
    }

    hr = SafeArrayGetElement(out_req, idx, &v);
    ok_(file, line)(hr == S_OK, "SafeArrayGetElement unexpected hr %#lx\n", hr);

    hr = UiaHUiaNodeFromVariant(&v, &tmp_node);
    ok_(file, line)(hr == S_OK, "UiaHUiaNodeFromVariant unexpected hr %#lx\n", hr);
    ok_(file, line)(!!tmp_node, "tmp_node == NULL\n");
    VariantClear(&v);

    hr = UiaGetPropertyValue(tmp_node, UIA_NativeWindowHandlePropertyId, &v);
    ok_(file, line)(hr == S_OK, "UiaGetPropertyValue unexpected hr %#lx\n", hr);
    ok_(file, line)(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok_(file, line)(V_I4(&v) == HandleToUlong(exp_dest_hwnd), "V_I4(&v) = %#lx, expected %#lx\n", V_I4(&v),
            HandleToUlong(exp_dest_hwnd));
    VariantClear(&v);
    UiaNodeRelease(tmp_node);

exit:
    ok_(file, line)(!wcscmp(tree_struct, exp_tree_struct), "unexpected tree structure %s\n", debugstr_w(tree_struct));
    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);
}

static void test_default_clientside_providers(void)
{
    struct UiaRect uia_rect = { 0 };
    HWND hwnd, hwnd_child, hwnd2;
    RECT rect = { 0 };
    IUnknown *unk_ns;
    HUIANODE node;
    HRESULT hr;
    VARIANT v;
    int i;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hwnd = create_test_hwnd("test_default_clientside_providers class");
    hwnd_child = create_child_test_hwnd("test_default_clientside_providers child class", hwnd);
    method_sequences_enabled = FALSE;

    hr = UiaGetReservedNotSupportedValue(&unk_ns);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /*
     * Test default BaseHwnd provider. Unlike the other default providers, the
     * default BaseHwnd IRawElementProviderSimple is not available to test
     * directly. To isolate the BaseHwnd provider, we set the node's nonclient
     * provider to Provider_nc, and its Main provider to Provider. These
     * providers will return nothing so that we can isolate properties coming
     * from the BaseHwnd provider.
     */
    create_base_hwnd_test_node(hwnd, FALSE, &Provider, &Provider_nc, &node);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc(V_BSTR(&v), L"Nonclient", L"Provider_nc", FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Main", L"Provider", FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Hwnd", NULL, TRUE);
    VariantClear(&v);

    method_sequences_enabled = TRUE;
    Provider.ret_invalid_prop_type = Provider_nc.ret_invalid_prop_type = TRUE;
    test_node_hwnd_provider(node, hwnd);
    ok_method_sequence(default_hwnd_prov_props_seq, "default_hwnd_prov_props_seq");
    method_sequences_enabled = FALSE;

    /* Get the bounding rectangle from the default BaseHwnd provider. */
    GetWindowRect(hwnd, &rect);
    set_uia_rect(&uia_rect, rect.left, rect.top, (rect.right - rect.left), (rect.bottom - rect.top));
    hr = UiaGetPropertyValue(node, UIA_BoundingRectanglePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_rect_val(&v, &uia_rect);
    VariantClear(&v);

    /* Minimized top-level HWNDs don't return a bounding rectangle. */
    ShowWindow(hwnd, SW_MINIMIZE);
    hr = UiaGetPropertyValue(node, UIA_BoundingRectanglePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&v) == VT_UNKNOWN, "Unexpected vt %d\n", V_VT(&v));
    ok(V_UNKNOWN(&v) == unk_ns, "unexpected IUnknown %p\n", V_UNKNOWN(&v));
    VariantClear(&v);

    UiaNodeRelease(node);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider_nc.ref == 1, "Unexpected refcnt %ld\n", Provider_nc.ref);

    /* Create a child window node. */
    create_base_hwnd_test_node(hwnd_child, TRUE, &Provider, &Provider_nc, &node);
    test_node_hwnd_provider(node, hwnd_child);

    /* Get the bounding rectangle from the default BaseHwnd provider. */
    GetWindowRect(hwnd_child, &rect);
    set_uia_rect(&uia_rect, rect.left, rect.top, (rect.right - rect.left), (rect.bottom - rect.top));
    hr = UiaGetPropertyValue(node, UIA_BoundingRectanglePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_rect_val(&v, &uia_rect);
    VariantClear(&v);

    /* Minimized non top-level HWNDs return a bounding rectangle. */
    ShowWindow(hwnd_child, SW_MINIMIZE);
    GetWindowRect(hwnd_child, &rect);
    set_uia_rect(&uia_rect, rect.left, rect.top, (rect.right - rect.left), (rect.bottom - rect.top));
    hr = UiaGetPropertyValue(node, UIA_BoundingRectanglePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_rect_val(&v, &uia_rect);
    VariantClear(&v);

    UiaNodeRelease(node);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider_nc.ref == 1, "Unexpected refcnt %ld\n", Provider_nc.ref);

    VariantInit(&v);
    for (i = 0; i < ARRAY_SIZE(hwnd_control_type_test); i++)
    {
        const struct uia_hwnd_control_type_test *test = &hwnd_control_type_test[i];
        HWND parent;

        if (test->parent_hwnd_type == PARENT_HWND_HWND)
            parent = hwnd;
        else if (test->parent_hwnd_type == PARENT_HWND_DESKTOP)
            parent = GetDesktopWindow();
        else
            parent = NULL;

        hwnd2 = CreateWindowExA(test->style_ex, "test_default_clientside_providers class", "Test window", test->style,
                0, 0, 100, 100, parent, NULL, NULL, NULL);
        initialize_provider(&Provider_nc, ProviderOptions_ClientSideProvider | ProviderOptions_NonClientAreaProvider, hwnd2, TRUE);
        initialize_provider(&Provider, ProviderOptions_ClientSideProvider, hwnd2, TRUE);
        Provider_nc.ignore_hwnd_prop = Provider.ignore_hwnd_prop = TRUE;
        Provider.ret_invalid_prop_type = Provider_nc.ret_invalid_prop_type = TRUE;

        /* If parent is hwnd, it will be queried for an override provider. */
        if (test->style == WS_CHILD && (parent == hwnd))
            SET_EXPECT_MULTI(winproc_GETOBJECT_UiaRoot, 2);
        else
            SET_EXPECT(winproc_GETOBJECT_UiaRoot);
        /* Only sent on Win7. */
        SET_EXPECT(winproc_GETOBJECT_CLIENT);
        hr = UiaNodeFromProvider(&Provider_nc.IRawElementProviderSimple_iface, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
        ok(Provider_nc.ref == 2, "Unexpected refcnt %ld\n", Provider_nc.ref);
        if (hwnd_control_type_test[i].style == WS_CHILD && (parent == hwnd))
            todo_wine CHECK_CALLED_MULTI(winproc_GETOBJECT_UiaRoot, 2);
        else
            CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
        called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

        hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
        ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
        ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
        ok(V_I4(&v) == test->exp_control_type, "Unexpected control type %ld\n", V_I4(&v));
        VariantClear(&v);

        UiaNodeRelease(node);
        DestroyWindow(hwnd2);
    }

    /*
     * Default ProviderType_BaseHwnd provider navigation tests.
     */
    create_base_hwnd_test_node(hwnd, FALSE, &Provider, &Provider_nc, &node);
    test_node_hwnd_provider(node, hwnd);

    /*
     * Navigate to the parent of our top-level HWND, will get a node
     * representing the desktop HWND.
     */
    test_node_hwnd_provider_navigation(node, NavigateDirection_Parent, GetDesktopWindow());
    UiaNodeRelease(node);

    /*
     * Create a node representing an HWND that is a top-level window, but is
     * owned by another window. For top-level HWNDs, parent navigation will go
     * to the owner instead of the parent.
     */
    hwnd2 = CreateWindowA("test_default_clientside_providers class", "Test window", WS_POPUP, 0, 0, 50, 50, hwnd, NULL,
            NULL, NULL);
    ok(GetAncestor(hwnd2, GA_PARENT) == GetDesktopWindow(), "unexpected parent hwnd");
    ok(GetWindow(hwnd2, GW_OWNER) == hwnd, "unexpected owner hwnd");
    create_base_hwnd_test_node(hwnd2, FALSE, &Provider, &Provider_nc, &node);
    test_node_hwnd_provider(node, hwnd2);

    /* Navigate to the parent. */
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    test_node_hwnd_provider_navigation(node, NavigateDirection_Parent, hwnd);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    UiaNodeRelease(node);
    DestroyWindow(hwnd2);

    /*
     * Create a node for our child window.
     */
    initialize_provider(&Provider2, ProviderOptions_ServerSideProvider, hwnd, TRUE);
    Provider2.ignore_hwnd_prop = TRUE;
    prov_root = &Provider2.IRawElementProviderSimple_iface;
    create_base_hwnd_test_node(hwnd_child, TRUE, &Provider, &Provider_nc, &node);
    test_node_hwnd_provider(node, hwnd_child);

    /* Navigate to parent. */
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);
    test_node_hwnd_provider_navigation(node, NavigateDirection_Parent, hwnd);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    called_winproc_GETOBJECT_CLIENT = expect_winproc_GETOBJECT_CLIENT = 0;

    UiaNodeRelease(node);
    prov_root = NULL;

    /*
     * Test default ProviderType_Proxy clientside provider. Provider will be
     * the HWND provider for this node, and Accessible will be the main
     * provider.
     */
    initialize_provider(&Provider, ProviderOptions_ClientSideProvider, hwnd, FALSE);
    set_accessible_props(&Accessible, ROLE_SYSTEM_TEXT, STATE_SYSTEM_FOCUSABLE, 0, L"Accessible", 0, 0, 20, 20);
    set_accessible_ia2_props(&Accessible, FALSE, 0);
    acc_client = &Accessible.IAccessible_iface;
    prov_root = child_win_prov_root = NULL;

    SET_ACC_METHOD_EXPECT(&Accessible, QI_IAccIdentity);
    SET_ACC_METHOD_EXPECT(&Accessible, get_accParent);
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    /* Only sent twice on Win7. */
    SET_EXPECT_MULTI(winproc_GETOBJECT_CLIENT, 2);
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Accessible.ref >= 2, "Unexpected refcnt %ld\n", Accessible.ref);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible, QI_IAccIdentity);
    todo_wine CHECK_ACC_METHOD_CALLED(&Accessible, get_accParent);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(winproc_GETOBJECT_CLIENT);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider", FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Nonclient", NULL, FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Main", NULL, TRUE);
    VariantClear(&v);

    SET_ACC_METHOD_EXPECT(&Accessible, get_accRole);
    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
    ok(V_I4(&v) == UIA_EditControlTypeId, "Unexpected I4 %#lx\n", V_I4(&v));
    VariantClear(&v);
    CHECK_ACC_METHOD_CALLED(&Accessible, get_accRole);

    UiaNodeRelease(node);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Accessible.ref == 1, "Unexpected refcnt %ld\n", Accessible.ref);

    /*
     * Unlike UiaProviderFromIAccessible which won't create a provider for an
     * MSAA proxy, the default clientside proxy provider will.
     */
    hwnd2 = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | ES_PASSWORD,
            0, 0, 100, 100, hwnd, NULL, NULL, NULL);
    initialize_provider(&Provider, ProviderOptions_ClientSideProvider, hwnd2, FALSE);

    /* Tries to get override provider from parent HWND. */
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    todo_wine CHECK_CALLED(winproc_GETOBJECT_UiaRoot);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), hwnd2);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider", FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Main", NULL, FALSE);
    check_node_provider_desc_todo(V_BSTR(&v), L"Annotation", NULL, TRUE);
    VariantClear(&v);

    hr = UiaGetPropertyValue(node, UIA_ControlTypePropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "Unexpected VT %d\n", V_VT(&v));
    ok(V_I4(&v) == UIA_EditControlTypeId, "Unexpected I4 %#lx\n", V_I4(&v));
    VariantClear(&v);

    hr = UiaGetPropertyValue(node, UIA_IsPasswordPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    ok(V_VT(&v) == VT_BOOL, "Unexpected VT %d\n", V_VT(&v));
    ok(check_variant_bool(&v, TRUE), "V_BOOL(&v) = %#x\n", V_BOOL(&v));
    VariantClear(&v);

    UiaNodeRelease(node);
    DestroyWindow(hwnd2);

    set_accessible_props(&Accessible, 0, 0, 0, NULL, 0, 0, 0, 0);
    acc_client = NULL;
    prov_root = child_win_prov_root = NULL;

    method_sequences_enabled = TRUE;
    DestroyWindow(hwnd);
    DestroyWindow(hwnd_child);
    UnregisterClassA("test_default_clientside_providers class", NULL);
    UnregisterClassA("test_default_clientside_providers child class", NULL);

    IUnknown_Release(unk_ns);
    CoUninitialize();
}

static void test_UiaGetRootNode(void)
{
    HUIANODE node;
    HRESULT hr;
    VARIANT v;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    UiaRegisterProviderCallback(test_uia_provider_callback);

    /*
     * UiaGetRootNode is the same as calling UiaNodeFromHandle with the
     * desktop window handle.
     */
    set_clientside_providers_for_hwnd(&Provider_proxy, &Provider_nc, &Provider_hwnd, GetDesktopWindow());
    base_hwnd_prov = &Provider_hwnd.IRawElementProviderSimple_iface;
    proxy_prov = &Provider_proxy.IRawElementProviderSimple_iface;
    nc_prov = &Provider_nc.IRawElementProviderSimple_iface;

    method_sequences_enabled = FALSE;
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_proxy);
    hr = UiaGetRootNode(&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!node, "Node == NULL.\n");
    ok(Provider_proxy.ref == 2, "Unexpected refcnt %ld\n", Provider_proxy.ref);
    ok(Provider_hwnd.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    ok(Provider_nc.ref == 2, "Unexpected refcnt %ld\n", Provider_nc.ref);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    CHECK_CALLED(prov_callback_proxy);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), GetDesktopWindow());
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider_proxy", TRUE);
    check_node_provider_desc(V_BSTR(&v), L"Hwnd", L"Provider_hwnd", FALSE);
    check_node_provider_desc(V_BSTR(&v), L"Nonclient", L"Provider_nc", FALSE);
    VariantClear(&v);

    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    ok(Provider_proxy.ref == 1, "Unexpected refcnt %ld\n", Provider_proxy.ref);
    ok(Provider_hwnd.ref == 1, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    ok(Provider_nc.ref == 1, "Unexpected refcnt %ld\n", Provider_nc.ref);

    initialize_provider(&Provider_hwnd, ProviderOptions_ClientSideProvider, NULL, TRUE);
    initialize_provider(&Provider_nc, ProviderOptions_ClientSideProvider | ProviderOptions_NonClientAreaProvider, NULL,
            TRUE);
    initialize_provider(&Provider_proxy, ProviderOptions_ClientSideProvider, NULL, TRUE);
    base_hwnd_prov = proxy_prov = nc_prov = NULL;

    method_sequences_enabled = TRUE;
    UiaRegisterProviderCallback(NULL);
    CoUninitialize();
}

#define test_node_from_focus( cache_req, exp_node_desc, proxy_cback_count, base_hwnd_cback_count, nc_cback_count, \
                              win_get_obj_count, child_win_get_obj_count, proxy_cback_todo, base_hwnd_cback_todo, \
                              nc_cback_todo, win_get_obj_todo, child_win_get_obj_todo) \
        test_node_from_focus_( (cache_req), (exp_node_desc), (proxy_cback_count), (base_hwnd_cback_count), (nc_cback_count), \
                               (win_get_obj_count), (child_win_get_obj_count), (proxy_cback_todo), (base_hwnd_cback_todo), \
                               (nc_cback_todo), (win_get_obj_todo), (child_win_get_obj_todo), __FILE__, __LINE__)
static void test_node_from_focus_(struct UiaCacheRequest *cache_req, struct node_provider_desc *exp_node_desc,
        int proxy_cback_count, int base_hwnd_cback_count, int nc_cback_count, int win_get_obj_count,
        int child_win_get_obj_count, BOOL proxy_cback_todo, BOOL base_hwnd_cback_todo, BOOL nc_cback_todo,
        BOOL win_get_obj_todo, BOOL child_win_get_obj_todo, const char *file, int line)
{
    const WCHAR *exp_tree_struct = exp_node_desc->prov_count ? L"P)" : L"";
    LONG exp_lbound[2], exp_elems[2];
    SAFEARRAY *out_req = NULL;
    BSTR tree_struct = NULL;
    HRESULT hr;

    SET_EXPECT_MULTI(prov_callback_base_hwnd, base_hwnd_cback_count);
    SET_EXPECT_MULTI(prov_callback_nonclient, nc_cback_count);
    SET_EXPECT_MULTI(prov_callback_proxy, proxy_cback_count);
    SET_EXPECT_MULTI(winproc_GETOBJECT_UiaRoot, win_get_obj_count);
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, child_win_get_obj_count);
    hr = UiaNodeFromFocus(cache_req, &out_req, &tree_struct);
    ok_(file, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (exp_node_desc->prov_count)
        ok_(file, line)(!!out_req, "out_req == NULL\n");
    else
        ok_(file, line)(!out_req, "out_req != NULL\n");
    ok_(file, line)(!!tree_struct, "tree_struct == NULL\n");
    todo_wine_if(base_hwnd_cback_todo) CHECK_CALLED_MULTI(prov_callback_base_hwnd, base_hwnd_cback_count);
    todo_wine_if(proxy_cback_todo) CHECK_CALLED_MULTI(prov_callback_proxy, proxy_cback_count);
    todo_wine_if(nc_cback_todo) CHECK_CALLED_MULTI(prov_callback_nonclient, nc_cback_count);
    todo_wine_if(win_get_obj_todo) CHECK_CALLED_MULTI(winproc_GETOBJECT_UiaRoot, win_get_obj_count);
    todo_wine_if(child_win_get_obj_todo) CHECK_CALLED_MULTI(child_winproc_GETOBJECT_UiaRoot, child_win_get_obj_count);

    ok_(file, line)(!wcscmp(tree_struct, exp_tree_struct), "unexpected tree structure %s\n", debugstr_w(tree_struct));
    if (exp_node_desc->prov_count)
    {
        exp_lbound[0] = exp_lbound[1] = 0;
        exp_elems[0] = 1;
        exp_elems[1] = 1 + cache_req->cProperties;
        test_cache_req_sa_(out_req, exp_lbound, exp_elems, exp_node_desc, file, line);
    }

    SafeArrayDestroy(out_req);
    SysFreeString(tree_struct);
}

static void test_UiaNodeFromFocus(void)
{
    struct Provider_prop_override prop_override;
    struct node_provider_desc exp_node_desc;
    struct UiaPropertyCondition prop_cond;
    struct UiaCacheRequest cache_req;
    struct UiaNotCondition not_cond;
    HWND hwnd, hwnd_child;
    int cache_prop;
    VARIANT v;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hwnd = create_test_hwnd("UiaNodeFromFocus class");
    hwnd_child = create_child_test_hwnd("UiaNodeFromFocus child class", hwnd);

    UiaRegisterProviderCallback(test_uia_provider_callback);

    /* Set clientside providers for our test windows and the desktop. */
    set_clientside_providers_for_hwnd(&Provider_proxy, &Provider_nc, &Provider_hwnd, GetDesktopWindow());
    base_hwnd_prov = &Provider_hwnd.IRawElementProviderSimple_iface;
    nc_prov = &Provider_nc.IRawElementProviderSimple_iface;
    proxy_prov = &Provider_proxy.IRawElementProviderSimple_iface;

    set_clientside_providers_for_hwnd(NULL, &Provider_nc2, &Provider_hwnd2, hwnd);
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, hwnd, TRUE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    prov_root = &Provider.IRawElementProviderSimple_iface;
    Provider.ignore_hwnd_prop = TRUE;

    set_clientside_providers_for_hwnd(NULL, &Provider_nc3, &Provider_hwnd3, hwnd_child);
    initialize_provider(&Provider2, ProviderOptions_ServerSideProvider, hwnd_child, TRUE);
    Provider2.frag_root = &Provider2.IRawElementProviderFragmentRoot_iface;
    child_win_prov_root = &Provider2.IRawElementProviderSimple_iface;
    Provider2.ignore_hwnd_prop = TRUE;

    /*
     * Nodes are normalized against the cache request view condition. Here,
     * we're setting it to the same as the default ControlView.
     */
    variant_init_bool(&v, FALSE);
    set_property_condition(&prop_cond, UIA_IsControlElementPropertyId, &v, PropertyConditionFlags_None);
    set_not_condition(&not_cond, (struct UiaCondition *)&prop_cond);
    cache_prop = UIA_RuntimeIdPropertyId;
    set_cache_request(&cache_req, (struct UiaCondition *)&not_cond, TreeScope_Element, &cache_prop, 1, NULL, 0,
            AutomationElementMode_Full);

    /*
     * None of the providers for the desktop node return a provider from
     * IRawElementProviderFragmentRoot::GetFocus, so we just get the
     * desktop node.
     */
    method_sequences_enabled = FALSE;
    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), GetDesktopWindow());
    add_provider_desc(&exp_node_desc, L"Main", L"Provider_proxy", TRUE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc", FALSE);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd", FALSE);

    test_node_from_focus(&cache_req, &exp_node_desc, 1, 1, 1, 0, 0, FALSE, FALSE, FALSE, FALSE, FALSE);

    /* Provider_hwnd returns Provider_hwnd2 from GetFocus. */
    Provider_hwnd.focus_prov = &Provider_hwnd2.IRawElementProviderFragment_iface;

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), hwnd);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider", TRUE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc2", FALSE);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd2", FALSE);

    test_node_from_focus(&cache_req, &exp_node_desc, 2, 1, 2, 1, 0, TRUE, FALSE, FALSE, FALSE, FALSE);

    /*
     * Provider_proxy returns Provider from GetFocus. The provider that
     * creates the node will not have GetFocus called on it to avoid returning
     * the same provider twice. Similarly, on nodes other than the desktop
     * node, the HWND provider will not have GetFocus called on it.
     */
    Provider_hwnd.focus_prov = NULL;
    Provider_proxy.focus_prov = &Provider.IRawElementProviderFragment_iface;
    Provider.focus_prov = Provider_hwnd2.focus_prov = &Provider2.IRawElementProviderFragment_iface;

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), hwnd);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider", TRUE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc2", FALSE);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd2", FALSE);

    test_node_from_focus(&cache_req, &exp_node_desc, 2, 2, 2, 1, 0, TRUE, FALSE, FALSE, FALSE, FALSE);

    /*
     * Provider_nc returns Provider_nc2 from GetFocus, Provider returns
     * Provider2, Provider_nc3 returns Provider_child.
     */
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    Provider_proxy.focus_prov = Provider_hwnd.focus_prov = Provider_hwnd2.focus_prov = NULL;
    Provider_nc.focus_prov = &Provider_nc2.IRawElementProviderFragment_iface;
    Provider.focus_prov = &Provider2.IRawElementProviderFragment_iface;
    Provider_nc3.focus_prov = &Provider_child.IRawElementProviderFragment_iface;

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider_child", TRUE);

    test_node_from_focus(&cache_req, &exp_node_desc, 2, 3, 2, 2, 1, TRUE, FALSE, FALSE, TRUE, FALSE);

    /*
     * Provider_proxy returns Provider_child_child from GetFocus. The focus
     * provider is normalized against the cache request view condition.
     * Provider_child_child and its ancestors don't match the cache request
     * view condition, so we'll get no provider.
     */
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    initialize_provider(&Provider_child_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    provider_add_child(&Provider, &Provider_child);
    provider_add_child(&Provider_child, &Provider_child_child);
    Provider_proxy.focus_prov = &Provider_child_child.IRawElementProviderFragment_iface;
    Provider_nc.focus_prov = Provider_hwnd.focus_prov = NULL;

    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsControlElementPropertyId, &v);
    set_provider_prop_override(&Provider_child_child, &prop_override, 1);
    set_provider_prop_override(&Provider_child, &prop_override, 1);
    set_provider_prop_override(&Provider, &prop_override, 1);

    init_node_provider_desc(&exp_node_desc, 0, NULL);
    test_node_from_focus(&cache_req, &exp_node_desc, 2, 2, 2, 1, 0, TRUE, FALSE, FALSE, FALSE, FALSE);

    /* This time, Provider_child matches our view condition. */
    set_provider_prop_override(&Provider_child, NULL, 0);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider_child", TRUE);

    test_node_from_focus(&cache_req, &exp_node_desc, 1, 1, 1, 0, 0, FALSE, FALSE, FALSE, FALSE, FALSE);

    method_sequences_enabled = TRUE;
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, TRUE);
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    initialize_provider(&Provider_child_child, ProviderOptions_ServerSideProvider, NULL, TRUE);

    CoUninitialize();
    UiaRegisterProviderCallback(NULL);
    DestroyWindow(hwnd);
    UnregisterClassA("UiaNodeFromFocus class", NULL);
    UnregisterClassA("UiaNodeFromFocus child class", NULL);
}

static const struct prov_method_sequence event_seq1[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win8+. */
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL }, /* Only called on Arabic version of Win10+. */
    { 0 },
};

static const struct prov_method_sequence event_seq2[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL }, /* Only done on Win10v1809+. */
    { &Provider, FRAG_GET_FRAGMENT_ROOT },
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL }, /* Called twice on Win11. */
    NODE_CREATE_SEQ_OPTIONAL(&Provider), /* Only done in Win11. */
    { &Provider, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL }, /* Only done on Win8+. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win10v1809+. */
    { &Provider, ADVISE_EVENTS_EVENT_ADDED },
    { 0 },
};

static const struct prov_method_sequence event_seq3[] = {
    { &Provider, PROV_GET_PROVIDER_OPTIONS },
    /* The following four methods are only called on Arabic versions of Win10+. */
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROPERTY_VALUE, METHOD_OPTIONAL }, /* UIA_NativeWindowHandlePropertyId */
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL },
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win8+. */
    NODE_CREATE_SEQ3(&Provider),
    { &Provider, FRAG_GET_RUNTIME_ID },
    { 0 },
};

static const struct prov_method_sequence event_seq4[] = {
    { &Provider, ADVISE_EVENTS_EVENT_REMOVED },
    { 0 },
};

static const struct prov_method_sequence event_seq5[] = {
    { &Provider, FRAG_GET_RUNTIME_ID },
    { &Provider, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL }, /* Only done on Win10v1809+. */
    { &Provider, FRAG_GET_FRAGMENT_ROOT },
    { &Provider, FRAG_GET_FRAGMENT_ROOT, METHOD_OPTIONAL }, /* Called twice on Win11. */
    { &Provider, FRAG_GET_EMBEDDED_FRAGMENT_ROOTS },
    NODE_CREATE_SEQ_OPTIONAL(&Provider), /* Only done in Win11. */
    { &Provider, FRAG_GET_RUNTIME_ID, METHOD_OPTIONAL }, /* Only done on Win8+. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL }, /* Only done on Win10v1809+. */
    { &Provider, ADVISE_EVENTS_EVENT_ADDED },
    { 0 },
};

static const struct prov_method_sequence event_seq6[] = {
    { &Provider, ADVISE_EVENTS_EVENT_REMOVED },
    { &Provider_child, ADVISE_EVENTS_EVENT_REMOVED },
    { &Provider_child2, ADVISE_EVENTS_EVENT_REMOVED },
    { 0 },
};

static struct EventData {
    LONG exp_lbound[2];
    LONG exp_elems[2];
    struct node_provider_desc exp_node_desc;
    const WCHAR *exp_tree_struct;

    struct node_provider_desc exp_nested_node_desc;
    HANDLE event_handle;
} EventData, EventData2;

static void set_event_data_struct(struct EventData *data, LONG exp_lbound0, LONG exp_lbound1, LONG exp_elems0,
        LONG exp_elems1, struct node_provider_desc *exp_node_desc, const WCHAR *exp_tree_struct)
{
    data->exp_lbound[0] = exp_lbound0;
    data->exp_lbound[1] = exp_lbound1;
    data->exp_elems[0] = exp_elems0;
    data->exp_elems[1] = exp_elems1;
    if (exp_node_desc)
    {
        int i;

        data->exp_node_desc = *exp_node_desc;
        for (i = 0; i < exp_node_desc->prov_count; i++)
        {
            if (exp_node_desc->nested_desc[i])
            {
                data->exp_nested_node_desc = *exp_node_desc->nested_desc[i];
                data->exp_node_desc.nested_desc[i] = &data->exp_nested_node_desc;
                break;
            }
        }
    }
    else
        memset(&data->exp_node_desc, 0, sizeof(data->exp_node_desc));
    data->exp_tree_struct = exp_tree_struct;
}

static void set_event_data(LONG exp_lbound0, LONG exp_lbound1, LONG exp_elems0, LONG exp_elems1,
        struct node_provider_desc *exp_node_desc, const WCHAR *exp_tree_struct)
{
    set_event_data_struct(&EventData, exp_lbound0, exp_lbound1, exp_elems0, exp_elems1, exp_node_desc,
            exp_tree_struct);
}

#define check_event_data( data, args, req_data, tree_struct ) \
        check_event_data_( (data), (args), (req_data), (tree_struct), __FILE__, __LINE__)
static void check_event_data_(struct EventData *data, struct UiaEventArgs *args, SAFEARRAY *req_data, BSTR tree_struct,
        const char *file, int line)
{
    if (!data->exp_elems[0] && !data->exp_elems[1])
        ok(!req_data, "req_data != NULL\n");
    else
        test_cache_req_sa_(req_data, data->exp_lbound, data->exp_elems, &data->exp_node_desc, file, line);

    ok(!wcscmp(tree_struct, data->exp_tree_struct), "tree structure %s\n", debugstr_w(tree_struct));
    if (data->event_handle)
        SetEvent(data->event_handle);
}

static void WINAPI uia_event_callback(struct UiaEventArgs *args, SAFEARRAY *req_data, BSTR tree_struct)
{
    CHECK_EXPECT(uia_event_callback);
    check_event_data(&EventData, args, req_data, tree_struct);
}

static void WINAPI uia_event_callback2(struct UiaEventArgs *args, SAFEARRAY *req_data, BSTR tree_struct)
{
    CHECK_EXPECT(uia_event_callback2);
    check_event_data(&EventData2, args, req_data, tree_struct);
}

enum {
    WM_UIA_TEST_RESET_EVENT_PROVIDERS = WM_APP,
    WM_UIA_TEST_SET_EVENT_PROVIDER_DATA,
    WM_UIA_TEST_RAISE_EVENT,
    WM_UIA_TEST_RAISE_EVENT_RT_ID,
    WM_UIA_TEST_CHECK_EVENT_ADVISE_ADDED,
    WM_UIA_TEST_CHECK_EVENT_ADVISE_REMOVED,
};

enum {
    PROVIDER_ID,
    PROVIDER2_ID,
    PROVIDER_CHILD_ID,
};

static struct Provider *event_test_provs[] = { &Provider, &Provider2, &Provider_child };
static struct Provider *get_event_test_prov(int idx)
{
    if (idx >= ARRAY_SIZE(event_test_provs))
        return NULL;
    else
        return event_test_provs[idx];
}

static void post_event_message(HWND hwnd, int msg, WPARAM wparam, int prov_id, int lparam_lower)
{
    PostMessageW(hwnd, msg, wparam, MAKELONG(lparam_lower, prov_id));
}

static void test_UiaAddEvent_client_proc(void)
{
    IRawElementProviderFragmentRoot *embedded_root = &Provider_hwnd2.IRawElementProviderFragmentRoot_iface;
    struct node_provider_desc exp_node_desc, exp_nested_node_desc;
    struct UiaCacheRequest cache_req;
    HUIAEVENT event;
    HUIANODE node;
    BOOL is_win11;
    HRESULT hr;
    DWORD pid;
    HWND hwnd;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hwnd = FindWindowA("UiaAddEvent test class", "Test window");
    UiaRegisterProviderCallback(test_uia_provider_callback);
    EventData.event_handle = CreateEventW(NULL, FALSE, FALSE, NULL);

    /* Provider_proxy/hwnd/nc are desktop providers. */
    set_clientside_providers_for_hwnd(&Provider_proxy, &Provider_nc, &Provider_hwnd, GetDesktopWindow());
    base_hwnd_prov = &Provider_hwnd.IRawElementProviderSimple_iface;
    proxy_prov = &Provider_proxy.IRawElementProviderSimple_iface;
    nc_prov = &Provider_nc.IRawElementProviderSimple_iface;

    /*
     * Provider_nc2/Provider_hwnd2 are clientside providers for our test
     * window.
     */
    set_clientside_providers_for_hwnd(NULL, &Provider_nc2, &Provider_hwnd2, hwnd);
    provider_add_child(&Provider_hwnd, &Provider_hwnd2);

    method_sequences_enabled = FALSE;
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_proxy);
    hr = UiaGetRootNode(&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!node, "Node == NULL.\n");
    ok(Provider_proxy.ref == 2, "Unexpected refcnt %ld\n", Provider_proxy.ref);
    ok(Provider_hwnd.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    ok(Provider_nc.ref == 2, "Unexpected refcnt %ld\n", Provider_nc.ref);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    CHECK_CALLED(prov_callback_proxy);

    PostMessageW(hwnd, WM_UIA_TEST_RESET_EVENT_PROVIDERS, 0, 0);

    /* Register an event on the desktop HWND with a scope of all elements. */
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    event = NULL;
    /* Only done on Win11. */
    SET_EXPECT_MULTI(prov_callback_base_hwnd, 2);
    SET_EXPECT_MULTI(prov_callback_nonclient, 2);
    SET_EXPECT_MULTI(prov_callback_proxy, 2);
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element | TreeScope_Descendants,
            NULL, 0, &cache_req, &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL\n");
    is_win11 = !!CALLED_COUNT(prov_callback_base_hwnd);
    CHECK_CALLED_AT_MOST(prov_callback_base_hwnd, 2);
    CHECK_CALLED_AT_MOST(prov_callback_nonclient, 2);
    CHECK_CALLED_AT_MOST(prov_callback_proxy, 2);
    method_sequences_enabled = TRUE;

    /*
     * Raise event in another process, prior to calling UiaEventAddWindow.
     * Event handler will not get triggered.
     */
    post_event_message(hwnd, WM_UIA_TEST_RAISE_EVENT, HandleToUlong(hwnd), PROVIDER_ID, ProviderOptions_ServerSideProvider);
    ok(WaitForSingleObject(EventData.event_handle, 300) == WAIT_TIMEOUT, "Wait for event_handle didn't timeout.\n");
    post_event_message(hwnd, WM_UIA_TEST_CHECK_EVENT_ADVISE_ADDED, 0, PROVIDER_ID, FALSE);

    /* Call UiaEventAddWindow, the event will now be connected. */
    SET_EXPECT_MULTI(prov_callback_base_hwnd, 2);
    SET_EXPECT_MULTI(prov_callback_nonclient, 2);
    SET_EXPECT_MULTI(prov_callback_proxy, 3);
    hr = UiaEventAddWindow(event, hwnd);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    todo_wine CHECK_CALLED(prov_callback_proxy);
    post_event_message(hwnd, WM_UIA_TEST_CHECK_EVENT_ADVISE_ADDED, UIA_AutomationFocusChangedEventId, PROVIDER_ID, FALSE);

    /* Successfully raise event. */
    GetWindowThreadProcessId(hwnd, &pid);
    init_node_provider_desc(&exp_nested_node_desc, pid, hwnd);
    add_provider_desc(&exp_nested_node_desc, L"Main", L"Provider", TRUE);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), hwnd);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd2", TRUE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc2", FALSE);
    add_nested_provider_desc(&exp_node_desc, L"Main", NULL, FALSE, &exp_nested_node_desc);
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");

    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_proxy);
    SET_EXPECT(uia_event_callback);
    post_event_message(hwnd, WM_UIA_TEST_RAISE_EVENT, HandleToUlong(hwnd), PROVIDER_ID, ProviderOptions_ServerSideProvider);
    ok(!WaitForSingleObject(EventData.event_handle, 2000), "Wait for event_handle failed.\n");
    CHECK_CALLED(uia_event_callback);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    todo_wine CHECK_CALLED(prov_callback_proxy);

    /*
     * If a clientside provider raises an event, it stays within its own
     * process.
     */
    post_event_message(hwnd, WM_UIA_TEST_RAISE_EVENT, 0, PROVIDER2_ID, ProviderOptions_ClientSideProvider);
    ok(WaitForSingleObject(EventData.event_handle, 300) == WAIT_TIMEOUT, "Wait for event_handle didn't timeout.\n");

    /* Raise serverside event. */
    GetWindowThreadProcessId(hwnd, &pid);
    init_node_provider_desc(&exp_node_desc, pid, NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider2", TRUE);
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");

    SET_EXPECT(uia_event_callback);
    post_event_message(hwnd, WM_UIA_TEST_RAISE_EVENT, 0, PROVIDER2_ID, ProviderOptions_ServerSideProvider);
    ok(!WaitForSingleObject(EventData.event_handle, 2000), "Wait for event_handle failed.\n");
    CHECK_CALLED(uia_event_callback);

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    post_event_message(hwnd, WM_UIA_TEST_CHECK_EVENT_ADVISE_REMOVED, UIA_AutomationFocusChangedEventId, PROVIDER_ID, FALSE);

    PostMessageW(hwnd, WM_UIA_TEST_RESET_EVENT_PROVIDERS, 0, 0);
    post_event_message(hwnd, WM_UIA_TEST_SET_EVENT_PROVIDER_DATA, HandleToUlong(hwnd), PROVIDER_ID,
            ProviderOptions_ServerSideProvider);

    /*
     * Register an event on the desktop node, except this time the scope
     * doesn't include the desktop node itself. In this case, navigation will
     * have to be done to confirm a provider that raised an event is a
     * descendant of the desktop, but not the desktop itself.
     *
     * No need for UiaEventAddWindow this time, because Provider_hwnd2 is an
     * embedded root. This method no longer works on Windows 11, which breaks
     * the managed UI Automation API. We match the old behavior instead.
     */
    Provider_hwnd.embedded_frag_roots = &embedded_root;
    Provider_hwnd.embedded_frag_roots_count = 1;
    Provider_hwnd2.frag_root = &Provider_hwnd2.IRawElementProviderFragmentRoot_iface;

    event = NULL;
    /* Only done on Win11. */
    SET_EXPECT_MULTI(prov_callback_base_hwnd, 2);
    SET_EXPECT_MULTI(prov_callback_nonclient, 3);
    SET_EXPECT_MULTI(prov_callback_proxy, 3);
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Descendants,
            NULL, 0, &cache_req, &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL\n");
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");
    CHECK_CALLED_AT_MOST(prov_callback_base_hwnd, 2);
    CHECK_CALLED_AT_MOST(prov_callback_nonclient, 3);
    CHECK_CALLED_AT_MOST(prov_callback_proxy, 3);

    if (is_win11)
    {
        post_event_message(hwnd, WM_UIA_TEST_CHECK_EVENT_ADVISE_ADDED, 0, PROVIDER_ID, FALSE);
        SET_EXPECT_MULTI(prov_callback_base_hwnd, 2);
        SET_EXPECT_MULTI(prov_callback_nonclient, 2);
        SET_EXPECT_MULTI(prov_callback_proxy, 3);
        hr = UiaEventAddWindow(event, hwnd);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        CHECK_CALLED_MULTI(prov_callback_base_hwnd, 2);
        CHECK_CALLED_MULTI(prov_callback_nonclient, 2);
        CHECK_CALLED_MULTI(prov_callback_proxy, 3);
    }

    post_event_message(hwnd, WM_UIA_TEST_CHECK_EVENT_ADVISE_ADDED, UIA_AutomationFocusChangedEventId, PROVIDER_ID, FALSE);

    /*
     * Starts navigation in server process, then completes navigation in the
     * client process.
     */
    GetWindowThreadProcessId(hwnd, &pid);
    init_node_provider_desc(&exp_node_desc, pid, NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider_child", TRUE);
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT_MULTI(prov_callback_nonclient, 2);
    SET_EXPECT_MULTI(prov_callback_proxy, 2);
    SET_EXPECT(uia_event_callback);

    post_event_message(hwnd, WM_UIA_TEST_RAISE_EVENT, 0, PROVIDER_CHILD_ID, ProviderOptions_ServerSideProvider);
    ok(!WaitForSingleObject(EventData.event_handle, 5000), "Wait for event_handle failed.\n");
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED_MULTI(prov_callback_nonclient, 2);
    todo_wine CHECK_CALLED_MULTI(prov_callback_proxy, 2);
    CHECK_CALLED(uia_event_callback);

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    post_event_message(hwnd, WM_UIA_TEST_CHECK_EVENT_ADVISE_REMOVED, UIA_AutomationFocusChangedEventId, PROVIDER_ID, FALSE);

    /*
     * Register an event on a node that won't require any navigation to reach.
     */
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, TRUE);
    Provider.runtime_id[0] = 0x1337;
    Provider.runtime_id[1] = 0xbeef;
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element,
            NULL, 0, &cache_req, &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL\n");
    ok(UiaNodeRelease(node), "UiaNodeRelease returned FALSE\n");

    SET_EXPECT_MULTI(prov_callback_base_hwnd, 2);
    SET_EXPECT_MULTI(prov_callback_nonclient, 2);
    SET_EXPECT_MULTI(prov_callback_proxy, 3);
    hr = UiaEventAddWindow(event, hwnd);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    todo_wine CHECK_CALLED(prov_callback_proxy);

    /* Wrong runtime ID, no match. */
    post_event_message(hwnd, WM_UIA_TEST_RAISE_EVENT_RT_ID, 0xb33f, PROVIDER2_ID, 0x1337);
    ok(WaitForSingleObject(EventData.event_handle, 500) == WAIT_TIMEOUT, "Wait for event_handle didn't timeout.\n");

    /* Successfully raise event. */
    GetWindowThreadProcessId(hwnd, &pid);
    init_node_provider_desc(&exp_node_desc, pid, NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider2", TRUE);
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");
    SET_EXPECT(uia_event_callback);
    post_event_message(hwnd, WM_UIA_TEST_RAISE_EVENT_RT_ID, 0xbeef, PROVIDER2_ID, 0x1337);
    ok(!WaitForSingleObject(EventData.event_handle, 2000), "Wait for event_handle failed.\n");
    CHECK_CALLED(uia_event_callback);

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CloseHandle(EventData.event_handle);
    CoUninitialize();
}

struct event_test_thread_data {
    HUIAEVENT event;
    DWORD exp_thread_id;
};

static DWORD WINAPI uia_add_event_test_thread(LPVOID param)
{
    struct event_test_thread_data *data = (struct event_test_thread_data *)param;
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = UiaRemoveEvent(data->event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider2.ref == 1, "Unexpected refcnt %ld\n", Provider2.ref);
    ok(Provider2.last_call_tid == data->exp_thread_id ||
            broken(Provider2.last_call_tid == GetCurrentThreadId()), "Expected method call on separate thread\n");
    ok(Provider2.advise_events_removed_event_id == UIA_AutomationFocusChangedEventId,
            "Unexpected advise event removed, event ID %d\n", Provider.advise_events_removed_event_id);

    CoUninitialize();
    return 0;
}

static void test_UiaAddEvent_args(HUIANODE node)
{
    struct UiaCacheRequest cache_req;
    HUIAEVENT event;
    HRESULT hr;

    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);

    /* NULL node. */
    hr = UiaAddEvent(NULL, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element, NULL, 0, &cache_req,
            &event);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* NULL event callback. */
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, NULL, TreeScope_Element, NULL, 0, &cache_req,
            &event);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* NULL cache request. */
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element, NULL, 0, NULL,
            &event);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* NULL event handle. */
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element, NULL, 0, &cache_req,
            NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Event IDs aren't checked for validity, 1 is not a valid UIA event ID. */
    event = NULL;
    hr = UiaAddEvent(node, 1, uia_event_callback, TreeScope_Element, NULL, 0, &cache_req, &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL\n");

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}

static void test_UiaRemoveEvent_args(HUIANODE node)
{
    HRESULT hr;

    hr = UiaRemoveEvent(NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = UiaRemoveEvent((HUIAEVENT)node);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
}

static void test_UiaRaiseAutomationEvent_args(void)
{
    HRESULT hr;

    hr = UiaRaiseAutomationEvent(NULL, UIA_AutomationFocusChangedEventId);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Returns failure code from get_ProviderOptions. */
    initialize_provider(&Provider2, 0, NULL, TRUE);
    hr = UiaRaiseAutomationEvent(&Provider2.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    /* Windows 7 returns S_OK. */
    ok(hr == E_NOTIMPL || broken(hr == S_OK), "Unexpected hr %#lx.\n", hr);

    /* Invalid event ID - doesn't return failure code. */
    initialize_provider(&Provider2, ProviderOptions_ServerSideProvider, NULL, TRUE);
    hr = UiaRaiseAutomationEvent(&Provider2.IRawElementProviderSimple_iface, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /*
     * UIA_StructureChangedEventId should use UiaRaiseStructureChangedEvent.
     * No failure code is returned, however.
     */
    hr = UiaRaiseAutomationEvent(&Provider2.IRawElementProviderSimple_iface, UIA_StructureChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}

static void test_UiaAddEvent(const char *name)
{
    IRawElementProviderFragmentRoot *embedded_roots[2] = { &Provider_child.IRawElementProviderFragmentRoot_iface,
                                                           &Provider_child2.IRawElementProviderFragmentRoot_iface };
    struct event_test_thread_data thread_data = { 0 };
    struct Provider_prop_override prop_override;
    struct node_provider_desc exp_node_desc;
    struct UiaPropertyCondition prop_cond;
    struct UiaCacheRequest cache_req;
    PROCESS_INFORMATION proc;
    char cmdline[MAX_PATH];
    STARTUPINFOA startup;
    HUIAEVENT event;
    DWORD exit_code;
    HUIANODE node;
    HANDLE thread;
    HRESULT hr;
    HWND hwnd;
    VARIANT v;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, TRUE);
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    provider_add_child(&Provider, &Provider_child);
    initialize_provider(&Provider_child_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    provider_add_child(&Provider_child, &Provider_child_child);

    /* Create a node with Provider as its only provider. */
    method_sequences_enabled = FALSE;
    hr = UiaNodeFromProvider(&Provider.IRawElementProviderSimple_iface, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    hr = UiaGetPropertyValue(node, UIA_ProviderDescriptionPropertyId, &v);
    ok(hr == S_OK, "Unexpected hr %#lx\n", hr);
    check_node_provider_desc_prefix(V_BSTR(&v), GetCurrentProcessId(), NULL);
    check_node_provider_desc(V_BSTR(&v), L"Main", L"Provider", TRUE);
    VariantClear(&v);

    /* Test valid function input arguments. */
    test_UiaAddEvent_args(node);
    test_UiaRemoveEvent_args(node);
    test_UiaRaiseAutomationEvent_args();

    /*
     * Raise event without any registered event handlers.
     */
    method_sequences_enabled = TRUE;
    hr = UiaRaiseAutomationEvent(&Provider.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_method_sequence(event_seq1, "event_seq1");

    /*
     * Register an event on a node without an HWND/RuntimeId. The event will
     * be created successfully, but without any way to match a provider to
     * this node, we won't be able to trigger the event handler.
     */
    event = NULL;
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element, NULL, 0, &cache_req,
            &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL\n");
    ok(Provider.ref == 3, "Unexpected refcnt %ld\n", Provider.ref);
    ok_method_sequence(event_seq2, "event_seq2");

    /*
     * Even though we raise an event on the same provider as the one our node
     * currently represents, without an HWND/RuntimeId, we have no way to
     * match them. The event handler will not be called.
     */
    hr = UiaRaiseAutomationEvent(&Provider.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok_method_sequence(event_seq3, "event_seq3");

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok_method_sequence(event_seq4, "event_seq4");

    /*
     * Register an event on the same node again, except this time we have a
     * runtimeID. Nodes returned to the event handler are normalized against
     * the cache request view condition.
     */
    event = NULL;
    variant_init_bool(&v, TRUE);
    set_property_condition(&prop_cond, UIA_IsControlElementPropertyId, &v, PropertyConditionFlags_None);
    set_cache_request(&cache_req, (struct UiaCondition *)&prop_cond, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);

    Provider.runtime_id[0] = Provider.runtime_id[1] = 0xdeadbeef;
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element, NULL, 0, &cache_req,
            &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL\n");
    ok(Provider.ref == 3, "Unexpected refcnt %ld\n", Provider.ref);
    ok_method_sequence(event_seq2, "event_seq2");

    /* Event callback is now invoked since we can match by runtime ID. */
    method_sequences_enabled = FALSE;
    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider", TRUE);
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");
    SET_EXPECT(uia_event_callback);
    hr = UiaRaiseAutomationEvent(&Provider.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(uia_event_callback);

    /*
     * Runtime ID matches so event callback is invoked, but nothing matches
     * the cache request view condition, so the callback will be passed a
     * NULL cache request.
     */
    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsControlElementPropertyId, &v);
    set_provider_prop_override(&Provider, &prop_override, 1);

    set_event_data(0, 0, 0, 0, &exp_node_desc, L"");
    SET_EXPECT(uia_event_callback);
    hr = UiaRaiseAutomationEvent(&Provider.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(uia_event_callback);
    set_provider_prop_override(&Provider, NULL, 0);

    method_sequences_enabled = TRUE;
    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok_method_sequence(event_seq4, "event_seq4");

    /* Create an event with TreeScope_Children. */
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Children, NULL, 0, &cache_req,
            &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL\n");
    ok(Provider.ref == 3, "Unexpected refcnt %ld\n", Provider.ref);
    ok_method_sequence(event_seq5, "event_seq5");

    /*
     * Only TreeScope_Children and not TreeScope_Element, handler won't be
     * called.
     */
    method_sequences_enabled = FALSE;
    hr = UiaRaiseAutomationEvent(&Provider.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Provider_child_child is not a direct child, handler won't be called. */
    hr = UiaRaiseAutomationEvent(&Provider_child_child.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Raised an event on Provider_child, handler will be called. */
    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider_child", TRUE);
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");
    SET_EXPECT(uia_event_callback);
    hr = UiaRaiseAutomationEvent(&Provider_child.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(uia_event_callback);

    /* Provider_child doesn't match the view condition, but Provider does. */
    variant_init_bool(&v, FALSE);
    set_property_override(&prop_override, UIA_IsControlElementPropertyId, &v);
    set_provider_prop_override(&Provider_child, &prop_override, 1);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider", TRUE);
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");
    SET_EXPECT(uia_event_callback);
    hr = UiaRaiseAutomationEvent(&Provider_child.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(uia_event_callback);
    set_provider_prop_override(&Provider_child, NULL, 0);

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    /* Create an event with TreeScope_Descendants. */
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element | TreeScope_Descendants, NULL, 0,
            &cache_req, &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL\n");
    ok(Provider.ref == 3, "Unexpected refcnt %ld\n", Provider.ref);

    /* Raised an event on Provider_child_child. */
    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider_child_child", TRUE);
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");
    SET_EXPECT(uia_event_callback);
    hr = UiaRaiseAutomationEvent(&Provider_child_child.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(uia_event_callback);

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);

    CoUninitialize();

    /*
     * When adding an event, each node provider's fragment root is queried
     * for, and if one is retrieved, it's IRawElementProviderAdviseEvents
     * interface is used. On Win10v1809+, ProviderOptions_UseComThreading is
     * respected for these interfaces. Set Provider2 as the fragment root here
     * to test this.
     */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, FALSE);
    initialize_provider(&Provider2, ProviderOptions_UseComThreading | ProviderOptions_ServerSideProvider, NULL, TRUE);
    Provider.frag_root = &Provider2.IRawElementProviderFragmentRoot_iface;
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element | TreeScope_Descendants, NULL, 0,
            &cache_req, &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL\n");
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider2.ref > 1, "Unexpected refcnt %ld\n", Provider2.ref);
    ok(!Provider.advise_events_added_event_id, "Unexpected advise event added, event ID %d\n", Provider.advise_events_added_event_id);
    ok(Provider2.advise_events_added_event_id == UIA_AutomationFocusChangedEventId,
            "Unexpected advise event added, event ID %d\n", Provider.advise_events_added_event_id);

    thread_data.exp_thread_id = GetCurrentThreadId();
    thread_data.event = event;
    thread = CreateThread(NULL, 0, uia_add_event_test_thread, (void *)&thread_data, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;

        while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);

    /* Test retrieving AdviseEvents on embedded fragment roots. */
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, FALSE);
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, FALSE);
    initialize_provider(&Provider_child2, ProviderOptions_ServerSideProvider, NULL, FALSE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    Provider.embedded_frag_roots = embedded_roots;
    Provider.embedded_frag_roots_count = ARRAY_SIZE(embedded_roots);
    Provider_child.frag_root = &Provider_child.IRawElementProviderFragmentRoot_iface;
    Provider_child2.frag_root = &Provider_child2.IRawElementProviderFragmentRoot_iface;

    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element | TreeScope_Descendants, NULL, 0,
            &cache_req, &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL\n");
    ok(Provider.ref == 3, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider_child.ref == 2, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(Provider_child2.ref == 2, "Unexpected refcnt %ld\n", Provider_child2.ref);
    ok(Provider.advise_events_added_event_id == UIA_AutomationFocusChangedEventId,
            "Unexpected advise event added, event ID %d\n", Provider.advise_events_added_event_id);
    ok(Provider_child.advise_events_added_event_id == UIA_AutomationFocusChangedEventId,
            "Unexpected advise event added, event ID %d\n", Provider_child.advise_events_added_event_id);
    ok(Provider_child2.advise_events_added_event_id == UIA_AutomationFocusChangedEventId,
            "Unexpected advise event added, event ID %d\n", Provider_child2.advise_events_added_event_id);

    method_sequences_enabled = TRUE;
    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(Provider.ref == 2, "Unexpected refcnt %ld\n", Provider.ref);
    ok(Provider_child.ref == 1, "Unexpected refcnt %ld\n", Provider_child.ref);
    ok(Provider_child2.ref == 1, "Unexpected refcnt %ld\n", Provider_child2.ref);
    ok_method_sequence(event_seq6, "event_seq6");

    UiaNodeRelease(node);
    ok(Provider.ref == 1, "Unexpected refcnt %ld\n", Provider.ref);
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, NULL, TRUE);
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    initialize_provider(&Provider_child2, ProviderOptions_ServerSideProvider, NULL, TRUE);

    hwnd = create_test_hwnd("UiaAddEvent test class");
    UiaRegisterProviderCallback(test_uia_provider_callback);

    /* Set clientside providers for our test window and the desktop. */
    set_clientside_providers_for_hwnd(&Provider_proxy, &Provider_nc, &Provider_hwnd, GetDesktopWindow());
    Provider_proxy.frag_root = Provider_hwnd.frag_root = Provider_nc.frag_root = NULL;
    base_hwnd_prov = &Provider_hwnd.IRawElementProviderSimple_iface;
    nc_prov = &Provider_nc.IRawElementProviderSimple_iface;
    proxy_prov = &Provider_proxy.IRawElementProviderSimple_iface;

    set_clientside_providers_for_hwnd(NULL, &Provider_nc2, &Provider_hwnd2, hwnd);
    provider_add_child(&Provider_nc, &Provider_nc2);
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, hwnd, TRUE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    prov_root = &Provider.IRawElementProviderSimple_iface;
    Provider.ignore_hwnd_prop = TRUE;

    method_sequences_enabled = FALSE;
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT(prov_callback_proxy);
    hr = UiaGetRootNode(&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!node, "Node == NULL.\n");
    ok(Provider_proxy.ref == 2, "Unexpected refcnt %ld\n", Provider_proxy.ref);
    ok(Provider_hwnd.ref == 2, "Unexpected refcnt %ld\n", Provider_hwnd.ref);
    ok(Provider_nc.ref == 2, "Unexpected refcnt %ld\n", Provider_nc.ref);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    CHECK_CALLED(prov_callback_proxy);

    /* Register an event on the desktop HWND. */
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element | TreeScope_Children, NULL, 0,
            &cache_req, &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL\n");

    /*
     * Raising an event on a serverside provider results in no clientside
     * providers being added for the HWND, which means we won't match this
     * event.
     */
    Provider_hwnd2.prov_opts = ProviderOptions_ServerSideProvider;
    hr = UiaRaiseAutomationEvent(&Provider_hwnd2.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Now that Provider has clientside providers, the event will be matched. */
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    SET_EXPECT(prov_callback_base_hwnd);
    SET_EXPECT(prov_callback_nonclient);
    SET_EXPECT_MULTI(prov_callback_proxy, 2);
    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), hwnd);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider", FALSE);
    add_provider_desc(&exp_node_desc, L"Nonclient", L"Provider_nc2", TRUE);
    add_provider_desc(&exp_node_desc, L"Hwnd", L"Provider_hwnd2", FALSE);
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");
    SET_EXPECT(uia_event_callback);
    Provider_hwnd2.prov_opts = ProviderOptions_ClientSideProvider;
    hr = UiaRaiseAutomationEvent(&Provider_hwnd2.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED(prov_callback_base_hwnd);
    CHECK_CALLED(prov_callback_nonclient);
    todo_wine CHECK_CALLED_MULTI(prov_callback_proxy, 2);
    CHECK_CALLED(uia_event_callback);

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /*
     * Register an event on the desktop HWND with a scope of TreeScope_Element
     * and TreeScope_Descendants. This is a special case where all providers
     * will match, regardless of whether or not they can navigate to the
     * desktop node.
     */
    set_cache_request(&cache_req, (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, NULL, 0, NULL, 0,
            AutomationElementMode_Full);
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element | TreeScope_Descendants, NULL, 0,
            &cache_req, &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!event, "event == NULL\n");

    /*
     * Raise an event on Provider2 - completely disconnected from all other
     * providers, will still trigger the event callback.
     */
    initialize_provider(&Provider2, ProviderOptions_ServerSideProvider, NULL, TRUE);
    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider2", TRUE);
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");
    SET_EXPECT(uia_event_callback);
    hr = UiaRaiseAutomationEvent(&Provider2.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(uia_event_callback);

    /*
     * No clientside providers to match us to the desktop node through
     * navigation, but event will still be triggered.
     */
    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), hwnd);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider_hwnd2", TRUE);
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");
    SET_EXPECT(uia_event_callback);
    Provider_hwnd2.prov_opts = ProviderOptions_ServerSideProvider;
    Provider_hwnd2.ignore_hwnd_prop = TRUE;
    hr = UiaRaiseAutomationEvent(&Provider_hwnd2.IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_CALLED(uia_event_callback);

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    UiaNodeRelease(node);
    CoUninitialize();

    /* Cross process event tests. */
    UiaRegisterProviderCallback(NULL);
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, hwnd, TRUE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    provider_add_child(&Provider, &Provider_child);

    prov_root = &Provider.IRawElementProviderSimple_iface;
    sprintf(cmdline, "\"%s\" uiautomation UiaAddEvent_client_proc", name);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    SET_EXPECT_MULTI(winproc_GETOBJECT_UiaRoot, 22);
    /* Only sent on Win7. */
    SET_EXPECT(winproc_GETOBJECT_CLIENT);

    CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &proc);
    while (MsgWaitForMultipleObjects(1, &proc.hProcess, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        struct Provider *prov;
        MSG msg = { 0 };

        while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            switch (msg.message)
            {
            case WM_UIA_TEST_RESET_EVENT_PROVIDERS:
            {
                int i;

                for (i = 0; i < ARRAY_SIZE(event_test_provs); i++)
                    initialize_provider(event_test_provs[i], ProviderOptions_ServerSideProvider, NULL, FALSE);
                break;
            }

            case WM_UIA_TEST_CHECK_EVENT_ADVISE_ADDED:
            case WM_UIA_TEST_CHECK_EVENT_ADVISE_REMOVED:
                if (!(prov = get_event_test_prov(HIWORD(msg.lParam))))
                    break;

                if (msg.message == WM_UIA_TEST_CHECK_EVENT_ADVISE_ADDED)
                    todo_wine_if(LOWORD(msg.lParam)) ok(prov->advise_events_added_event_id == msg.wParam,
                            "Unexpected advise event added, event ID %d\n", Provider.advise_events_added_event_id);
                else
                    todo_wine_if(LOWORD(msg.lParam)) ok(prov->advise_events_removed_event_id == msg.wParam,
                            "Unexpected advise event removed, event ID %d\n", Provider.advise_events_removed_event_id);
                break;

            case WM_UIA_TEST_RAISE_EVENT:
            case WM_UIA_TEST_RAISE_EVENT_RT_ID:
            case WM_UIA_TEST_SET_EVENT_PROVIDER_DATA:
                if (!(prov = get_event_test_prov(HIWORD(msg.lParam))))
                    break;

                if ((msg.message == WM_UIA_TEST_RAISE_EVENT) || (msg.message == WM_UIA_TEST_SET_EVENT_PROVIDER_DATA))
                {
                    prov->prov_opts = LOWORD(msg.lParam);
                    prov->hwnd = UlongToHandle(msg.wParam);
                    prov->ignore_hwnd_prop = !!prov->hwnd;
                }
                else if (msg.message == WM_UIA_TEST_RAISE_EVENT_RT_ID)
                {
                    prov->runtime_id[0] = LOWORD(msg.lParam);
                    prov->runtime_id[1] = msg.wParam;
                }

                if (msg.message != WM_UIA_TEST_SET_EVENT_PROVIDER_DATA)
                {
                    hr = UiaRaiseAutomationEvent(&prov->IRawElementProviderSimple_iface, UIA_AutomationFocusChangedEventId);
                    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                }
                break;

            default:
                prov = NULL;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    CHECK_CALLED_AT_LEAST(winproc_GETOBJECT_UiaRoot, 5);
    GetExitCodeProcess(proc.hProcess, &exit_code);
    if (exit_code > 255)
        ok(0, "unhandled exception %08x in child process %04x\n", (UINT)exit_code, (UINT)GetProcessId(proc.hProcess));
    else if (exit_code)
        ok(0, "%u failures in child process\n", (UINT)exit_code);
    CloseHandle(proc.hProcess);

    method_sequences_enabled = TRUE;
    CoUninitialize();
    DestroyWindow(hwnd);
    UnregisterClassA("UiaAddEvent test class", NULL);
}

static const struct prov_method_sequence serverside_prov_seq[] = {
    NODE_CREATE_SEQ2(&Provider),
    /* Windows 10+ calls this. */
    { &Provider, PROV_GET_PROVIDER_OPTIONS, METHOD_OPTIONAL },
    { 0 }
};

static void test_UiaHasServerSideProvider(void)
{
    BOOL ret_val;
    HWND hwnd;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hwnd = create_test_hwnd("UiaHasServerSideProvider test class");

    /* NULL hwnd. */
    ret_val = UiaHasServerSideProvider(NULL);
    ok(!ret_val, "UiaHasServerSideProvider returned TRUE\n");

    /* Desktop window has no serverside providers. */
    UiaRegisterProviderCallback(test_uia_provider_callback);
    ret_val = UiaHasServerSideProvider(GetDesktopWindow());
    ok(!ret_val, "UiaHasServerSideProvider returned TRUE\n");

    /* No provider to pass to UiaReturnRawElementProvider, returns FALSE. */
    prov_root = NULL;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    ret_val = UiaHasServerSideProvider(hwnd);
    ok(!ret_val, "UiaHasServerSideProvider returned TRUE\n");
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);

    /*
     * Provider passed to UiaReturnRawElementProvider returns a failure from
     * get_ProviderOptions. Returns FALSE.
     */
    initialize_provider(&Provider, 0, hwnd, TRUE);
    prov_root = &Provider.IRawElementProviderSimple_iface;
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    ret_val = UiaHasServerSideProvider(hwnd);
    ok(!ret_val, "UiaHasServerSideProvider returned TRUE\n");
    ok_method_sequence(node_from_hwnd1, NULL);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);

    /* Successfully return a provider from UiaReturnRawElementProvider. */
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, hwnd, TRUE);
    SET_EXPECT(winproc_GETOBJECT_UiaRoot);
    ret_val = UiaHasServerSideProvider(hwnd);
    ok(ret_val, "UiaHasServerSideProvider returned FALSE\n");
    ok_method_sequence(serverside_prov_seq, "serverside_prov_seq");
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);

    UiaRegisterProviderCallback(NULL);
    prov_root = NULL;
    CoUninitialize();
    DestroyWindow(hwnd);
    UnregisterClassA("UiaHasServerSideProvider test class", NULL);
}

static const struct prov_method_sequence win_event_handler_seq[] = {
    { &Provider_proxy, HWND_OVERRIDE_GET_OVERRIDE_PROVIDER, METHOD_TODO },
    { &Provider_hwnd2, PROV_GET_HOST_RAW_ELEMENT_PROVIDER, METHOD_OPTIONAL }, /* Only done on Win10v1809+. */
    { &Provider_nc2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_hwnd2, FRAG_NAVIGATE }, /* NavigateDirection_Parent */
    { &Provider_nc2, WINEVENT_HANDLER_RESPOND_TO_WINEVENT },
    { &Provider_hwnd2, WINEVENT_HANDLER_RESPOND_TO_WINEVENT },
    NODE_CREATE_SEQ(&Provider_child),
    { &Provider_child, FRAG_GET_RUNTIME_ID },
    { &Provider_child, PROV_GET_PROPERTY_VALUE }, /* UIA_ProviderDescriptionPropertyId */
    { 0 }
};

#define test_uia_event_win_event_mapping( win_event, hwnd, obj_id, child_id, event_handles, event_handle_count, \
                                         expect_event1, expect_event2, todo ) \
        test_uia_event_win_event_mapping_( (win_event), (hwnd), (obj_id), (child_id), (event_handles), (event_handle_count), \
                                                 (expect_event1), (expect_event2), (todo), __FILE__, __LINE__)
static void test_uia_event_win_event_mapping_(DWORD win_event, HWND hwnd, LONG obj_id, LONG child_id,
        HANDLE *event_handles, int event_handle_count, BOOL expect_event1, BOOL expect_event2,
        BOOL todo, const char *file, int line)
{
    const BOOL exp_timeout = (!expect_event1 && !expect_event2);
    DWORD timeout_val = exp_timeout ? 500 : 3000;
    DWORD wait_res;

    SET_EXPECT_MULTI(uia_event_callback, !!expect_event1);
    SET_EXPECT_MULTI(uia_event_callback2, !!expect_event2);
    if (expect_event2)
        SET_EXPECT(uia_event_callback2);
    NotifyWinEvent(win_event, hwnd, obj_id, child_id);

    wait_res = msg_wait_for_all_events(event_handles, event_handle_count, timeout_val);
    todo_wine_if(todo) ok_(file, line)((wait_res == WAIT_TIMEOUT) == exp_timeout,
            "Unexpected result while waiting for event callback(s).\n");
    if (expect_event1)
        todo_wine_if(todo) CHECK_CALLED(uia_event_callback);
    if (expect_event2)
        todo_wine_if(todo) CHECK_CALLED(uia_event_callback2);
}

static DWORD WINAPI uia_proxy_provider_win_event_handler_test_thread(LPVOID param)
{
    struct UiaCacheRequest cache_req = { (struct UiaCondition *)&UiaTrueCondition, TreeScope_Element, NULL, 0, NULL, 0,
                                         AutomationElementMode_Full };
    HWND hwnd[2] = { ((HWND *)param)[0], ((HWND *)param)[1] };
    struct node_provider_desc exp_node_desc;
    HWND tmp_hwnd, tmp_hwnd2;
    HUIAEVENT event, event2;
    HANDLE event_handles[2];
    HUIANODE node;
    HRESULT hr;
    int i;

    method_sequences_enabled = FALSE;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    for (i = 0; i < ARRAY_SIZE(event_handles); i++)
        event_handles[i] = CreateEventW(NULL, FALSE, FALSE, NULL);

    set_uia_hwnd_expects(1, 1, 1, 2, 0);
    hr = UiaNodeFromHandle(hwnd[0], &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!node, "Node == NULL.\n");
    check_uia_hwnd_expects_at_most(1, 1, 1, 2, 0);

    set_uia_hwnd_expects(2, 2, 2, 4, 0);
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element, NULL, 0, &cache_req,
            &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* Windows 11 recreates HWND clientside providers for the node passed into UiaAddEvent. */
    check_uia_hwnd_expects_at_most(2, 2, 2, 4, 0);

    set_uia_hwnd_expects(2, 2, 2, 4, 0);
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback2, TreeScope_Subtree, NULL, 0, &cache_req,
            &event2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* Windows 11 recreates HWND clientside providers for the node passed into UiaAddEvent. */
    check_uia_hwnd_expects_at_most(2, 2, 2, 4, 0);
    UiaNodeRelease(node);

    /*
     * Raise EVENT_OBJECT_FOCUS. If none of our clientside providers returned
     * an IProxyProviderWinEventHandler interface when being advised of events
     * being listened for, nothing happens.
     */
    EventData.event_handle = event_handles[0];
    EventData2.event_handle = event_handles[1];
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, hwnd[0], OBJID_WINDOW, CHILDID_SELF, event_handles,
            ARRAY_SIZE(event_handles), FALSE, FALSE, FALSE);

    /*
     * Return an IProxyProviderWinEventHandler interface on our clientside
     * providers. WinEvents will now be listened for. If a provider returns a
     * WinEvent handler interface, IRawElementProviderAdviseEvents will not be
     * queried for or used.
     */
    initialize_provider_advise_events_ids(&Provider_hwnd2);
    initialize_provider_advise_events_ids(&Provider_nc2);
    Provider_hwnd2.win_event_handler_data.is_supported = Provider_nc2.win_event_handler_data.is_supported = TRUE;
    set_uia_hwnd_expects(1, 1, 1, 2, 0);
    hr = UiaEventAddWindow(event, hwnd[0]);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    test_provider_event_advise_added(&Provider_hwnd2, 0, FALSE);
    test_provider_event_advise_added(&Provider_nc2, 0, FALSE);
    check_uia_hwnd_expects_at_least(1, TRUE, 1, FALSE, 1, FALSE, 1, FALSE, 0, FALSE);

    /*
     * WinEvents will now be listened for, however if our HWND has a
     * serverside provider they will be ignored.
     */
    SET_EXPECT_MULTI(winproc_GETOBJECT_UiaRoot, 2); /* Only called twice on Win11. */
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, hwnd[0], OBJID_WINDOW, CHILDID_SELF, event_handles,
            1, FALSE, FALSE, FALSE);
    CHECK_CALLED(winproc_GETOBJECT_UiaRoot);

    /*
     * Get rid of our serverside provider and raise EVENT_OBJECT_FOCUS
     * again. Now, our WinEvent handler interfaces will be invoked.
     */
    prov_root = NULL;
    set_provider_win_event_handler_win_event_expects(&Provider_hwnd2, EVENT_OBJECT_FOCUS, hwnd[0], OBJID_WINDOW, CHILDID_SELF);
    set_provider_win_event_handler_win_event_expects(&Provider_nc2, EVENT_OBJECT_FOCUS, hwnd[0], OBJID_WINDOW, CHILDID_SELF);

    initialize_provider(&Provider_child, ProviderOptions_ServerSideProvider, NULL, TRUE);
    set_provider_runtime_id(&Provider_child, UIA_RUNTIME_ID_PREFIX, HandleToUlong(hwnd[0]));
    set_provider_win_event_handler_respond_prov(&Provider_hwnd2, &Provider_child.IRawElementProviderSimple_iface,
            UIA_AutomationFocusChangedEventId);

    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", L"Provider_child", TRUE);
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");

    method_sequences_enabled = TRUE;
    set_uia_hwnd_expects(1, 1, 1, 4, 3);
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, hwnd[0], OBJID_WINDOW, CHILDID_SELF, event_handles,
            1, TRUE, FALSE, FALSE);
    ok_method_sequence(win_event_handler_seq, "win_event_handler_seq");
    check_uia_hwnd_expects_at_least(1, TRUE, 1, FALSE, 1, FALSE, 1, FALSE, 1, FALSE);
    method_sequences_enabled = FALSE;

    /*
     * Not all WinEvents are passed to our WinEvent responder interface -
     * they're filtered by HWND.
     */
    Provider_hwnd.win_event_handler_data.is_supported = Provider_nc.win_event_handler_data.is_supported = TRUE;
    set_provider_win_event_handler_win_event_expects(&Provider_nc, EVENT_OBJECT_FOCUS, GetDesktopWindow(), OBJID_WINDOW, CHILDID_SELF);
    set_provider_win_event_handler_respond_prov(&Provider_nc, &Provider_child.IRawElementProviderSimple_iface,
            UIA_AutomationFocusChangedEventId);
    SET_EXPECT(uia_event_callback);
    set_uia_hwnd_expects(0, 1, 1, 0, 0);
    NotifyWinEvent(EVENT_OBJECT_FOCUS, GetDesktopWindow(), OBJID_WINDOW, CHILDID_SELF);
    if (msg_wait_for_all_events(event_handles, 1, 3000) == WAIT_OBJECT_0)
    {
        win_skip("Win10v1507 and below don't filter WinEvents by HWND, skipping further tests.\n");

        CHECK_CALLED(uia_event_callback);
        check_uia_hwnd_expects(0, FALSE, 1, FALSE, 1, FALSE, 0, FALSE, 0, FALSE);
        hr = UiaRemoveEvent(event);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = UiaRemoveEvent(event2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        goto skip_win_event_hwnd_filter_test;
    }

    /* Clear expects/called values. */
    CHECK_CALLED_MULTI(uia_event_callback, 0);

    /*
     * Child HWNDs of top level HWNDs that are within our scope are listened
     * to by default.
     */
    child_win_prov_root = NULL;
    Provider_hwnd3.win_event_handler_data.is_supported = Provider_nc3.win_event_handler_data.is_supported = TRUE;
    set_provider_win_event_handler_win_event_expects(&Provider_nc3, EVENT_OBJECT_FOCUS, hwnd[1], OBJID_WINDOW, CHILDID_SELF);
    set_provider_win_event_handler_win_event_expects(&Provider_hwnd3, EVENT_OBJECT_FOCUS, hwnd[1], OBJID_WINDOW, CHILDID_SELF);
    set_provider_win_event_handler_respond_prov(&Provider_nc3, &Provider_child.IRawElementProviderSimple_iface,
            UIA_AutomationFocusChangedEventId);

    set_uia_hwnd_expects(0, 1, 1, 2, 0);
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 4); /* Only sent 4 times on Win11. */
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, hwnd[1], OBJID_WINDOW, CHILDID_SELF, event_handles,
        1, TRUE, FALSE, FALSE);
    check_uia_hwnd_expects_at_least(0, FALSE, 1, FALSE, 1, FALSE, 1, TRUE, 0, FALSE);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);

    /*
     * Child HWND now has a serverside provider, WinEvent is ignored.
     */
    child_win_prov_root = &Provider2.IRawElementProviderSimple_iface;

    SET_EXPECT(winproc_GETOBJECT_UiaRoot); /* Only sent on Win11. */
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 2); /* Only sent 2 times on Win11. */
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, hwnd[1], OBJID_WINDOW, CHILDID_SELF, event_handles,
        1, FALSE, FALSE, FALSE);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);
    CHECK_CALLED_AT_MOST(winproc_GETOBJECT_UiaRoot, 1);

    /*
     * HWNDs owned by a top level HWND that is within our scope are ignored.
     */
    child_win_prov_root = NULL;
    tmp_hwnd = CreateWindowA("ProxyProviderWinEventHandler test child class", "Test child window 2", WS_POPUP,
            0, 0, 50, 50, hwnd[0], NULL, NULL, NULL);
    Provider_nc3.hwnd = Provider_hwnd3.hwnd = tmp_hwnd;
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, tmp_hwnd, OBJID_WINDOW, CHILDID_SELF, event_handles,
        1, FALSE, FALSE, FALSE);
    DestroyWindow(tmp_hwnd);

    /*
     * Add our test child HWND to event2. This only puts the child HWND within
     * the scope of event2, it doesn't put the parent HWND within its scope.
     */
    child_win_prov_root = &Provider2.IRawElementProviderSimple_iface;
    Provider_nc3.hwnd = Provider_hwnd3.hwnd = hwnd[1];
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 2); /* Only sent 2 times on Win11. */
    set_uia_hwnd_expects(0, 1, 1, 1, 0);
    hr = UiaEventAddWindow(event2, hwnd[1]);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_uia_hwnd_expects_at_least(0, FALSE, 1, FALSE, 1, FALSE, 1, TRUE, 0, FALSE);
    CHECK_CALLED(child_winproc_GETOBJECT_UiaRoot);

    /*
     * Raise a WinEvent on our top level test HWND, will not invoke the
     * callback on event2.
     */
    set_uia_hwnd_expects(1, 1, 1, 4, 3);
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, hwnd[0], OBJID_WINDOW, CHILDID_SELF, event_handles,
            1, TRUE, FALSE, FALSE);
    check_uia_hwnd_expects_at_least(1, TRUE, 1, FALSE, 1, FALSE, 1, FALSE, 1, FALSE);

    /* Raise a WinEvent on our test child HWND, both event callbacks invoked. */
    child_win_prov_root = NULL;
    set_event_data_struct(&EventData2, 0, 0, 1, 1, &exp_node_desc, L"P)");

    set_uia_hwnd_expects(0, 2, 2, 4, 0);
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 8); /* Only sent 8 times on Win11. */
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, hwnd[1], OBJID_WINDOW, CHILDID_SELF, event_handles,
        ARRAY_SIZE(event_handles), TRUE, TRUE, FALSE);
    CHECK_CALLED_AT_LEAST(child_winproc_GETOBJECT_UiaRoot, 2);
    check_uia_hwnd_expects_at_least(0, FALSE, 2, FALSE, 2, FALSE, 2, TRUE, 0, FALSE);

    /*
     * Raise a WinEvent on a descendant HWND of our test HWND. If any ancestor
     * in the parent chain of HWNDs up to the root HWND is within scope, this
     * WinEvent is within scope.
     */
    tmp_hwnd = CreateWindowA("ProxyProviderWinEventHandler test child class", "Test child window 2", WS_CHILD,
            0, 0, 50, 50, hwnd[1], NULL, NULL, NULL);
    tmp_hwnd2 = CreateWindowA("ProxyProviderWinEventHandler test child class", "Test child window 3", WS_CHILD,
            0, 0, 50, 50, tmp_hwnd, NULL, NULL, NULL);
    set_provider_win_event_handler_win_event_expects(&Provider_nc3, EVENT_OBJECT_FOCUS, tmp_hwnd2, OBJID_WINDOW, CHILDID_SELF);
    set_provider_win_event_handler_win_event_expects(&Provider_hwnd3, EVENT_OBJECT_FOCUS, tmp_hwnd2, OBJID_WINDOW, CHILDID_SELF);
    Provider_nc3.hwnd = Provider_hwnd3.hwnd = tmp_hwnd2;

    set_uia_hwnd_expects(0, 2, 2, 0, 0);
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 12); /* Only sent 12 times on Win11. */
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, tmp_hwnd2, OBJID_WINDOW, CHILDID_SELF, event_handles,
        ARRAY_SIZE(event_handles), TRUE, TRUE, FALSE);
    CHECK_CALLED_AT_LEAST(child_winproc_GETOBJECT_UiaRoot, 2);
    check_uia_hwnd_expects_at_least(0, FALSE, 2, FALSE, 2, FALSE, 0, FALSE, 0, FALSE);

    DestroyWindow(tmp_hwnd);
    Provider_nc3.hwnd = Provider_hwnd3.hwnd = hwnd[1];
    set_provider_win_event_handler_win_event_expects(&Provider_nc3, EVENT_OBJECT_FOCUS, hwnd[1], OBJID_WINDOW, CHILDID_SELF);
    set_provider_win_event_handler_win_event_expects(&Provider_hwnd3, EVENT_OBJECT_FOCUS, hwnd[1], OBJID_WINDOW, CHILDID_SELF);

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = UiaRemoveEvent(event2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /*
     * Create an event on the desktop HWND. If a WinEvent handler interface is
     * returned on a provider representing the desktop HWND, all visible
     * top-level HWNDs at the time of advisement will be considered within
     * scope.
     */
    set_uia_hwnd_expects(1, 1, 1, 0, 0);
    hr = UiaGetRootNode(&node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!node, "Node == NULL.\n");
    check_uia_hwnd_expects(1, FALSE, 1, FALSE, 1, FALSE, 0, FALSE, 0, FALSE);

    Provider_proxy.win_event_handler_data.is_supported = TRUE;
    Provider_hwnd.win_event_handler_data.is_supported = Provider_nc.win_event_handler_data.is_supported = TRUE;
    set_provider_win_event_handler_win_event_expects(&Provider_nc, EVENT_OBJECT_FOCUS, GetDesktopWindow(), OBJID_WINDOW, CHILDID_SELF);
    set_provider_win_event_handler_win_event_expects(&Provider_hwnd, EVENT_OBJECT_FOCUS, GetDesktopWindow(), OBJID_WINDOW, CHILDID_SELF);
    set_provider_win_event_handler_respond_prov(&Provider_nc, &Provider_child.IRawElementProviderSimple_iface,
            UIA_AutomationFocusChangedEventId);

    /* Register a focus change event handler on the desktop HWND. */
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element, NULL, 0, &cache_req,
            &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    test_provider_event_advise_added(&Provider_proxy, 0, FALSE);
    test_provider_event_advise_added(&Provider_hwnd, 0, FALSE);
    test_provider_event_advise_added(&Provider_nc, 0, FALSE);

    /* Raise WinEvent on the desktop HWND. */
    set_provider_runtime_id(&Provider_child, UIA_RUNTIME_ID_PREFIX, HandleToUlong(GetDesktopWindow()));
    set_provider_win_event_handler_respond_prov(&Provider_hwnd, &Provider_child.IRawElementProviderSimple_iface,
            UIA_AutomationFocusChangedEventId);
    set_uia_hwnd_expects(0, 1, 1, 0, 0);
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, GetDesktopWindow(), OBJID_WINDOW, CHILDID_SELF, event_handles,
            1, TRUE, FALSE, FALSE);
    check_uia_hwnd_expects(0, FALSE, 1, FALSE, 1, FALSE, 0, FALSE, 0, FALSE);

    /*
     * Top-level HWND, a child of the desktop HWND. Will not have an event
     * raised since it was not visible when the desktop providers were advised
     * of an event being added.
     */
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, hwnd[0], OBJID_WINDOW, CHILDID_SELF, event_handles,
            1, FALSE, FALSE, FALSE);

    /* Test child hwnd, same deal. */
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, hwnd[1], OBJID_WINDOW, CHILDID_SELF, event_handles,
            1, FALSE, FALSE, FALSE);

    /*
     * Show window after calling UiaAddEvent(), does nothing. Window must be
     * visible when provider is advised of an event being added.
     */
    ShowWindow(hwnd[0], SW_SHOW);
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, hwnd[0], OBJID_WINDOW, CHILDID_SELF, event_handles,
            1, FALSE, FALSE, FALSE);

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /*
     * Create the event again, except this time our test HWND was visible when
     * the desktop provider was advised that our event was being added. Now
     * WinEvents on our test HWND will be handled.
     */
    hr = UiaAddEvent(node, UIA_AutomationFocusChangedEventId, uia_event_callback, TreeScope_Element, NULL, 0, &cache_req,
            &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    test_provider_event_advise_added(&Provider_hwnd, 0, FALSE);
    test_provider_event_advise_added(&Provider_nc, 0, FALSE);
    test_provider_event_advise_added(&Provider_proxy, 0, FALSE);

    /* WinEvent handled. */
    set_uia_hwnd_expects(1, 1, 1, 2, 1);
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, hwnd[0], OBJID_WINDOW, CHILDID_SELF, event_handles,
            1, TRUE, FALSE, FALSE);
    check_uia_hwnd_expects(1, TRUE, 1, FALSE, 1, FALSE, 2, FALSE, 1, FALSE);

    /* Child HWNDs of our test window are handled as well. */
    SET_EXPECT_MULTI(child_winproc_GETOBJECT_UiaRoot, 2);
    set_uia_hwnd_expects(0, 1, 1, 1, 0);
    test_uia_event_win_event_mapping(EVENT_OBJECT_FOCUS, hwnd[1], OBJID_WINDOW, CHILDID_SELF, event_handles,
            1, TRUE, FALSE, FALSE);
    check_uia_hwnd_expects(0, FALSE, 1, FALSE, 1, FALSE, 1, TRUE, 0, FALSE);

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    UiaNodeRelease(node);

skip_win_event_hwnd_filter_test:
    /*
     * Test default MSAA proxy WinEvent handler.
     */
    prov_root = &Provider.IRawElementProviderSimple_iface;
    set_uia_hwnd_expects(2, 1, 1, 2, 0);
    hr = UiaNodeFromHandle(hwnd[0], &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!node, "Node == NULL.\n");
    check_uia_hwnd_expects_at_most(1, 1, 1, 2, 0);

    Provider_hwnd2.win_event_handler_data.is_supported = Provider_nc2.win_event_handler_data.is_supported = TRUE;
    hr = UiaAddEvent(node, UIA_SystemAlertEventId, uia_event_callback, TreeScope_Subtree, NULL, 0, &cache_req,
            &event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    UiaNodeRelease(node);

    set_provider_win_event_handler_respond_prov(&Provider_hwnd2, NULL, 0);
    set_provider_win_event_handler_win_event_expects(&Provider_hwnd2, 0, hwnd[0], 0, 0);
    set_provider_win_event_handler_respond_prov(&Provider_nc2, NULL, 0);
    set_provider_win_event_handler_win_event_expects(&Provider_nc2, 0, NULL, 0, 0);

    prov_root = NULL;
    init_node_provider_desc(&exp_node_desc, GetCurrentProcessId(), NULL);
    add_provider_desc(&exp_node_desc, L"Main", NULL, TRUE); /* MSAA proxy. */
    set_event_data(0, 0, 1, 1, &exp_node_desc, L"P)");

    /* WinEvent handled by default MSAA proxy provider. */
    set_uia_hwnd_expects(1, 1, 1, 4, 5);
    test_uia_event_win_event_mapping(EVENT_SYSTEM_ALERT, hwnd[0], OBJID_CLIENT, 2, event_handles,
            1, TRUE, FALSE, FALSE);
    check_uia_hwnd_expects_at_most(1, 1, 1, 4, 5);

    hr = UiaRemoveEvent(event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(event_handles); i++)
        CloseHandle(event_handles[i]);
    method_sequences_enabled = TRUE;
    CoUninitialize();
    return 0;
}

static void test_uia_event_ProxyProviderWinEventHandler(void)
{
    HANDLE thread;
    HWND hwnd[2];

    /*
     * Windows 7 behaves different than all other versions, just skip the
     * tests.
     */
    if (!UiaLookupId(AutomationIdentifierType_Property, &OptimizeForVisualContent_Property_GUID))
    {
        win_skip("Skipping IProxyProviderWinEventSink tests for Win7\n");
        return;
    }

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hwnd[0] = create_test_hwnd("ProxyProviderWinEventHandler test class");
    hwnd[1] = create_child_test_hwnd("ProxyProviderWinEventHandler test child class", hwnd[0]);

    UiaRegisterProviderCallback(test_uia_provider_callback);

    /* Set clientside providers for our test windows and the desktop. */
    set_clientside_providers_for_hwnd(&Provider_proxy, &Provider_nc, &Provider_hwnd, GetDesktopWindow());
    base_hwnd_prov = &Provider_hwnd.IRawElementProviderSimple_iface;
    nc_prov = &Provider_nc.IRawElementProviderSimple_iface;
    proxy_prov = &Provider_proxy.IRawElementProviderSimple_iface;

    set_clientside_providers_for_hwnd(NULL, &Provider_nc2, &Provider_hwnd2, hwnd[0]);
    initialize_provider(&Provider, ProviderOptions_ServerSideProvider, hwnd[0], TRUE);
    Provider.frag_root = &Provider.IRawElementProviderFragmentRoot_iface;
    Provider.ignore_hwnd_prop = TRUE;

    set_clientside_providers_for_hwnd(NULL, &Provider_nc3, &Provider_hwnd3, hwnd[1]);
    initialize_provider(&Provider2, ProviderOptions_ServerSideProvider, hwnd[1], TRUE);
    Provider2.frag_root = &Provider2.IRawElementProviderFragmentRoot_iface;
    Provider2.ignore_hwnd_prop = TRUE;

    prov_root = &Provider.IRawElementProviderSimple_iface;
    child_win_prov_root = &Provider2.IRawElementProviderSimple_iface;

    thread = CreateThread(NULL, 0, uia_proxy_provider_win_event_handler_test_thread, (void *)hwnd, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;

        while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);

    CoUninitialize();
    destroy_test_hwnd(hwnd[0], "ProxyProviderWinEventHandler test class", "ProxyProviderWinEventHandler test child class");
    UiaRegisterProviderCallback(NULL);
}

static void test_UiaClientsAreListening(void)
{
    BOOL ret;

    /* Always returns TRUE on Windows 7 and above. */
    ret = UiaClientsAreListening();
    ok(!!ret, "ret != TRUE\n");
}

/*
 * Once a process returns a UI Automation provider with
 * UiaReturnRawElementProvider it ends up in an implicit MTA until exit. This
 * messes with tests around COM initialization, so we run these tests in
 * separate processes.
 */
static void launch_test_process(const char *name, const char *test_name)
{
    PROCESS_INFORMATION proc;
    STARTUPINFOA startup;
    char cmdline[MAX_PATH];

    sprintf(cmdline, "\"%s\" uiautomation %s", name, test_name);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &proc);
    wait_child_process(proc.hProcess);
}

START_TEST(uiautomation)
{
    HMODULE uia_dll = LoadLibraryA("uiautomationcore.dll");
    BOOL (WINAPI *pImmDisableIME)(DWORD);
    HMODULE hModuleImm32;
    char **argv;
    int argc;

    /* Make sure COM isn't initialized by imm32. */
    hModuleImm32 = LoadLibraryA("imm32.dll");
    if (hModuleImm32) {
        pImmDisableIME = (void *)GetProcAddress(hModuleImm32, "ImmDisableIME");
        if (pImmDisableIME)
            pImmDisableIME(0);
    }
    pImmDisableIME = NULL;
    FreeLibrary(hModuleImm32);

    if (uia_dll)
        pUiaDisconnectProvider = (void *)GetProcAddress(uia_dll, "UiaDisconnectProvider");

    argc = winetest_get_mainargs(&argv);
    if (argc == 3)
    {
        if (!strcmp(argv[2], "UiaNodeFromHandle"))
            test_UiaNodeFromHandle(argv[0]);
        else if (!strcmp(argv[2], "UiaNodeFromHandle_client_proc"))
            test_UiaNodeFromHandle_client_proc();
        else if (!strcmp(argv[2], "UiaRegisterProviderCallback"))
            test_UiaRegisterProviderCallback();
        else if (!strcmp(argv[2], "UiaAddEvent_client_proc"))
            test_UiaAddEvent_client_proc();

        FreeLibrary(uia_dll);
        return;
    }

    test_UiaClientsAreListening();
    test_UiaHostProviderFromHwnd();
    test_uia_reserved_value_ifaces();
    test_UiaLookupId();
    test_UiaNodeFromProvider();
    test_UiaGetPropertyValue();
    test_UiaGetRuntimeId();
    test_UiaHUiaNodeFromVariant();
    launch_test_process(argv[0], "UiaNodeFromHandle");
    launch_test_process(argv[0], "UiaRegisterProviderCallback");
    test_UiaGetUpdatedCache();
    test_UiaNavigate();
    test_UiaFind();
    test_CUIAutomation();
    test_default_clientside_providers();
    test_UiaGetRootNode();
    test_UiaNodeFromFocus();
    test_UiaAddEvent(argv[0]);
    test_UiaHasServerSideProvider();
    test_uia_event_ProxyProviderWinEventHandler();
    if (uia_dll)
    {
        pUiaProviderFromIAccessible = (void *)GetProcAddress(uia_dll, "UiaProviderFromIAccessible");
        if (pUiaProviderFromIAccessible)
            test_UiaProviderFromIAccessible();
        else
            win_skip("UiaProviderFromIAccessible not exported by uiautomationcore.dll\n");

        FreeLibrary(uia_dll);
    }
}
