#ifndef XEN_PGTABLE_H_INCLUDED
#define XEN_PGTABLE_H_INCLUDED

#define PGDIR_SHIFT    22
#define PTRS_PER_PGD   1024
#define PTRS_PER_PTE   1024

/*
 * the pgd page can be thought of an array like this: pgd_t[PTRS_PER_PGD]
 *
 * this macro returns the index of the entry in the pgd page which would
 * control the given virtual address
 */
#define pgd_index(address) (((address) >> PGDIR_SHIFT) & (PTRS_PER_PGD-1))

/*
 * the pte page can be thought of an array like this: pte_t[PTRS_PER_PTE]
 *
 * this macro returns the index of the entry in the pte page which would
 * control the given virtual address
 */
#define pte_index(address) \
                (((address) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))

#endif /* XEN_PGTABLE_H_INCLUDED */
