#include <stdio.h>
#include <memory.h>
#include <unistd.h>    /* for get pagesize */
#include <sys/mman.h>  /* conatins mmap() */
#include "mm.h"
#include <assert.h>

static size_t SYSTEM_PAGE_SIZE = 0;
static vm_page_for_families_t *family_page_head = NULL;

void mm_init(){
	SYSTEM_PAGE_SIZE = getpagesize();
}

/* Function to request VM page from kernel */
static void *mm_get_new_vm_page_from_kernel(int units){

	char *vm_page;

	vm_page = mmap(
		NULL, // starting address if want some address specifically
		units * SYSTEM_PAGE_SIZE,
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_ANONYMOUS | MAP_PRIVATE,
		0, 
		0);

	if (vm_page == MAP_FAILED){
		printf("Error: VM Page allocation failed\n");
		return NULL;
	}

	memset(vm_page, 0, units * SYSTEM_PAGE_SIZE);
	return (void *)vm_page;
}

static void mmap_return_vm_page_to_kernel(void* vm_page, int units){

	if (munmap(vm_page, units * SYSTEM_PAGE_SIZE)){
		printf("Error: Could not munmap vm page to kernel\n");
	}
}

static inline uint32_t mm_max_page_allocable_memory(int units){
	return (uint32_t)((SYSTEM_PAGE_SIZE * units) - offset_of(vm_page_t, page_memory));
}

void mm_instantiate_new_page_family(char *struct_name, int struct_size){

	vm_page_for_families_t *new_vm_page_for_families = NULL;
	vm_page_for_families_t *itr = NULL;
	vm_page_family_t *vm_page_family_curr = NULL;

	if (struct_size > SYSTEM_PAGE_SIZE){
		printf("Error: %s() Structure %s size exceeds system page size\n", __func__, struct_name);
		return;
	}

	if(!family_page_head){
		family_page_head = (vm_page_for_families_t*) mm_get_new_vm_page_from_kernel(1);
		family_page_head->next = NULL;
		strcpy(family_page_head->family_array[0].struct_name, struct_name);
		family_page_head->family_array[0].struct_size = struct_size;
		family_page_head->family_array[0].first_page = NULL;
		init_glthread(&family_page_head->family_array[0].free_block_priority_list_head);
		return;
	}

	int counter = 0;
	for(itr = family_page_head; itr; itr = itr->next){
		ITERATE_PAGE_FAMILIES_BEGIN(family_page_head, vm_page_family_curr, counter)
			// check for duplicates
			if(strcmp(vm_page_family_curr->struct_name, struct_name) == 0)
				return;
		ITERATE_PAGE_FAMILIES_END(family_page_head, vm_page_family_curr, counter)
	}
	
	if ((counter % MAX_FAMILIES_PER_VM_PAGE) == 0){
		new_vm_page_for_families = (vm_page_for_families_t *) mm_get_new_vm_page_from_kernel(1);
		new_vm_page_for_families->next = family_page_head;
		family_page_head = new_vm_page_for_families;
		vm_page_family_curr = &family_page_head->family_array[0];
	}

	strcpy(vm_page_family_curr->struct_name, struct_name);
	vm_page_family_curr->struct_size = struct_size;
	init_glthread(&vm_page_family_curr->free_block_priority_list_head);
	return;
}

void mm_print_registered_page_families(){

	vm_page_for_families_t *itr = NULL;
	vm_page_family_t *vm_page_family_curr = NULL;
	int counter = 0;
	for(itr = family_page_head; itr; itr = itr->next){
		ITERATE_PAGE_FAMILIES_BEGIN(family_page_head, vm_page_family_curr, counter)
		printf("name %s, size %d\n", vm_page_family_curr->struct_name, vm_page_family_curr->struct_size);
		ITERATE_PAGE_FAMILIES_END(family_page_head, vm_page_family_curr, counter)
	}
}

vm_page_family_t * lookup_page_family_by_name(char *struct_name){

	vm_page_for_families_t *itr = NULL;
	vm_page_family_t *vm_page_family_curr = NULL;
	int counter = 0;

	for(itr = family_page_head; itr; itr = itr->next){
		ITERATE_PAGE_FAMILIES_BEGIN(family_page_head, vm_page_family_curr, counter)
			if(strcmp(vm_page_family_curr->struct_name, struct_name) == 0)
				return vm_page_family_curr;
		ITERATE_PAGE_FAMILIES_END(family_page_head, vm_page_family_curr, counter)
	}

	// no object found.
	return NULL;
}

static void mm_union_free_blocks(block_meta_data_t *first, block_meta_data_t *second){

	assert(first->is_free == MM_TRUE && second->is_free == MM_TRUE);
	first->block_size += second->block_size + sizeof(block_meta_data_t);

	first->next_block = second->next_block;
	if(second->next_block)
		second->next_block->prev_block = first;
}

vm_bool_t mm_is_vm_page_empty(vm_page_t *vm_page){

	if(vm_page->block_meta_data.is_free == MM_TRUE && vm_page->block_meta_data.next_block == NULL && vm_page->block_meta_data.prev_block == NULL)
		return MM_TRUE;
	return MM_FALSE;
}

vm_page_t *allocate_vm_page (vm_page_family_t *vm_page_family){
	vm_page_t *vm_page = mm_get_new_vm_page_from_kernel(1);
	MARK_VM_PAGE_EMPTY(vm_page);
	vm_page->block_meta_data.block_size = mm_max_page_allocable_memory(1);
	vm_page->block_meta_data.offset =  offset_of(vm_page_t, block_meta_data);
	init_glthread(&vm_page->block_meta_data.priority_thread_glue);
	vm_page->prev = NULL;
	vm_page->next = NULL;

	/* Set the back pointer to page_family */
	vm_page->pg_family = vm_page_family;

	/* If it is the first VM data page for a given page family */
	if (vm_page_family->first_page == NULL){
		vm_page_family->first_page = vm_page;
		return (vm_page);
	}
	/* else */
	vm_page->next = vm_page_family->first_page;
	vm_page_family->first_page->prev = vm_page;
	vm_page_family->first_page = vm_page;

	return (vm_page);
}

void mm_vm_page_delete_and_free(vm_page_t *vm_page){
	vm_page_family_t* vm_page_family = vm_page->pg_family;

	if(vm_page_family->first_page != vm_page){
		vm_page_t *prev = vm_page->prev; // prev definitely exists
		prev->next = vm_page->next;
		if(vm_page->next){
			vm_page->next->prev = prev;
		}
	} else {
		// delete first page
		vm_page_family->first_page = vm_page->next;
		if(vm_page->next){
			vm_page->next->prev = NULL;
		}
	}
	mmap_return_vm_page_to_kernel((void *) vm_page, 1);
	return;
}

// int main(){

// 	// mm_init();
// 	// printf("VM page size %d\n", SYSTEM_PAGE_SIZE);

// 	// void *addr1 = mm_get_new_vm_page_from_kernel(1);
// 	// void *addr2 = mm_get_new_vm_page_from_kernel(1);

// 	// printf("page 1 = %p, page 2 = %p, diff %lld\n", addr1, addr2, addr2 - addr1);

// 	// while(1)
// 	 	mm_instantiate_new_page_family("abc", sizeof(abc));

// 	int a =0;
// 	{
// 		int a  =1;
// 	}

// 	printf("%d\n", a);
// 	return 0;
// }

