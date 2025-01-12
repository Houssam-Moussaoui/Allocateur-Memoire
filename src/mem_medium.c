/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <stdint.h>
#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

unsigned int puiss2(unsigned long size) {
    unsigned int p=0;
    size = size -1; 
    while(size) {  
	p++;
	size >>= 1;
    }
    if (size > (1 << p))
	p++;
    return p;
}


void * find_buddy(void * ptr,unsigned int id){

    void* buddy=(void*)((uintptr_t)ptr ^ (1<<(id)));
    void * current=arena.TZL[id];
    void * precedent=arena.TZL+id;
    while(buddy!=current && current ){
        precedent=current;
        current=*(void**)current;
    }
    if(!current){
        return NULL;
    }
    else
    {
        *(void**)precedent=*(void**)current;
        return buddy;
    }
    return buddy;
}


void * find_memory_spot_recursive(unsigned int id)
{
    
    unsigned int max=FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant; //taille maximal 

    if(id==max && !arena.TZL[id]){  //si on atteint la taille maximale et qu'il n'y a pas de blocs libres à cet indice
        mem_realloc_medium(); // Réalloue de la mémoire
        
        void* cour=arena.TZL[id];

        *((void**)cour)=NULL;


        void * returned_bloc=arena.TZL[id];
        arena.TZL[id]=*(void **)arena.TZL[id];
        return returned_bloc;
    }

    else if(id<max && !arena.TZL[id]){  //si aucun bloc libr n'est disponible à cet indice mais qu'on peut monter en taille
        void * bigger_spot=find_memory_spot_recursive(id+1);
        void* buddy=(void*)((uintptr_t)bigger_spot ^ (1<<(id)));

        arena.TZL[id]=buddy;
        *(void **)buddy=NULL;

        return bigger_spot;
    }
    else if(id<=max && arena.TZL[id]){ //Si un bloc est disponible à cet indice, on  le retourne directement
        void * returned_bloc=arena.TZL[id];
        arena.TZL[id]=*(void **)returned_bloc;
        return returned_bloc;
    }
    return NULL; // Si aucun bloc n'est trouvé
}

void *
emalloc_medium(unsigned long size)
{
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);

    unsigned int indice_=puiss2(size+32); //l'indice correspondant à la puissance de 2
    void *spot= find_memory_spot_recursive(indice_); // Cherche un bloc de mémoire disponible
    return mark_memarea_and_get_user_ptr(spot,(1<<(indice_)),MEDIUM_KIND); // on le amrque


}



void* find_buddy_and_merge_recursive(void * ptr,unsigned int* id){
    void * buddy=find_buddy(ptr,*id); // on cherche le buddy

    if(!buddy){ // Si aucun buddy n'est trouvé, retourne le bloc actuel
        return ptr;
    }
    else    // on fusionne les deux blocs et continue la recherche du buddy à un indice supérieur
    {
        void * min_ptr=(ptr < buddy) ? ptr : buddy;
        (*id)++;
        return find_buddy_and_merge_recursive(min_ptr,id);
    }

}


void efree_medium(Alloc a) {

    unsigned long size=a.size;
    unsigned int id =puiss2(size);

    // //on trouve et fusionne le bloc avec son buddy (si c'est possible)
    unsigned int * address_id=&id;
    void *final_bloc=find_buddy_and_merge_recursive(a.ptr,address_id);

    // on ajoute le bloc(qui peut venir d'une fusion) à la liste chainée 
    *(void**)final_bloc=arena.TZL[*address_id];
    arena.TZL[*address_id]=final_bloc;

}


