/*
 * edns.c
 *
 * edns implementation
 *
 * a Net::DNS like library for C
 *
 * (c) NLnet Labs, 2004-2022
 *
 * See the file LICENSE for the license
 */

#include <ldns/ldns.h>

#define LDNS_OPTIONLIST_INIT 8

/*
 * Access functions
 * functions to get and set type checking
 */

/* read */
size_t
ldns_edns_get_size(const ldns_edns_option *edns)
{
	assert(edns != NULL);
	return edns->_size;
}

ldns_edns_option_code
ldns_edns_get_code(const ldns_edns_option *edns)
{
	assert(edns != NULL);
	return edns->_code;
}

uint8_t *
ldns_edns_get_data(const ldns_edns_option *edns)
{
	assert(edns != NULL);
	return edns->_data;
}

ldns_buffer *
ldns_edns_get_wireformat_buffer(const ldns_edns_option *edns)
{
	uint16_t option;
	size_t size;
	uint8_t* data;
	ldns_buffer* buffer;

	if (edns == NULL) {
		return NULL;
	}

	option = ldns_edns_get_code(edns);
	size = ldns_edns_get_size(edns);
	data = ldns_edns_get_data(edns);

	buffer = ldns_buffer_new(size + 4);

	if (buffer == NULL) {
		return NULL;
	}

	ldns_buffer_write_u16(buffer, option);
	ldns_buffer_write_u16(buffer, size);
	ldns_buffer_write(buffer, data, size);

	ldns_buffer_flip(buffer);

	return buffer;
}

/* write */
void
ldns_edns_set_size(ldns_edns_option *edns, size_t size)
{
	assert(edns != NULL);
	edns->_size = size;
}

void
ldns_edns_set_code(ldns_edns_option *edns, ldns_edns_option_code code)
{
	assert(edns != NULL);
	edns->_code = code;
}

void
ldns_edns_set_data(ldns_edns_option *edns, void *data)
{
	/* only copy the pointer */
	assert(edns != NULL);
	edns->_data = data;
}

/* note: data must be allocated memory */
ldns_edns_option *
ldns_edns_new(ldns_edns_option_code code, size_t size, void *data)
{
	ldns_edns_option *edns;
	edns = LDNS_MALLOC(ldns_edns_option);
	if (!edns) {
		return NULL;
	}
	ldns_edns_set_code(edns, code);
	ldns_edns_set_size(edns, size);
	ldns_edns_set_data(edns, data);

	return edns;
}

ldns_edns_option *
ldns_edns_new_from_data(ldns_edns_option_code code, size_t size, const void *data)
{
	ldns_edns_option *edns;
	edns = LDNS_MALLOC(ldns_edns_option);
	if (!edns) {
		return NULL;
	}
	edns->_data = LDNS_XMALLOC(uint8_t, size);
	if (!edns->_data) {
		LDNS_FREE(edns);
		return NULL;
	}

	/* set the values */
	ldns_edns_set_code(edns, code);
	ldns_edns_set_size(edns, size);
	memcpy(edns->_data, data, size);

	return edns;
}

ldns_edns_option *
ldns_edns_clone(ldns_edns_option *edns)
{
	ldns_edns_option *new_option;

	assert(edns != NULL);

	new_option = ldns_edns_new_from_data(ldns_edns_get_code(edns),
		ldns_edns_get_size(edns),
		ldns_edns_get_data(edns));

	return new_option;
}

void
ldns_edns_deep_free(ldns_edns_option *edns)
{
	if (edns) {
		if (edns->_data) {
			LDNS_FREE(edns->_data);
		}
		LDNS_FREE(edns);
	}
}

void 
ldns_edns_free(ldns_edns_option *edns)
{
	if (edns) {
		LDNS_FREE(edns);
	}
}

ldns_edns_option_list*
ldns_edns_option_list_new()
{
	ldns_edns_option_list *option_list = LDNS_MALLOC(ldns_edns_option_list);
	if(!option_list) {
		return NULL;
	}

	option_list->_option_count = 0;
	option_list->_option_capacity = 0;
	option_list->_options_size = 0;
	option_list->_options = NULL;
	return option_list;
}

ldns_edns_option_list *
ldns_edns_option_list_clone(ldns_edns_option_list *old_list)
{
	size_t i;
	ldns_edns_option_list *new_list;

	if (!old_list) {
		return NULL;
	}

	new_list = ldns_edns_option_list_new();
	if (!new_list) {
		return NULL;
	}

	if (old_list->_option_count == 0) {
		return new_list;
	}

	/* adding options also updates the total options size */
	for (i = 0; i < old_list->_option_count; i++) {
		ldns_edns_option *option = ldns_edns_clone(ldns_edns_option_list_get_option(old_list, i));
		if (!ldns_edns_option_list_push(new_list, option)) {
			ldns_edns_deep_free(option);
			ldns_edns_option_list_deep_free(new_list);
			return NULL;
		}
	}
	return new_list;
}

void
ldns_edns_option_list_free(ldns_edns_option_list *option_list)
{
	if (option_list) {
		LDNS_FREE(option_list->_options);
		LDNS_FREE(option_list);
	}
}

