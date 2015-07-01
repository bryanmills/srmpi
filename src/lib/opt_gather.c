#include "mpi.h"
#include <stdio.h>
#include "string.h"
#include "checks.h"
#include "bit_ops.h"
#include "constants.h"
#include "topo.h"
#include "sendrecv_local.h"
#include "mpi_optimized_collectives.h"


int
MPI_Gather_binomial(void *sbuf, int scount,
                    MPI_Datatype *sdtype,
                    void *rbuf, int rcount,
                    MPI_Datatype *rdtype,
                    int root,
                    MPI_Comm *comm)
{
    int line = -1;
    int i;
    int rank;
    int vrank;
    int size = 0;
    int total_recv = 0;
    char *ptmp     = NULL;
    char *tempbuf  = NULL;
    int err = 0;
    tree_t* bmtree;
    MPI_Status status;
    MPI_Aint sextent=0, slb=0, strue_lb=0, strue_extent=0; 
    MPI_Aint rextent, rlb, rtrue_lb, rtrue_extent;

    // not sure if this is MPI spec!
    MPI_CHECK( err = MPI_Type_get_extent( sdtype, &slb, &sextent ) );
	MPI_CHECK( err = MPI_Type_get_true_extent( sdtype, &strue_lb, &strue_extent ) );

	MPI_CHECK( err = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( err = MPI_Comm_size( comm, &size ) );
    // could cache this somewhere?
    bmtree = topo_build_in_order_bmtree(comm, root);
    vrank = (rank - root + size) % size;

    if (rank == root) {
        MPI_CHECK( err = MPI_Type_get_extent( rdtype, &rlb, &rextent ) );
        MPI_CHECK( err = MPI_Type_get_true_extent( rdtype, &rtrue_lb, &rtrue_extent ) );

        if (0 == root){
            /* root on 0, just use the recv buffer */
            ptmp = (char *) rbuf;
            if (sbuf != MPI_IN_PLACE) {
                // This is really bad
                err = sndrcv_local(sbuf, scount, sdtype,
                                   ptmp, rcount, rdtype, comm);
//                err = ompi_datatype_sndrcv(sbuf, scount, sdtype,
//                                           ptmp, rcount, rdtype);
                if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }
            }
        } else {
            /* root is not on 0, allocate temp buffer for recv,
             * rotate data at the end */
            NULL_CHECK( tempbuf = (char *) malloc(rtrue_extent + (rcount*size - 1) * rextent) );

            ptmp = tempbuf - rlb;
            if (sbuf != MPI_IN_PLACE) {
                /* copy from sbuf to temp buffer */
                err = sndrcv_local(sbuf, scount, sdtype,
                                   ptmp, rcount, rdtype, comm);
//                err = ompi_datatype_sndrcv(sbuf, scount, sdtype,
//                                           ptmp, rcount, rdtype);
                if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }
            } else {
                /* copy from rbuf to temp buffer  */
                memcpy(ptmp, (char *) rbuf + rank*rextent*rcount, rextent * rcount);

//                err = ompi_datatype_copy_content_same_ddt(rdtype, rcount, ptmp, 
//                                                          (char *) rbuf + rank*rextent*rcount);
                if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }
            }
        }
        total_recv = rcount;
    } else if (!(vrank % 2)) {
        /* other non-leaf nodes, allocate temp buffer for data received from
         * children, the most we need is half of the total data elements due
         * to the property of binimoal tree */
        NULL_CHECK( tempbuf = (char *) malloc(strue_extent + (scount*size - 1) * sextent) );
        if (NULL == tempbuf) {
            goto err_hndl;
        }

        ptmp = tempbuf - slb;
        /* local copy to tempbuf */
        err = sndrcv_local(sbuf, scount, sdtype,
                           ptmp, scount, sdtype, comm);

//        err = ompi_datatype_sndrcv(sbuf, scount, sdtype,
//                                   ptmp, scount, sdtype);
        if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }

        /* use sdtype,scount as rdtype,rdcount since they are ignored on
         * non-root procs */
        rdtype = sdtype;
        rcount = scount;
        rextent = sextent;
        total_recv = rcount;
    } else {
        /* leaf nodes, no temp buffer needed, use sdtype,scount as
         * rdtype,rdcount since they are ignored on non-root procs */
        ptmp = (char *) sbuf;
        total_recv = scount;
    }

    if (!(vrank % 2)) {
        /* all non-leaf nodes recv from children */
        for (i = 0; i < bmtree->tree_nextsize; i++) {
            int mycount = 0, vkid;
            /* figure out how much data I have to send to this child */
            vkid = (bmtree->tree_next[i] - root + size) % size;
            mycount = vkid - vrank;
            if (mycount > (size - vkid))
                mycount = size - vkid;
            mycount *= rcount;

//            OPAL_OUTPUT((ompi_coll_tuned_stream,
//                         "ompi_coll_tuned_gather_intra_binomial rank %d recv %d mycount = %d",
//                         rank, bmtree->tree_next[i], mycount));

            MPI_CHECK( err = MPI_Recv( ptmp + total_recv*rextent, rcount*size-total_recv, rdtype,
                                       bmtree->tree_next[i], GATHER_TAG,
                                       comm, MPI_STATUS_IGNORE) );

//            err = MCA_PML_CALL(recv(ptmp + total_recv*rextent, rcount*size-total_recv, rdtype,
//                                    bmtree->tree_next[i], MCA_COLL_BASE_TAG_GATHER,
//                                    comm, &status));
            if (MPI_SUCCESS != err) { 
                goto err_hndl; 
            }
            total_recv += mycount;
        }
    }

    if (rank != root) {
        /* all nodes except root send to parents */
//        OPAL_OUTPUT((ompi_coll_tuned_stream,
//                     "ompi_coll_tuned_gather_intra_binomial rank %d send %d count %d\n",
//                     rank, bmtree->tree_prev, total_recv));

        MPI_CHECK( err = MPI_Send( ptmp, total_recv, sdtype,
                                   bmtree->tree_prev,
                                   GATHER_TAG, comm) );

//        err = MCA_PML_CALL(send(ptmp, total_recv, sdtype,
//                                bmtree->tree_prev,
//                                MCA_COLL_BASE_TAG_GATHER,
//                                MCA_PML_BASE_SEND_STANDARD, comm));
        if (MPI_SUCCESS != err) { goto err_hndl; }
    }

    if (rank == root) {
        if (root != 0) {
            /* rotate received data on root if root != 0 */
            memcpy((char *) rbuf + rextent*root*rcount, ptmp, rextent * rcount*(size - root));

//            err = ompi_datatype_copy_content_same_ddt(rdtype, rcount*(size - root),
//                                                      (char *) rbuf + rextent*root*rcount, ptmp);
            if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }

            memcpy((char *) rbuf, ptmp + rextent*rcount*(size-root), rcount*root*rextent);

//            err = ompi_datatype_copy_content_same_ddt(rdtype, rcount*root,
//                                                      (char *) rbuf, ptmp + rextent*rcount*(size-root));
            if (MPI_SUCCESS != err) { line = __LINE__; goto err_hndl; }

            free(tempbuf);
        }
    } else if (!(vrank % 2)) {
        /* other non-leaf nodes */
        free(tempbuf);
    }
    return MPI_SUCCESS;

 err_hndl:
    if (NULL != tempbuf)
        free(tempbuf);

    return err;
}
