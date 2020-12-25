#include <stdint.h>
#include "glthread/glthread.h"

#define MM_MAX_STRUCT_NAME 32

#include <stddef.h> /*for size_t*/

typedef enum{
    MM_FALSE,
    MM_TRUE
} vm_bool_t;

typedef struct _block_meta_data{
    vm_bool_t  is_free;
    uint32_t   block_size;
    uint32_t   offset;    /* offset from the start of the page */
    glthread_t priority_thread_glue; /* glue-ing this struct for dl_list */
    struct     _block_meta_data *prev_block;
    struct     _block_meta_data *next_block;
} block_meta_data_t;

struct _vm_page_t;

typedef struct _vm_page_family{
	char              struct_name[MM_MAX_STRUCT_NAME];
	uint32_t          struct_size;
	struct _vm_page_t *first_page;
	glthread_t        free_block_priority_list_head;  /* header/container of dl_list */
} vm_page_family_t;

typedef struct _vm_page_t{
	struct _vm_page_t *next;
	struct _vm_page_t *prev;
	vm_page_family_t  *pg_family; /* back pointer */
	block_meta_data_t  block_meta_data;
	char               page_memory[0]; /* First data block in memory */
} vm_page_t;

typedef struct _vm_page_for_families{
	struct _vm_page_for_families  *next;
	vm_page_family_t              family_array[0];
}vm_page_for_families_t;

//#define GLTHREAD_TO_STRUCT(glthread_to_block_meta_data, block_meta_data_t, priority_thread_glue, glthread_ptr) 
#define MAX_FAMILIES_PER_VM_PAGE    \
	(SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t))/sizeof(vm_page_family_t)

#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr, curr, counter)      \
{                                                                                 \
	for (curr = (vm_page_family_t*)vm_page_for_families_ptr->family_array;        \
		curr->struct_size && counter < MAX_FAMILIES_PER_VM_PAGE;                  \
		curr++, counter++){                                                       

#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr, curr, counter)       }}

#define ITERATE_VM_PAGE_BEGIN(vm_page_family_ptr, curr)  \
{   for (curr = (vm_page_family_ptr)->first_page;        \
		curr;                                            \
		curr = curr->next){
#define ITERATE_VM_PAGE_END(vm_page_family_ptr, curr)  }}

#define ITERATE_VM_PAGE_ALL_BLOCK_BEGIN(vm_page_ptr, curr) \
{   for(curr = (vm_page_ptr)->block_meta_data; \
	    curr && (size_t)curr < (size_t)(vm_page_ptr + SYSTEM_PAGE_SIZE);     \
	    curr = curr->next_block){
#define ITERATE_VM_PAGE_ALL_BLOCK_END(vm_page_ptr, curr) }}

vm_page_family_t *lookup_page_family_by_name (char *struct_name);
vm_bool_t         mm_is_vm_page_empty        (vm_page_t *vm_page);
vm_page_t        *allocate_vm_page           (vm_page_family_t *vm_page_family);

#define MARK_VM_PAGE_EMPTY(vm_page_t_ptr) \
	(vm_page_t_ptr)->block_meta_data.prev_block = NULL;   \
	(vm_page_t_ptr)->block_meta_data.next_block = NULL;   \
	(vm_page_t_ptr)->block_meta_data.is_free = MM_TRUE;

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

#define mm_bind_blocks_for_allocation(allocated_meta_block, free_meta_block)  \
    free_meta_block->prev_block = allocated_meta_block;                       \
    free_meta_block->next_block = allocated_meta_block->next_block;           \
    allocated_meta_block->next_block = free_meta_block;                       \
    if (free_meta_block->next_block)                                          \
	    free_meta_block->next_block->prev_block = free_meta_block
