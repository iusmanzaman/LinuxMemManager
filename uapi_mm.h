#ifndef __UAPI_MM__
#define __UAPI_MM__

#include <stdint.h>


void mm_init();

void mm_instantiate_new_page_family(char *struct_name, int struct_size);
void mm_print_registered_page_families ();

#define MM_REG_STRUCT(struct_name) \
	(mm_instantiate_new_page_family(#struct_name, sizeof(struct_name)))


#endif /* __UAPI_MM__ */