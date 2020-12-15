#include <stdint.h>

#define MM_MAX_STRUCT_NAME 32

#include <stddef.h> /*for size_t*/


typedef enum{
    MM_FALSE,
    MM_TRUE
} vm_bool_t;

typedef struct _block_meta_data{
    vm_bool_t is_free;
    uint32_t block_size;
    uint32_t offset;    /* offset from the start of the page */
    struct _block_meta_data *prev_block;
    struct _block_meta_data *next_block;
} block_meta_data_t;

typedef struct _vm_page_family{
	char struct_name[MM_MAX_STRUCT_NAME];
	uint32_t struct_size;	
} vm_page_family_t;

typedef struct _vm_page_for_families{
	struct _vm_page_for_families *next;
	vm_page_family_t family_array[0];
}vm_page_for_families_t;

#define MAX_FAMILIES_PER_VM_PAGE    \
	(SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t))/sizeof(vm_page_family_t)

#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr, curr)             \
{                  \
	int count = 0;                                                                \
	for (curr = (vm_page_family_t*)vm_page_for_families_ptr->family_array;         \
		curr->struct_size && count < MAX_FAMILIES_PER_VM_PAGE;                    \
		curr++, count++){                                                         \

#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr, curr)        }}

vm_page_family_t * lookup_page_family_by_name (char *struct_name);

#define offset_of(container_structure, field_name)                          \
	((size_t)(&(((container_structure *)0)->field_name)))

#define MM_GET_PAGE_FROM_META_BLOCK(block_meta_data_ptr)                    \
	((void*)((char *)(block_meta_data_ptr) - ((block_meta_data_ptr)->offset)))

#define NEXT_META_BLOCK(block_meta_data_ptr)                                \
	((block_meta_data_ptr)->next_block)

#define PREV_META_BLOCK(block_meta_data_ptr)                                \
	((block_meta_data_ptr)->prev_block)

#define NEXT_META_BLOCK_BY_SIZE(block_meta_data_ptr)                        \
	((block_meta_data_ptr*)                                                 \
	((char *)((block_meta_data_ptr) + 1) +                                  \
	((block_meta_data_ptr)->block_size)))
