/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * 
 * Shameless stolen from openmpi
 */

#include "mpi.h"

#ifndef OPTIMIZED_TOPO_H
#define OPTIMIZED_TOPO_H

#define MAXTREEFANOUT 32

//BEGIN_C_DECLS

typedef struct tree_t {
    int tree_root;
    int tree_fanout;
    int tree_bmtree;
    int tree_prev;
    int tree_next[MAXTREEFANOUT];
    int tree_nextsize;
} tree_t;

//tree_t*
//topo_build_tree( int fanout,
//                                 struct ompi_communicator_t* com,
//                                 int root );
//tree_t*
//topo_build_in_order_bintree( struct ompi_communicator_t* comm );
//
//tree_t*
//topo_build_bmtree( struct ompi_communicator_t* comm,
//                                   int root );

tree_t*
topo_build_in_order_bmtree( MPI_Comm *comm,
                            int root );
//tree_t*
//topo_build_chain( int fanout,
//                                  struct ompi_communicator_t* com,
//                                  int root );

//int topo_destroy_tree( tree_t** tree );

/* debugging stuff, will be removed later */
//int topo_dump_tree (tree_t* tree, int rank);

//END_C_DECLS

#endif  /* OPTIMIZED_TOPO_H */

