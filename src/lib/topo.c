#include "topo.h"
#include "mpi.h"
#include "checks.h"

/* stolen from openmpi, tuned topo */

/*
 * Constructs in-order binomial tree which can be used for gather/scatter
 * operations.
 * 
 * Here are some of the examples of this tree:
 * size == 2                   size = 4                 size = 8
 *      0                           0                        0
 *     /                          / |                      / | \
 *    1                          1  2                     1  2  4
 *                                  |                        |  | \
 *                                  3                        3  5  6
 *                                                                 |
 *                                                                 7
 */
tree_t*
topo_build_in_order_bmtree( MPI_Comm *comm,
                            int root )
{
    int childs = 0;
    int rank, vrank;
    int size;
    int mask = 1;
    int remote;
    tree_t *bmtree;
    int i;
    int rc;

    /* 
     * Get size and rank of the process in this communicator 
     */
	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );

    vrank = (rank - root + size) % size;

    bmtree = (tree_t*)malloc(sizeof(tree_t));
    if (!bmtree) {
        return NULL;
    }

    bmtree->tree_bmtree   = 1;
    bmtree->tree_root     = MPI_UNDEFINED;
    bmtree->tree_nextsize = MPI_UNDEFINED;
    for(i=0;i<MAXTREEFANOUT;i++) {
        bmtree->tree_next[i] = -1;
    }

    if (root == rank) {
        bmtree->tree_prev = root;
    }

    while (mask < size) {
        remote = vrank ^ mask;
        if (remote < vrank) {
            bmtree->tree_prev = (remote + root) % size;
            break;
        } else if (remote < size) {
            bmtree->tree_next[childs] = (remote + root) % size;
            childs++;
            if (childs==MAXTREEFANOUT) {
                return NULL;
            }
        }
        mask <<= 1;
    }
    bmtree->tree_nextsize = childs;
    bmtree->tree_root     = root;

    return bmtree;
}
