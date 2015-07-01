#include "mpi.h"
#include <stdio.h>
#include "checks.h"
#include "string.h"
#include "bit_ops.h"
#include "constants.h"
#include <stdlib.h>
#include "topo.h"
#include "sendrecv_local.h"
#include "mpi_optimized_collectives.h"


int
MPI_Scatter_binomial( void *sbuf, int scount, MPI_Datatype sdtype,
                      void *rbuf, int rcount, MPI_Datatype rdtype, int root,
                      MPI_Comm comm )
{
    int line = -1;
    int i;
    int rank;
    int vrank;
    int size;
    int total_send = 0;
    char *ptmp     = NULL;
    char *tempbuf  = NULL;
    int err;
    tree_t* bmtree;
    MPI_Status status;
    MPI_Aint sextent, slb, strue_lb, strue_extent; 
    MPI_Aint rextent, rlb, rtrue_lb, rtrue_extent;
//    mca_coll_tuned_module_t *tuned_module = (mca_coll_tuned_module_t*) module;
//    mca_coll_tuned_comm_t *data = tuned_module->tuned_data;

    // for some reason this call interferes with the sizE!!
    MPI_CHECK( err = MPI_Type_get_extent( sdtype, &slb, &sextent ) );

	MPI_CHECK( err = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( err = MPI_Comm_size( comm, &size ) );

    // could cache this somewhere?
    bmtree = topo_build_in_order_bmtree(comm, root);

    
	MPI_CHECK( err = MPI_Type_get_true_extent( sdtype, &strue_lb, &strue_extent ) );

    MPI_CHECK( err = MPI_Type_get_extent( rdtype, &rlb, &rextent ) );
    MPI_CHECK( err = MPI_Type_get_true_extent( rdtype, &rtrue_lb, &rtrue_extent ) );

    vrank = (rank - root + size) % size;

    if (rank == root) {
        if (0 == root) {
            /* root on 0, just use the send buffer */
            ptmp = (char *) sbuf;
            if (rbuf != MPI_IN_PLACE) {
                /* local copy to rbuf */
                err = sndrcv_local(sbuf, scount, sdtype,
                                   rbuf, rcount, rdtype, comm);
//                err = ompi_datatype_sndrcv(sbuf, scount, sdtype,
//                                           rbuf, rcount, rdtype);
                if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }
            }
        } else {
            /* root is not on 0, allocate temp buffer for send */
            NULL_CHECK( tempbuf = (char *) malloc(strue_extent + (scount*size - 1) * sextent) )

            ptmp = tempbuf - slb;

            /* and rotate data so they will eventually in the right place */
            memcpy(ptmp, (char *) sbuf + sextent*root*scount, rextent * scount*(size - root));

//            err = ompi_datatype_copy_content_same_ddt(sdtype, scount*(size - root),
//                                                      ptmp, (char *) sbuf + sextent*root*scount);
            if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }


            memcpy(ptmp + sextent*scount*(size - root), (char *) sbuf, sextent*scount*root);
//            err = ompi_datatype_copy_content_same_ddt(sdtype, scount*root,
//                                                      ptmp + sextent*scount*(size - root), (char *) sbuf);
            if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }

            if (rbuf != MPI_IN_PLACE) {
                /* local copy to rbuf */
                err = sndrcv_local(ptmp, scount, sdtype,
                                   rbuf, rcount, rdtype, comm);

//                err = ompi_datatype_sndrcv(ptmp, scount, sdtype,
//                                           rbuf, rcount, rdtype);
                if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }
            }
        }
        total_send = scount;
    } else if (!(vrank % 2)) {
        /* non-root, non-leaf nodes, allocte temp buffer for recv
         * the most we need is rcount*size/2 */
        NULL_CHECK( tempbuf = (char *) malloc(rtrue_extent + (rcount*size - 1) * rextent) );

        ptmp = tempbuf - rlb;

        sdtype = rdtype;
        scount = rcount;
        sextent = rextent;
        total_send = scount;
    } else {
        /* leaf nodes, just use rbuf */
        ptmp = (char *) rbuf;
    }

    if (!(vrank % 2)) {
        if (rank != root) {

            MPI_CHECK( err = MPI_Recv( ptmp, rcount*size, rdtype, bmtree->tree_prev,
                                       SCATTER_TAG, comm, MPI_STATUS_IGNORE) );

                       //ptmp + total_recv*rextent, rcount*size-total_recv, rdtype,
                       //                bmtree->tree_next[i], GATHER_TAG,
                       //                comm, MPI_STATUS_IGNORE) );

            /* recv from parent on non-root */
//            err = MCA_PML_CALL(recv(ptmp, rcount*size, rdtype, bmtree->tree_prev,
//                                    MCA_COLL_BASE_TAG_SCATTER, comm, &status));
            if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }
            /* local copy to rbuf */
            err = sndrcv_local(ptmp, scount, sdtype,
                               rbuf, rcount, rdtype, comm);

//            err = ompi_datatype_sndrcv(ptmp, scount, sdtype,
//                                       rbuf, rcount, rdtype);
            if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }
        }
        /* send to children on all non-leaf */
        for (i = 0; i < bmtree->tree_nextsize; i++) {
            int mycount = 0, vkid;
            /* figure out how much data I have to send to this child */
            vkid = (bmtree->tree_next[i] - root + size) % size;
            mycount = vkid - vrank;
            if (mycount > (size - vkid))
                mycount = size - vkid;
            mycount *= scount;

            MPI_CHECK( err = MPI_Send( ptmp + total_send*sextent, mycount, sdtype,
                                       bmtree->tree_next[i],
                                       SCATTER_TAG, comm) );

//                                       ptmp, total_recv, sdtype,
//                                       bmtree->tree_prev,
//                                       GATHER_TAG, comm) );
//
//            err = MCA_PML_CALL(send(ptmp + total_send*sextent, mycount, sdtype,
//                                    bmtree->tree_next[i],
//                                    MCA_COLL_BASE_TAG_SCATTER,
//                                    MCA_PML_BASE_SEND_STANDARD, comm));
            if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }

            total_send += mycount;
        }

        if (NULL != tempbuf) 
            free(tempbuf);
    } else {
        /* recv from parent on leaf nodes */

        MPI_CHECK( err = MPI_Recv( ptmp, rcount, rdtype, bmtree->tree_prev,
                                   SCATTER_TAG, comm, MPI_STATUS_IGNORE) );

//        err = MCA_PML_CALL(recv(ptmp, rcount, rdtype, bmtree->tree_prev,
//                                MCA_COLL_BASE_TAG_SCATTER, comm, &status));
        if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }
    }

    return MPI_SUCCESS;

 err_hndl:
    if (NULL != tempbuf)
        free(tempbuf);

//    OPAL_OUTPUT((ompi_coll_tuned_stream,  "%s:%4d\tError occurred %d, rank %2d",
//                 __FILE__, line, err, rank));
    return err;
}
