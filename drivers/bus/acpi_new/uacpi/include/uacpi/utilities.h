#pragma once

#include <uacpi/status.h>
#include <uacpi/types.h>
#include <uacpi/namespace.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UACPI_BAREBONES_MODE

/*
 * Checks whether the device at 'node' matches any of the PNP ids provided in
 * 'list' (terminated by a UACPI_NULL). This is done by first attempting to
 * match the value returned from _HID and then the value(s) from _CID.
 *
 * Note that the presence of the device (_STA) is not verified here.
 */
uacpi_bool uacpi_device_matches_pnp_id(
    uacpi_namespace_node *node,
    const uacpi_char *const *list
);

/*
 * Find all the devices in the namespace starting at 'parent' matching the
 * specified 'hids' (terminated by a UACPI_NULL) against any value from _HID or
 * _CID. Only devices reported as present via _STA are checked. Any matching
 * devices are then passed to the 'cb'.
 */
uacpi_status uacpi_find_devices_at(
    uacpi_namespace_node *parent,
    const uacpi_char *const *hids,
    uacpi_iteration_callback cb,
    void *user
);

/*
 * Same as uacpi_find_devices_at, except this starts at the root and only
 * matches one hid.
 */
uacpi_status uacpi_find_devices(
    const uacpi_char *hid,
    uacpi_iteration_callback cb,
    void *user
);

typedef enum uacpi_interrupt_model {
    UACPI_INTERRUPT_MODEL_PIC = 0,
    UACPI_INTERRUPT_MODEL_IOAPIC = 1,
    UACPI_INTERRUPT_MODEL_IOSAPIC = 2,
} uacpi_interrupt_model;

uacpi_status uacpi_set_interrupt_model(uacpi_interrupt_model);

typedef struct uacpi_pci_routing_table_entry {
    uacpi_u32 address;
    uacpi_u32 index;
    uacpi_namespace_node *source;
    uacpi_u8 pin;
} uacpi_pci_routing_table_entry;

typedef struct uacpi_pci_routing_table {
    uacpi_size num_entries;
    uacpi_pci_routing_table_entry entries[];
} uacpi_pci_routing_table;
void uacpi_free_pci_routing_table(uacpi_pci_routing_table*);

uacpi_status uacpi_get_pci_routing_table(
    uacpi_namespace_node *parent, uacpi_pci_routing_table **out_table
);

typedef struct uacpi_id_string {
    // size of the string including the null byte
    uacpi_u32 size;
    uacpi_char *value;
} uacpi_id_string;
void uacpi_free_id_string(uacpi_id_string *id);

/*
 * Evaluate a device's _HID method and get its value.
 * The returned struture must be freed using uacpi_free_id_string.
 */
uacpi_status uacpi_eval_hid(uacpi_namespace_node*, uacpi_id_string **out_id);

typedef struct uacpi_pnp_id_list {
    // number of 'ids' in the list
    uacpi_u32 num_ids;

    // size of the 'ids' list including the string lengths
    uacpi_u32 size;

    // list of PNP ids
    uacpi_id_string ids[];
} uacpi_pnp_id_list;
void uacpi_free_pnp_id_list(uacpi_pnp_id_list *list);

/*
 * Evaluate a device's _CID method and get its value.
 * The returned structure must be freed using uacpi_free_pnp_id_list.
 */
uacpi_status uacpi_eval_cid(uacpi_namespace_node*, uacpi_pnp_id_list **out_list);

/*
 * Evaluate a device's _STA method and get its value.
 * If this method is not found, the value of 'flags' is set to all ones.
 */
uacpi_status uacpi_eval_sta(uacpi_namespace_node*, uacpi_u32 *flags);

/*
 * Evaluate a device's _ADR method and get its value.
 */
uacpi_status uacpi_eval_adr(uacpi_namespace_node*, uacpi_u64 *out);

/*
 * Evaluate a device's _CLS method and get its value.
 * The format of returned string is BBSSPP where:
 *     BB => Base Class (e.g. 01 => Mass Storage)
 *     SS => Sub-Class (e.g. 06 => SATA)
 *     PP => Programming Interface (e.g. 01 => AHCI)
 * The returned struture must be freed using uacpi_free_id_string.
 */
uacpi_status uacpi_eval_cls(uacpi_namespace_node*, uacpi_id_string **out_id);

/*
 * Evaluate a device's _UID method and get its value.
 * The returned struture must be freed using uacpi_free_id_string.
 */
uacpi_status uacpi_eval_uid(uacpi_namespace_node*, uacpi_id_string **out_uid);


// uacpi_namespace_node_info->flags
#define UACPI_NS_NODE_INFO_HAS_ADR (1 << 0)
#define UACPI_NS_NODE_INFO_HAS_HID (1 << 1)
#define UACPI_NS_NODE_INFO_HAS_UID (1 << 2)
#define UACPI_NS_NODE_INFO_HAS_CID (1 << 3)
#define UACPI_NS_NODE_INFO_HAS_CLS (1 << 4)
#define UACPI_NS_NODE_INFO_HAS_SXD (1 << 5)
#define UACPI_NS_NODE_INFO_HAS_SXW (1 << 6)

typedef struct uacpi_namespace_node_info {
    // Size of the entire structure
    uacpi_u32 size;

    // Object information
    uacpi_object_name name;
    uacpi_object_type type;
    uacpi_u8 num_params;

    // UACPI_NS_NODE_INFO_HAS_*
    uacpi_u8 flags;

    /*
     * A mapping of [S1..S4] to the shallowest D state supported by the device
     * in that S state.
     */
    uacpi_u8 sxd[4];

    /*
     * A mapping of [S0..S4] to the deepest D state supported by the device
     * in that S state to be able to wake itself.
     */
    uacpi_u8 sxw[5];

    uacpi_u64 adr;
    uacpi_id_string hid;
    uacpi_id_string uid;
    uacpi_id_string cls;
    uacpi_pnp_id_list cid;
} uacpi_namespace_node_info;
void uacpi_free_namespace_node_info(uacpi_namespace_node_info*);

/*
 * Retrieve information about a namespace node. This includes the attached
 * object's type, name, number of parameters (if it's a method), the result of
 * evaluating _ADR, _UID, _CLS, _HID, _CID, as well as _SxD and _SxW.
 *
 * The returned structure must be freed with uacpi_free_namespace_node_info.
 */
uacpi_status uacpi_get_namespace_node_info(
    uacpi_namespace_node *node, uacpi_namespace_node_info **out_info
);

#endif // !UACPI_BAREBONES_MODE

#ifdef __cplusplus
}
#endif
