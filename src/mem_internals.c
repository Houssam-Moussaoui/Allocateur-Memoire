/******************************************************
 * Copyright Grégory Mounié 2018-2022                 *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include "mem.h"
#include "mem_internals.h"

unsigned long knuth_mmix_one_round(unsigned long in)
{
    return in * 6364136223846793005UL % 1442695040888963407UL;
}

void *mark_memarea_and_get_user_ptr(void *ptr, unsigned long size, MemKind k)
{
    unsigned long size_midddle=size-32; // correspond à la taille  du bloc centrale    
    uint64_t  magic = ( knuth_mmix_one_round((unsigned long)ptr) & ~0b11UL)|(unsigned long)k ;

    uint64_t* point=(uint64_t*)ptr;


    *point=size; 
    point++;

    *point=magic;
    point++;

    *(uint64_t*)((char*)point+size_midddle)=magic;

    *(uint64_t*)((char*)point+8+size_midddle)=size;
    
    return (void *)point;
}

Alloc
mark_check_and_get_alloc(void *ptr)
{

    uint64_t* point=((uint64_t*)ptr)-2;
    
    uint64_t taille1=*(point);
    uint64_t magic1=*(point+1);

    uint64_t type_memoire= (magic1 & 0b11UL);

    uint64_t size_midddle=taille1-32;
    uint64_t taille2 = *(uint64_t*)(3*8+(char *)point+size_midddle);
    assert(taille1==taille2);
    assert(magic1==(uint64_t)((knuth_mmix_one_round((unsigned long)point) & ~0b11UL)|(unsigned long)(MemKind)type_memoire));
    uint64_t magic2 = *(uint64_t*)(2*8+(char *)point+size_midddle);
    assert(magic1==magic2);

    Alloc a = {};
    a.kind=(MemKind)type_memoire;
    a.size=(unsigned long )taille1;
    a.ptr= (void*)point;

    return a;
}


unsigned long
mem_realloc_small() 
{
    assert(arena.chunkpool == 0);
    unsigned long size = (FIRST_ALLOC_SMALL << arena.small_next_exponant);
    arena.chunkpool = mmap(0,
			   size,
			   PROT_READ | PROT_WRITE | PROT_EXEC,
			   MAP_PRIVATE | MAP_ANONYMOUS,
			   -1,
			   0);
    if (arena.chunkpool == MAP_FAILED)
	handle_fatalError("small realloc");
    arena.small_next_exponant++;
    return size;
}

unsigned long
mem_realloc_medium() {
    uint32_t indice = FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant;
    assert(arena.TZL[indice] == 0);
    unsigned long size = (FIRST_ALLOC_MEDIUM << arena.medium_next_exponant);
    assert( size == (1UL << indice));
    arena.TZL[indice] = mmap(0,
			     size*2, // twice the size to allign
			     PROT_READ | PROT_WRITE | PROT_EXEC,
			     MAP_PRIVATE | MAP_ANONYMOUS,
			     -1,
			     0);
    if (arena.TZL[indice] == MAP_FAILED)
	handle_fatalError("medium realloc");
    // align allocation to a multiple of the size
    // for buddy algo
    arena.TZL[indice] += (size - (((intptr_t)arena.TZL[indice]) % size));
    arena.medium_next_exponant++;
    return size; // lie on allocation size, but never free
}


// used for test in buddy algo
unsigned int
nb_TZL_entries() {
    int nb = 0;
    
    for(int i=0; i < TZL_SIZE; i++)
	if ( arena.TZL[i] )
	    nb ++;

    return nb;
}
