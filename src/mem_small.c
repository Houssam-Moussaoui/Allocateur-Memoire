/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include "mem.h"
#include "mem_internals.h"



void *
emalloc_small(unsigned long size)
{

    if (size > SMALLALLOC) {
        handle_fatalError("taille trop grande");
    }

    if(arena.chunkpool == NULL){
        unsigned long taille= mem_realloc_small(); // si la liste chainée est vide , on réalloue de la mémoire

        void* cour=arena.chunkpool;
        void* suiv;

        for (int i=CHUNKSIZE;i<taille;i=i+CHUNKSIZE){
            suiv=(void*)((char*)cour+CHUNKSIZE); // on relie chaque noeud au suivant
            *((void**)cour)=suiv; //stocke pointeur vers le noeud suivant
            cour=suiv;
        }

        *((void**)cour)=NULL; //dernier noeud pointe vers NULL

    }

    void* premier_bloc=arena.chunkpool; //récupère premier bloc

    arena.chunkpool=  *((void**)arena.chunkpool); 

    return mark_memarea_and_get_user_ptr(premier_bloc, CHUNKSIZE, SMALL_KIND);

}

void efree_small(Alloc a) {
    *((void**)a.ptr)=arena.chunkpool; // on ajoute le bloc libéré au  début de la chunkpool
    arena.chunkpool=a.ptr; // mise à jour de la chunkpool 

}