void
ldns_edns_option_list_deep_free(ldns_edns_option_list *option_list)
{
	size_t i;

	if (option_list) {
		for (i=0; i < ldns_edns_option_list_get_count(option_list); i++) {
			ldns_edns_deep_free(ldns_edns_option_list_get_option(option_list, i));
		}
		ldns_edns_option_list_free(option_list);
	}
}

size_t
ldns_edns_option_list_get_count(const ldns_edns_option_list *option_list)
{
	if (option_list) {
		return option_list->_option_count;
	} else {
		return 0;
	}
}

ldns_edns_option *
ldns_edns_option_list_get_option(const ldns_edns_option_list *option_list, size_t index)
{
	if (option_list && index < ldns_edns_option_list_get_count(option_list)) {
		assert(option_list->_options[index]);
		return option_list->_options[index];
	} else {
		return NULL;
	}
}

size_t
ldns_edns_option_list_get_options_size(const ldns_edns_option_list *option_list)
{
	if (option_list) {
		return option_list->_options_size;
	} else {
		return 0;
	}
}


ldns_edns_option *
ldns_edns_option_list_set_option(ldns_edns_option_list *option_list,
	ldns_edns_option *option, size_t index)
{
	ldns_edns_option* old;

	assert(option_list != NULL);

	if (index > ldns_edns_option_list_get_count(option_list)) {
		return NULL;
	}

	if (option == NULL) {
		return NULL;
	}

	old = ldns_edns_option_list_get_option(option_list, index);

	/* shrink the total EDNS size if the old EDNS option exists */
	if (old != NULL) {
		option_list->_options_size -= (ldns_edns_get_size(old) + 4);
	}

	option_list->_options_size += (ldns_edns_get_size(option) + 4);

	option_list->_options[index] = option;
	return old;
}

bool
ldns_edns_option_list_push(ldns_edns_option_list *option_list,
	ldns_edns_option *option)
{
	size_t cap;
	size_t option_count;

	assert(option_list != NULL);

	if (option == NULL) {
		return false;
	}

	cap = option_list->_option_capacity;
	option_count = ldns_edns_option_list_get_count(option_list);

	/* verify we need to grow the array to fit the new option */
	if (option_count+1 > cap) {
		ldns_edns_option **new_list;

		/* initialize the capacity if needed, otherwise grow by doubling */
		if (cap == 0) {
			cap = LDNS_OPTIONLIST_INIT; /* initial list size */
		} else {
			cap *= 2;
		}

		new_list = LDNS_XREALLOC(option_list->_options,
			ldns_edns_option *, cap);

		if (!new_list) {
			return false;
		}

		option_list->_options = new_list;
		option_list->_option_capacity = cap;
	}

	/* add the new option */
	ldns_edns_option_list_set_option(option_list, option,
		option_list->_option_count);
	option_list->_option_count += 1;

	return true;
}

ldns_edns_option *
ldns_edns_option_list_pop(ldns_edns_option_list *option_list)
{
	ldns_edns_option* pop;
	size_t count;
	size_t cap;

	assert(option_list != NULL);

	cap = option_list->_option_capacity;
	count = ldns_edns_option_list_get_count(option_list);

	if (count == 0) {
		return NULL;
	}
	/* get the last option from the list */
	pop = ldns_edns_option_list_get_option(option_list, count-1);

	/* shrink the array */
	if (cap > LDNS_OPTIONLIST_INIT && count-1 <= cap/2) {
		ldns_edns_option **new_list;

		cap /= 2;

		new_list = LDNS_XREALLOC(option_list->_options,
			ldns_edns_option *, cap);
		if (new_list) {
			option_list->_options = new_list;
		}
		/* if the realloc fails, the capacity for the list remains unchanged */
	}

	/* shrink the total EDNS size of the options if the popped EDNS option exists */
	if (pop != NULL) {
		option_list->_options_size -= (ldns_edns_get_size(pop) + 4);
	}

	option_list->_option_count = count - 1;

	return pop;
}

ldns_buffer *
ldns_edns_option_list2wireformat_buffer(const ldns_edns_option_list *option_list)
{
	size_t i, list_size, options_size, option, size;
	ldns_buffer* buffer;
	ldns_edns_option *edns;
	uint8_t* data = NULL;

	if (!option_list) {
		return NULL;
	}

	/* get the number of EDNS options in the list*/
	list_size = ldns_edns_option_list_get_count(option_list);

	/* create buffer the size of the total EDNS wireformat options */
	options_size = ldns_edns_option_list_get_options_size(option_list);
	buffer = ldns_buffer_new(options_size);
	
	if (!buffer) {
		return NULL;
	}

	/* write individual serialized EDNS options to final buffer*/
	for (i = 0; i < list_size; i++) {
		edns = ldns_edns_option_list_get_option(option_list, i);

		if (edns == NULL) {
			/* this shouldn't be possible */
			return NULL;
		}

		option = ldns_edns_get_code(edns);
		size = ldns_edns_get_size(edns);
		data = ldns_edns_get_data(edns);

		/* make sure the option fits */
		if (!(ldns_buffer_available(buffer, size + 4))) {
			ldns_buffer_free(buffer);
			return NULL;
		}

		ldns_buffer_write_u16(buffer, option);
		ldns_buffer_write_u16(buffer, size);
		ldns_buffer_write(buffer, data, size);
	}

	ldns_buffer_flip(buffer);

	return buffer;
}
