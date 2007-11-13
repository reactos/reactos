#ifndef __R300_MEM_H__
#define __R300_MEM_H__

//#define R300_MEM_PDL 0
#define R300_MEM_UL 1

#define R300_MEM_R 1
#define R300_MEM_W 2
#define R300_MEM_RW (R300_MEM_R | R300_MEM_W)

#define R300_MEM_SCRATCH 2

struct r300_memory_manager {
	struct {
		void *ptr;
		uint32_t size;
		uint32_t age;
		uint32_t h_pending;
		int pending;
		int mapped;
	} *u_list;
	int u_head, u_size, u_last;

};

extern void r300_mem_init(r300ContextPtr rmesa);
extern void r300_mem_destroy(r300ContextPtr rmesa);
extern void *r300_mem_ptr(r300ContextPtr rmesa, int id);
extern int r300_mem_find(r300ContextPtr rmesa, void *ptr);
extern int r300_mem_alloc(r300ContextPtr rmesa, int alignment, int size);
extern void r300_mem_use(r300ContextPtr rmesa, int id);
extern unsigned long r300_mem_offset(r300ContextPtr rmesa, int id);
extern void *r300_mem_map(r300ContextPtr rmesa, int id, int access);
extern void r300_mem_unmap(r300ContextPtr rmesa, int id);
extern void r300_mem_free(r300ContextPtr rmesa, int id);

#endif
