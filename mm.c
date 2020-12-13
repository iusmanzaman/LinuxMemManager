#include <stdio.h>
#include <memory.h>
#include <unistd.h>    /* for get pagesize */
#include <sys/mman.h>  /* conatins mmap() */
#include "mm.h"

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
		return;
	}

	for(itr = family_page_head; itr; itr = itr->next){
		ITERATE_PAGE_FAMILIES_BEGIN(family_page_head, vm_page_family_curr)
			// check for duplicates
			if(strcmp(vm_page_family_curr->struct_name, struct_name) == 0)
				return;
		ITERATE_PAGE_FAMILIES_END(family_page_head, vm_page_family_curr)
	}

	if(!vm_page_family_curr->struct_size){ //we have room in same vm_page_for_family
		strcpy(vm_page_family_curr->struct_name, struct_name);
		vm_page_family_curr->struct_size = struct_size;
	}else{
		// allocate new page
		new_vm_page_for_families = (vm_page_for_families_t*) mm_get_new_vm_page_from_kernel(1);
		new_vm_page_for_families->next = family_page_head;
		family_page_head = new_vm_page_for_families;

		strcpy(family_page_head->family_array[0].struct_name, struct_name);
		family_page_head->family_array[0].struct_size = struct_size;
	}
}

void mm_print_registered_page_families(){

	vm_page_for_families_t *itr = NULL;
	vm_page_family_t *vm_page_family_curr = NULL;

	for(itr = family_page_head; itr; itr = itr->next){
		ITERATE_PAGE_FAMILIES_BEGIN(family_page_head, vm_page_family_curr)
		printf("name %s, size %d\n", vm_page_family_curr->struct_name, vm_page_family_curr->struct_size);
		ITERATE_PAGE_FAMILIES_END(family_page_head, vm_page_family_curr)
	}
}

vm_page_family_t * lookup_page_family_by_name(char *struct_name){

	vm_page_for_families_t *itr = NULL;
	vm_page_family_t *vm_page_family_curr = NULL;

	for(itr = family_page_head; itr; itr = itr->next){
		ITERATE_PAGE_FAMILIES_BEGIN(family_page_head, vm_page_family_curr)
			// check for duplicates
			if(strcmp(vm_page_family_curr->struct_name, struct_name) == 0)
				return;
		ITERATE_PAGE_FAMILIES_END(family_page_head, vm_page_family_curr)
	}	
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

