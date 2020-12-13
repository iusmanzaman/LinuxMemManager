#include <stdint.h>

#define MM_MAX_STRUCT_NAME 32

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