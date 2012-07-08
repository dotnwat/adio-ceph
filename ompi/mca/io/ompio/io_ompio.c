/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2007 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008-2012 University of Houston. All rights reserved.
 * Copyright (c) 2011      Cisco Systems, Inc. All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"

#include "ompi/runtime/params.h"
#include "ompi/communicator/communicator.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/mca/topo/topo.h"
#include "opal/datatype/opal_convertor.h"
#include "opal/datatype/opal_datatype.h"
#include "ompi/datatype/ompi_datatype.h"
#include "ompi/info/info.h"
#include "ompi/request/request.h"

#include <math.h>
#include <unistd.h>

#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h> /* or <sys/vfs.h> */ 
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif



#include "io_ompio.h"

int ompi_io_ompio_set_file_defaults (mca_io_ompio_file_t *fh)
{

    if (NULL != fh) {
        ompi_datatype_t *types[2], *default_file_view;
        int blocklen[2] = {1, 1};
        OPAL_PTRDIFF_TYPE d[2], base;
        int i;

        fh->f_io_array = NULL;
        fh->f_perm = OMPIO_PERM_NULL;
        fh->f_flags = 0;
        fh->f_bytes_per_agg = mca_io_ompio_bytes_per_agg;
        fh->f_datarep = strdup ("native");

        fh->f_offset = 0;
        fh->f_disp = 0;
        fh->f_position_in_file_view = 0;
        fh->f_index_in_file_view = 0;
        fh->f_total_bytes = 0;


        fh->f_procs_in_group = NULL;

        fh->f_procs_per_group = -1;

	ompi_datatype_create_contiguous(1048576, &ompi_mpi_byte.dt, &default_file_view);
	ompi_datatype_commit (&default_file_view);
		
	fh->f_etype = &ompi_mpi_byte.dt;
	fh->f_filetype =  default_file_view;
	
	
        /* Default file View */
        fh->f_iov_type = MPI_DATATYPE_NULL;
        fh->f_stripe_size = mca_io_ompio_bytes_per_agg;
	/*Decoded iovec of the file-view*/
	fh->f_decoded_iov = NULL;
       
	mca_io_ompio_set_view_internal(fh,
				       0,
				       &ompi_mpi_byte.dt,
				       default_file_view,
				       "native",
				       fh->f_info);
    

	/*Create a derived datatype for the created iovec */
	types[0] = &ompi_mpi_long.dt;
        types[1] = &ompi_mpi_long.dt;

        d[0] = (OPAL_PTRDIFF_TYPE) fh->f_decoded_iov; 
        d[1] = (OPAL_PTRDIFF_TYPE) &fh->f_decoded_iov[0].iov_len;	

        base = d[0];
        for (i=0 ; i<2 ; i++) {
            d[i] -= base;
        }

        ompi_datatype_create_struct (2,
                                     blocklen,
                                     d,
                                     types,
                                     &fh->f_iov_type);
        ompi_datatype_commit (&fh->f_iov_type);

        return OMPI_SUCCESS;
    }
    else {
        return OMPI_ERROR;
    }
}

int ompi_io_ompio_generate_current_file_view (mca_io_ompio_file_t *fh,
                                              size_t max_data,
                                              struct iovec **f_iov,
                                              int *iov_count)
{




    struct iovec *iov = NULL;
    size_t bytes_to_write;
    size_t sum_previous_counts = 0;
    int j, k;
    int block = 1;
 
   /*Merging if the data is split-up */
    int merge = 0, i;
    int merge_count = 0;    
    struct iovec *merged_iov = NULL;
    size_t merge_length = 0;
    IOVBASE_TYPE * merge_offset = 0;






    /* allocate an initial iovec, will grow if needed */
    iov = (struct iovec *) malloc 
        (OMPIO_IOVEC_INITIAL_SIZE * sizeof (struct iovec));
    if (NULL == iov) {
        opal_output(1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    sum_previous_counts = fh->f_position_in_file_view;
    j = fh->f_index_in_file_view;
    bytes_to_write = max_data;
    k = 0;
    
    while (bytes_to_write) {
        OPAL_PTRDIFF_TYPE disp;
        /* reallocate if needed */
        if (OMPIO_IOVEC_INITIAL_SIZE*block <= k) {
            block ++;
            iov = (struct iovec *)realloc
                (iov, OMPIO_IOVEC_INITIAL_SIZE *block *sizeof(struct iovec));
            if (NULL == iov) {
                opal_output(1, "OUT OF MEMORY\n");
                return OMPI_ERR_OUT_OF_RESOURCE;
            }
        }

        if (fh->f_decoded_iov[j].iov_len - 
            (fh->f_total_bytes - sum_previous_counts) <= 0) {
            sum_previous_counts += fh->f_decoded_iov[j].iov_len;
            j = j + 1;
            if (j == (int)fh->f_iov_count) {
                j = 0;
                sum_previous_counts = 0;
                fh->f_offset += fh->f_view_extent;
                fh->f_position_in_file_view = sum_previous_counts;
                fh->f_index_in_file_view = j;
                fh->f_total_bytes = 0;
            }
        }
	
        disp = (OPAL_PTRDIFF_TYPE)(fh->f_decoded_iov[j].iov_base) + 
            (fh->f_total_bytes - sum_previous_counts);
        iov[k].iov_base = (IOVBASE_TYPE *)(disp + fh->f_offset);

        if ((fh->f_decoded_iov[j].iov_len - 
             (fh->f_total_bytes - sum_previous_counts)) 
            >= bytes_to_write) {
            iov[k].iov_len = bytes_to_write;
        }
        else {
            iov[k].iov_len =  fh->f_decoded_iov[j].iov_len - 
                (fh->f_total_bytes - sum_previous_counts);
        }

        fh->f_total_bytes += iov[k].iov_len;
        bytes_to_write -= iov[k].iov_len;
        k = k + 1;
    }
    fh->f_position_in_file_view = sum_previous_counts;
    fh->f_index_in_file_view = j;

    
    /* Merging Contiguous entries in case of using default fileview*/
    merged_iov = (struct iovec *)malloc(k*sizeof(struct iovec));
    for(i=0;i<k;i++){
	if(k != i+1){
	    if(((OPAL_PTRDIFF_TYPE)iov[i].iov_base +
		(OPAL_PTRDIFF_TYPE)iov[i].iov_len)==
	       (OPAL_PTRDIFF_TYPE)iov[i+1].iov_base) {
		if(!merge){
		    merge_offset = (IOVBASE_TYPE *)iov[i].iov_base;
		}
		merge_length += (size_t)iov[i].iov_len;
		merge++;
	    }
	    else{ /*If there are many contiguous blocks... with breaks in-between*/
		if (merge){
		    merged_iov[merge_count].iov_base =(IOVBASE_TYPE *)merge_offset;
		    /*1 has to be added as k goes till i+1*/
		    merged_iov[merge_count].iov_len = merge_length+1;
		    merge_count++;
		    merge = 0;
		}
		else{ /* Non-contiguous entry also has to be added!,
			 Handles cases where there are only non-contiguous
			 entries and a mix*/
		    merged_iov[merge_count].iov_base = iov[i].iov_base;
		    merged_iov[merge_count].iov_len = iov[i].iov_len;
		    merge_count++;
		}
	    }
	}
	else{/*When everything is contiguous*/
	    if (merge){
		merged_iov[merge_count].iov_base = (IOVBASE_TYPE *)merge_offset;
		merged_iov[merge_count].iov_len = merge_length+1048576;
		merge_count++;
		merge = 0;
	    }
	    else{ /* When there is nothing to be merged */	    
		merged_iov[merge_count].iov_base = iov[i].iov_base;
		merged_iov[merge_count].iov_len = iov[i].iov_len;
		merge_count++;
	    }
		
	}
	
    }
    
    #if 0
    printf("%d: Number of new_entries : %d\n", fh->f_rank, merge_count);
    for (i=0;i<merge_count;i++){
	printf("%d: OFFSET: %d, length: %ld\n",
	       fh->f_rank, merged_iov[i].iov_base, merged_iov[i].iov_len);
    }
    #endif
    *iov_count = merge_count;
    *f_iov = merged_iov;



    return OMPI_SUCCESS;
}

int ompi_io_ompio_set_explicit_offset (mca_io_ompio_file_t *fh,
                                       OMPI_MPI_OFFSET_TYPE offset)
{
    int i = 0;
    int k = 0;

    fh->f_offset = fh->f_view_extent * 
        ((offset*fh->f_etype_size - fh->f_disp) / fh->f_view_size);

    fh->f_position_in_file_view = 0;

    fh->f_total_bytes = (offset*fh->f_etype_size - fh->f_disp) % fh->f_view_size;

    fh->f_index_in_file_view = 0;
    i = fh->f_total_bytes;

    k = 0;
    while (1) {
        k += fh->f_decoded_iov[fh->f_index_in_file_view].iov_len;
        if (i >= k) {
            i = i - fh->f_decoded_iov[fh->f_index_in_file_view].iov_len;
            fh->f_position_in_file_view += 
                fh->f_decoded_iov[fh->f_index_in_file_view].iov_len;
            fh->f_index_in_file_view = fh->f_index_in_file_view+1;
        }
        else {
            break;
        }
    }

    return OMPI_SUCCESS;
}

int ompi_io_ompio_decode_datatype (mca_io_ompio_file_t *fh, 
                                   ompi_datatype_t *datatype,
                                   int count,
                                   void *buf,
                                   size_t *max_data,
                                   struct iovec **iov,
                                   uint32_t *iovec_count)
{                      


    
    opal_convertor_t convertor;
    size_t remaining_length = 0;
    uint32_t i;
    uint32_t temp_count;
    struct iovec * temp_iov;
    size_t temp_data;
    

    opal_convertor_clone (fh->f_convertor, &convertor, 0);

    if (OMPI_SUCCESS != opal_convertor_prepare_for_send (&convertor, 
                                                         &(datatype->super),
                                                         count,
                                                         buf)) {
        opal_output (1, "Cannot attach the datatype to a convertor\n");
        return OMPI_ERROR;
    }
    remaining_length = count * datatype->super.size;

    temp_count = OMPIO_IOVEC_INITIAL_SIZE;
    temp_iov = (struct iovec*)malloc(temp_count * sizeof(struct iovec));
    if (NULL == temp_iov) {
        opal_output (1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    while (0 == opal_convertor_raw(&convertor, 
				   temp_iov,
                                   &temp_count, 
                                   &temp_data)) {
/* #if 0 */
        printf ("%d: New raw extraction (iovec_count = %d, max_data = %d)\n",
                fh->f_rank,temp_count, temp_data);
        for (i = 0; i < temp_count; i++) {
            printf ("%d: \t{%p, %d}\n",fh->f_rank,
		    temp_iov[i].iov_base,
		    temp_iov[i].iov_len);
        }
/* #endif */

        *iovec_count = *iovec_count + temp_count;
        *max_data = *max_data + temp_data;
        *iov = (struct iovec *) realloc (*iov, *iovec_count * sizeof(struct iovec));
        if (NULL == *iov) {
            opal_output(1, "OUT OF MEMORY\n");
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
        for (i=0 ; i<temp_count ; i++) {
            (*iov)[i+(*iovec_count-temp_count)].iov_base = temp_iov[i].iov_base;
            (*iov)[i+(*iovec_count-temp_count)].iov_len = temp_iov[i].iov_len;
        }

        remaining_length -= temp_data;
        temp_count = OMPIO_IOVEC_INITIAL_SIZE;
    }
#if 0
    printf ("%d: LAST raw extraction (iovec_count = %d, max_data = %d)\n",
            fh->f_rank,temp_count, temp_data);
    for (i = 0; i < temp_count; i++) {
        printf ("%d: \t offset[%d]: %ld; length[%d]: %ld\n", fh->f_rank,i,temp_iov[i].iov_base, i,temp_iov[i].iov_len);
    }
#endif
    *iovec_count = *iovec_count + temp_count;
    *max_data = *max_data + temp_data;
    *iov = (struct iovec *) realloc (*iov, *iovec_count * sizeof(struct iovec));
    if (NULL == *iov) {
        opal_output(1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    for (i=0 ; i<temp_count ; i++) {
        (*iov)[i+(*iovec_count-temp_count)].iov_base = temp_iov[i].iov_base;
        (*iov)[i+(*iovec_count-temp_count)].iov_len = temp_iov[i].iov_len;
    }

    remaining_length -= temp_data;
    #if 0
    if (0 == fh->f_rank) {

        printf ("%d Entries: \n",*iovec_count);
        for (i=0 ; i<*iovec_count ; i++) {
            printf ("\t{%p, %d}\n", 
                    (*iov)[i].iov_base, 
                    (*iov)[i].iov_len);
        }
    }
    #endif
    if (remaining_length != 0) {
        printf( "Not all raw description was been extracted (%lu bytes missing)\n",
                (unsigned long) remaining_length );
    }

    if (NULL != temp_iov) {
        free (temp_iov);
        temp_iov = NULL;
    }

    return OMPI_SUCCESS;
}

int ompi_io_ompio_sort (mca_io_ompio_io_array_t *io_array,
                        int num_entries,
                        int *sorted)
{
    int i = 0;
    int j = 0;
    int left = 0;
    int right = 0;
    int largest = 0;
    int heap_size = num_entries - 1;
    int temp = 0;
    unsigned char done = 0;
    int* temp_arr = NULL;

    temp_arr = (int*)malloc(num_entries*sizeof(int));
    if (NULL == temp_arr) {
        opal_output (1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    temp_arr[0] = 0;
    for (i = 1; i < num_entries; ++i) {
        temp_arr[i] = i;
    }
    /* num_entries can be a large no. so NO RECURSION */
    for (i = num_entries/2-1 ; i>=0 ; i--) {
        done = 0;
        j = i;
        largest = j;

        while (!done) {
            left = j*2+1;
            right = j*2+2;
            if ((left <= heap_size) && 
                (io_array[temp_arr[left]].offset > io_array[temp_arr[j]].offset)) {
                largest = left;
            }
            else {
                largest = j;
            }
            if ((right <= heap_size) && 
                (io_array[temp_arr[right]].offset > 
                 io_array[temp_arr[largest]].offset)) {
                largest = right;
            }
            if (largest != j) {
                temp = temp_arr[largest];
                temp_arr[largest] = temp_arr[j];
                temp_arr[j] = temp;
                j = largest;
            }
            else {
                done = 1;
            }
        }
    }

    for (i = num_entries-1; i >=1; --i) {
        temp = temp_arr[0];
        temp_arr[0] = temp_arr[i];
        temp_arr[i] = temp;            
        heap_size--;            
        done = 0;
        j = 0;
        largest = j;

        while (!done) {
            left =  j*2+1;
            right = j*2+2;
            
            if ((left <= heap_size) && 
                (io_array[temp_arr[left]].offset > 
                 io_array[temp_arr[j]].offset)) {
                largest = left;
            }
            else {
                largest = j;
            }
            if ((right <= heap_size) && 
                (io_array[temp_arr[right]].offset > 
                 io_array[temp_arr[largest]].offset)) {
                largest = right;
            }
            if (largest != j) {
                temp = temp_arr[largest];
                temp_arr[largest] = temp_arr[j];
                temp_arr[j] = temp;
                j = largest;
            }
            else {
                done = 1;
            }
        }
        sorted[i] = temp_arr[i];
    }
    sorted[0] = temp_arr[0];

    if (NULL != temp_arr) {
        free(temp_arr);
        temp_arr = NULL;
    }
    return OMPI_SUCCESS;
}

int ompi_io_ompio_sort_iovec (struct iovec *iov,
                              int num_entries,
                              int *sorted)
{
    int i = 0;
    int j = 0;
    int left = 0;
    int right = 0;
    int largest = 0;
    int heap_size = num_entries - 1;
    int temp = 0;
    unsigned char done = 0;
    int* temp_arr = NULL;

    if (0 == num_entries) {
        return OMPI_SUCCESS;
    }

    temp_arr = (int*)malloc(num_entries*sizeof(int));
    if (NULL == temp_arr) {
        opal_output (1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    temp_arr[0] = 0;
    for (i = 1; i < num_entries; ++i) {
        temp_arr[i] = i;
    }
    /* num_entries can be a large no. so NO RECURSION */
    for (i = num_entries/2-1 ; i>=0 ; i--) {
        done = 0;
        j = i;
        largest = j;

        while (!done) {
            left = j*2+1;
            right = j*2+2;
            if ((left <= heap_size) && 
                (iov[temp_arr[left]].iov_base > iov[temp_arr[j]].iov_base)) {
                largest = left;
            }
            else {
                largest = j;
            }
            if ((right <= heap_size) && 
                (iov[temp_arr[right]].iov_base > 
                 iov[temp_arr[largest]].iov_base)) {
                largest = right;
            }
            if (largest != j) {
                temp = temp_arr[largest];
                temp_arr[largest] = temp_arr[j];
                temp_arr[j] = temp;
                j = largest;
            }
            else {
                done = 1;
            }
        }
    }

    for (i = num_entries-1; i >=1; --i) {
        temp = temp_arr[0];
        temp_arr[0] = temp_arr[i];
        temp_arr[i] = temp;            
        heap_size--;            
        done = 0;
        j = 0;
        largest = j;

        while (!done) {
            left =  j*2+1;
            right = j*2+2;
            
            if ((left <= heap_size) && 
                (iov[temp_arr[left]].iov_base > 
                 iov[temp_arr[j]].iov_base)) {
                largest = left;
            }
            else {
                largest = j;
            }
            if ((right <= heap_size) && 
                (iov[temp_arr[right]].iov_base > 
                 iov[temp_arr[largest]].iov_base)) {
                largest = right;
            }
            if (largest != j) {
                temp = temp_arr[largest];
                temp_arr[largest] = temp_arr[j];
                temp_arr[j] = temp;
                j = largest;
            }
            else {
                done = 1;
            }
        }
        sorted[i] = temp_arr[i];
    }
    sorted[0] = temp_arr[0];

    if (NULL != temp_arr) {
        free(temp_arr);
        temp_arr = NULL;
    }
    return OMPI_SUCCESS;
}

int ompi_io_ompio_set_aggregator_props (mca_io_ompio_file_t *fh,
                                        int num_aggregators,
                                        size_t bytes_per_proc)
{


    int j;
    int root_offset=0;
    int ndims, i=1, n=0, total_groups=0;
    int *dims=NULL, *periods=NULL, *coords=NULL, *coords_tmp=NULL;
    int procs_per_node = 1; /* MSC TODO - Figure out a way to get this info */
    size_t max_bytes_per_proc = 0;
 


    /*
    OMPI_MPI_OFFSET_TYPE temp;
    int global_flag, flag;
    */
    fh->f_flags |= OMPIO_AGGREGATOR_IS_SET;

    if (-1 == num_aggregators) {
        /* Determine Topology Information */
        if (fh->f_comm->c_flags & OMPI_COMM_CART) {
            fh->f_comm->c_topo->topo_cartdim_get(fh->f_comm, &ndims);

            dims = (int*)malloc (ndims * sizeof(int));
            if (NULL == dims) {
                opal_output (1, "OUT OF MEMORY\n");
                return OMPI_ERR_OUT_OF_RESOURCE;
            }

            periods = (int*)malloc (ndims * sizeof(int));
            if (NULL == periods) {
                opal_output (1, "OUT OF MEMORY\n");
                return OMPI_ERR_OUT_OF_RESOURCE;
            }

            coords = (int*)malloc (ndims * sizeof(int));
            if (NULL == coords) {
                opal_output (1, "OUT OF MEMORY\n");
                return OMPI_ERR_OUT_OF_RESOURCE;
            }

            coords_tmp = (int*)malloc (ndims * sizeof(int));
            if (NULL == coords_tmp) {
                opal_output (1, "OUT OF MEMORY\n");
                return OMPI_ERR_OUT_OF_RESOURCE;
            }

            fh->f_comm->c_topo->topo_cart_get(fh->f_comm, ndims, dims, periods, coords);

            /*
              printf ("NDIMS = %d\n", ndims);
              for (j=0 ; j<ndims; j++) {
              printf ("%d:  dims[%d] = %d     period[%d] = %d    coords[%d] = %d\n",
              fh->f_rank,j,dims[j],j,periods[j],j,coords[j]);
              }
            */

            while (1) {
                if (fh->f_size/dims[0]*i >= procs_per_node) {
                    fh->f_procs_per_group = fh->f_size/dims[0]*i;
                    break;
                }
                i++;
            }

            total_groups = ceil((float)fh->f_size/fh->f_procs_per_group);

            if ((coords[0]/i + 1) == total_groups && 0 != (total_groups%i)) {
                fh->f_procs_per_group = (fh->f_size/dims[0]) * (total_groups%i);
            }
            /*
            printf ("BEFORE ADJUSTMENT: %d ---> procs_per_group = %d   total_groups = %d\n", 
                    fh->f_rank, fh->f_procs_per_group, total_groups);
            */
            /* check if the current grouping needs to be expanded or shrinked */
            if ((size_t)mca_io_ompio_bytes_per_agg < 
                bytes_per_proc * fh->f_procs_per_group) {

                root_offset = ceil ((float)mca_io_ompio_bytes_per_agg/bytes_per_proc);
                if (fh->f_procs_per_group/root_offset != coords[1]/root_offset) {
                    fh->f_procs_per_group = root_offset;
                }
                else {
                    fh->f_procs_per_group = fh->f_procs_per_group%root_offset;
                }
            }
            else if ((size_t)mca_io_ompio_bytes_per_agg > 
                     bytes_per_proc * fh->f_procs_per_group) {
                i = ceil ((float)mca_io_ompio_bytes_per_agg/
                          (bytes_per_proc * fh->f_procs_per_group));
                root_offset = fh->f_procs_per_group * i;

                if (fh->f_size/root_offset != fh->f_rank/root_offset) {
                    fh->f_procs_per_group = root_offset;
                }
                else {
                    fh->f_procs_per_group = fh->f_size%root_offset;
                }
            }
            /*
            printf ("AFTER ADJUSTMENT: %d (%d) ---> procs_per_group = %d\n", 
                    fh->f_rank, coords[1], fh->f_procs_per_group);
            */
            fh->f_procs_in_group = (int*)malloc (fh->f_procs_per_group * sizeof(int));
            if (NULL == fh->f_procs_in_group) {
                opal_output (1, "OUT OF MEMORY\n");
                return OMPI_ERR_OUT_OF_RESOURCE;
            }

            for (j=0 ; j<fh->f_size ; j++) {
                fh->f_comm->c_topo->topo_cart_coords (fh->f_comm, j, ndims, coords_tmp);
                if (coords_tmp[0]/i == coords[0]/i) {
                    if ((coords_tmp[1]/root_offset)*root_offset == 
                        (coords[1]/root_offset)*root_offset) {
                        fh->f_procs_in_group[n] = j;
                        n++;
                    }
                }
            }

            fh->f_aggregator_index = 0;

            /*
            if (fh->f_procs_in_group[fh->f_aggregator_index] == fh->f_rank)  {
                for (j=0 ; j<fh->f_procs_per_group; j++) {
                    printf ("%d: Proc %d: %d\n", fh->f_rank, j, fh->f_procs_in_group[j]);
                }
            }
            */

            if (NULL != dims) {
                free (dims);
                dims = NULL;
            }
            if (NULL != periods) {
                free (periods);
                periods = NULL;
            }
            if (NULL != coords) {
                free (coords);
                coords = NULL;
            }
            if (NULL != coords_tmp) {
                free (coords_tmp);
                coords_tmp =  NULL;
            }
            return OMPI_SUCCESS;
        }

        /*
        temp = fh->f_iov_count;
        fh->f_comm->c_coll.coll_bcast (&temp,
                                       1,
                                       MPI_LONG,
                                       OMPIO_ROOT,
                                       fh->f_comm,
                                       fh->f_comm->c_coll.coll_bcast_module);

        if (temp != fh->f_iov_count) {
            flag = 0;
        }
        else {
            flag = 1;
        }
        fh->f_comm->c_coll.coll_allreduce (&flag,
                                           &global_flag,
                                           1,
                                           MPI_INT,
                                           MPI_MIN,
                                           fh->f_comm,
                                           fh->f_comm->c_coll.coll_allreduce_module);
        */
        if (fh->f_flags & OMPIO_UNIFORM_FVIEW) {
            OMPI_MPI_OFFSET_TYPE *start_offsets = NULL;
            OMPI_MPI_OFFSET_TYPE stride = 0;

            if (OMPIO_ROOT == fh->f_rank) {
                start_offsets = malloc (fh->f_size * sizeof(OMPI_MPI_OFFSET_TYPE));
            }

            fh->f_comm->c_coll.coll_gather (&fh->f_decoded_iov[0].iov_base,
                                            1,
                                            MPI_LONG,
                                            start_offsets,
                                            1,
                                            MPI_LONG,
                                            OMPIO_ROOT,
                                            fh->f_comm,
                                            fh->f_comm->c_coll.coll_gather_module);
            if (OMPIO_ROOT == fh->f_rank) {
                stride = start_offsets[1] - start_offsets[0];
                for (i=2 ; i<fh->f_size ; i++) {
                    if (stride != start_offsets[i]-start_offsets[i-1]) {
                        break;
                    }
                }
            }

            if (NULL != start_offsets) {
                free (start_offsets);
                start_offsets = NULL;
            }

            fh->f_comm->c_coll.coll_bcast (&i,
                                           1,
                                           MPI_INT,
                                           OMPIO_ROOT,
                                           fh->f_comm,
                                           fh->f_comm->c_coll.coll_bcast_module);

            fh->f_procs_per_group = i;
            max_bytes_per_proc = bytes_per_proc;
        }
        else {
            fh->f_procs_per_group = 1;
            fh->f_comm->c_coll.coll_allreduce (&bytes_per_proc,
                                               &max_bytes_per_proc,
                                               1,
                                               MPI_LONG,
                                               MPI_MAX,
                                               fh->f_comm,
                                               fh->f_comm->c_coll.coll_allreduce_module);
        }
        /*
        printf ("BEFORE ADJUSTMENT: %d ---> procs_per_group = %d\n", 
                fh->f_rank, fh->f_procs_per_group);
        
        printf ("COMPARING %d   to  %d x %d = %d\n", 
                mca_io_ompio_bytes_per_agg,
                bytes_per_proc,
                fh->f_procs_per_group,
                fh->f_procs_per_group*bytes_per_proc);
        */
        /* check if the current grouping needs to be expanded or shrinked */
        if ((size_t)mca_io_ompio_bytes_per_agg < 
            max_bytes_per_proc * fh->f_procs_per_group) {
            root_offset = ceil ((float)mca_io_ompio_bytes_per_agg/max_bytes_per_proc);

            if (fh->f_procs_per_group/root_offset != 
                (fh->f_rank%fh->f_procs_per_group)/root_offset) {
                fh->f_procs_per_group = root_offset;
            }
            else {
                fh->f_procs_per_group = fh->f_procs_per_group%root_offset;
            }
        }
        else if ((size_t)mca_io_ompio_bytes_per_agg > 
                 max_bytes_per_proc * fh->f_procs_per_group) {
            i = ceil ((float)mca_io_ompio_bytes_per_agg/
                      (max_bytes_per_proc * fh->f_procs_per_group));
            root_offset = fh->f_procs_per_group * i;
            i = root_offset;

            if (root_offset > fh->f_size) {
                root_offset = fh->f_size;
            }

            if (fh->f_size/root_offset != fh->f_rank/root_offset) {
                fh->f_procs_per_group = root_offset;
            }
            else {
                fh->f_procs_per_group = fh->f_size%root_offset;
            }
        }
        /*
        printf ("AFTER ADJUSTMENT: %d ---> procs_per_group = %d\n", 
                fh->f_rank, fh->f_procs_per_group);
        */
        fh->f_procs_in_group = (int*)malloc 
            (fh->f_procs_per_group * sizeof(int));
        if (NULL == fh->f_procs_in_group) {
            opal_output (1, "OUT OF MEMORY\n");
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
        for (j=0 ; j<fh->f_size ; j++) {
            if (j/i == fh->f_rank/i) {
                if (((j%i)/root_offset)*root_offset == 
                    ((fh->f_rank%i)/root_offset)*root_offset) {
                    fh->f_procs_in_group[n] = j;
                    n++;
                }
            }
        }

        fh->f_aggregator_index = 0;
        /*
          if (fh->f_procs_in_group[fh->f_aggregator_index] == fh->f_rank)  {
          for (j=0 ; j<fh->f_procs_per_group; j++) {
          printf ("%d: Proc %d: %d\n", fh->f_rank, j, fh->f_procs_in_group[j]);
          }
          }
        */
        return OMPI_SUCCESS;
    }

    /* calculate the offset at which each group of processes will start */
    root_offset = ceil ((float)fh->f_size/num_aggregators);

    /* calculate the number of processes in the local group */
    if (fh->f_size/root_offset != fh->f_rank/root_offset) {
        fh->f_procs_per_group = root_offset;
    }
    else {
        fh->f_procs_per_group = fh->f_size%root_offset;
    }

    fh->f_procs_in_group = (int*)malloc (fh->f_procs_per_group * sizeof(int));
    if (NULL == fh->f_procs_in_group) {
        opal_output (1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    for (j=0 ; j<fh->f_procs_per_group ; j++) {
        fh->f_procs_in_group[j] = (fh->f_rank/root_offset) * root_offset + j;
    }

    fh->f_aggregator_index = 0;

    return OMPI_SUCCESS;
}


/*Based on ROMIO's domain partitioning implementaion 
Series of functions implementations for two-phase implementation
Functions to support Domain partitioning and aggregator
selection for two_phase .
This is commom to both two_phase_read and write. */

int ompi_io_ompio_domain_partition (mca_io_ompio_file_t *fh,
				    OMPI_MPI_OFFSET_TYPE *start_offsets,
				    OMPI_MPI_OFFSET_TYPE *end_offsets,
				    OMPI_MPI_OFFSET_TYPE *min_st_offset_ptr,
				    OMPI_MPI_OFFSET_TYPE **fd_st_ptr,
				    OMPI_MPI_OFFSET_TYPE **fd_end_ptr,
				    int min_fd_size,
				    OMPI_MPI_OFFSET_TYPE *fd_size_ptr,
				    int striping_unit,
				    int nprocs_for_coll){
    

   

    
    OMPI_MPI_OFFSET_TYPE min_st_offset, max_end_offset, *fd_start=NULL, *fd_end=NULL, fd_size;
    int i;



    min_st_offset = start_offsets[0];
    max_end_offset = end_offsets[0];

    for (i=0; i< fh->f_size; i++){
	min_st_offset = OMPIO_MIN(min_st_offset, start_offsets[i]);
	max_end_offset = OMPIO_MAX(max_end_offset, end_offsets[i]);
	
    }

    fd_size = ((max_end_offset - min_st_offset + 1) + nprocs_for_coll - 1)/nprocs_for_coll; 
    
    if (fd_size < min_fd_size)
	fd_size = min_fd_size;
    
/*    printf("fd_size :%lld, min_fd_size : %d\n", fd_size, min_fd_size);*/
    
    *fd_st_ptr = (OMPI_MPI_OFFSET_TYPE *)
	malloc(nprocs_for_coll*sizeof(OMPI_MPI_OFFSET_TYPE)); 

    if ( NULL == *fd_st_ptr ) {
	return OMPI_ERR_OUT_OF_RESOURCE;
    }

    *fd_end_ptr = (OMPI_MPI_OFFSET_TYPE *)
	malloc(nprocs_for_coll*sizeof(OMPI_MPI_OFFSET_TYPE)); 

    if ( NULL == *fd_end_ptr ) {
	return OMPI_ERR_OUT_OF_RESOURCE;
    }

    
    fd_start = *fd_st_ptr;
    fd_end = *fd_end_ptr;
    
    
    if (striping_unit > 0){
	
	/* Lock Boundary based domain partitioning */
	int rem_front, rem_back;
	OMPI_MPI_OFFSET_TYPE end_off;
	
	printf("striping unit based partitioning!\n ");
	fd_start[0] = min_st_offset;
        end_off     = fd_start[0] + fd_size;
        rem_front   = end_off % striping_unit;
        rem_back    = striping_unit - rem_front;
        if (rem_front < rem_back) 
		end_off -= rem_front;
        else                      
		end_off += rem_back;
        fd_end[0] = end_off - 1;
    
	/* align fd_end[i] to the nearest file lock boundary */
        for (i=1; i<nprocs_for_coll; i++) {
            fd_start[i] = fd_end[i-1] + 1;
            end_off     = min_st_offset + fd_size * (i+1);
            rem_front   = end_off % striping_unit;
            rem_back    = striping_unit - rem_front;
            if (rem_front < rem_back) 
		    end_off -= rem_front;
            else                      
		    end_off += rem_back;
            fd_end[i] = end_off - 1;
        }
        fd_end[nprocs_for_coll-1] = max_end_offset;
    }
    else{
	fd_start[0] = min_st_offset;
        fd_end[0] = min_st_offset + fd_size - 1;
	
        for (i=1; i<nprocs_for_coll; i++) {
            fd_start[i] = fd_end[i-1] + 1;
            fd_end[i] = fd_start[i] + fd_size - 1;
        }

    }
    
    for (i=0; i<nprocs_for_coll; i++) {
	if (fd_start[i] > max_end_offset)
	    fd_start[i] = fd_end[i] = -1;
	if (fd_end[i] > max_end_offset)
	    fd_end[i] = max_end_offset;
    }
    
    *fd_size_ptr = fd_size;
    *min_st_offset_ptr = min_st_offset;
 
    return OMPI_SUCCESS;
}



int ompi_io_ompio_calc_aggregator(mca_io_ompio_file_t *fh,
				  OMPI_MPI_OFFSET_TYPE off, 
 				  OMPI_MPI_OFFSET_TYPE min_off,
				  OMPI_MPI_OFFSET_TYPE *len,
				  OMPI_MPI_OFFSET_TYPE fd_size,
				  OMPI_MPI_OFFSET_TYPE *fd_start,
				  OMPI_MPI_OFFSET_TYPE *fd_end,
				  int striping_unit,
				  int num_aggregators, 
				  int *aggregator_list)
{


    int rank_index, rank;
    OMPI_MPI_OFFSET_TYPE avail_bytes;
    
    rank_index = (int) ((off - min_off + fd_size)/ fd_size - 1);
    
    if (striping_unit > 0){
	rank_index = 0;
	while (off > fd_end[rank_index]) rank_index++;
    }
    

    if (rank_index >= num_aggregators || rank_index < 0) {
       fprintf(stderr,
	       "Error in ompi_io_ompio_calcl_aggregator():");
       fprintf(stderr,
	       "rank_index(%d) >= num_aggregators(%d)fd_size=%lld off=%lld\n",
	       rank_index,num_aggregators,fd_size,off);
       MPI_Abort(MPI_COMM_WORLD, 1);
    }


    avail_bytes = fd_end[rank_index] + 1 - off;
    if (avail_bytes < *len){
	*len = avail_bytes;
    }
    

    rank = aggregator_list[rank_index];
    
    #if 0
    printf("rank : %d, rank_index : %d\n",rank, rank_index);
    #endif
    
    return rank;
}

int ompi_io_ompio_calc_others_requests(mca_io_ompio_file_t *fh,
				       int count_my_req_procs,
				       int *count_my_req_per_proc,
				       mca_io_ompio_access_array_t *my_req,
				       int *count_others_req_procs_ptr,
				       mca_io_ompio_access_array_t **others_req_ptr)
{
    

    int *count_others_req_per_proc=NULL, count_others_req_procs;
    int i,j, ret=OMPI_SUCCESS;
    MPI_Request *requests=NULL;
    mca_io_ompio_access_array_t *others_req=NULL;
    
    count_others_req_per_proc = (int *)malloc(fh->f_size*sizeof(int));

    if ( NULL == count_others_req_per_proc ) {
	return OMPI_ERR_OUT_OF_RESOURCE;
    }
    
    /* Change it to the ompio specific alltoall in coll module : VVN*/
    ret =  fh->f_comm->c_coll.coll_alltoall (count_my_req_per_proc,
					     1,
					     MPI_INT,
					     count_others_req_per_proc, 
					     1,
					     MPI_INT,
					     fh->f_comm, 
					     fh->f_comm->c_coll.coll_alltoall_module);
    if ( OMPI_SUCCESS != ret ) {
	return ret;
    }
    
#if 0
    for( i = 0; i< fh->f_size; i++){
	printf("my: %d, others: %d\n",count_my_req_per_proc[i],
	       count_others_req_per_proc[i]);
	
    }
#endif

    *others_req_ptr = (mca_io_ompio_access_array_t  *) malloc 
	(fh->f_size*sizeof(mca_io_ompio_access_array_t)); 
    others_req = *others_req_ptr;
    
    count_others_req_procs = 0;
    for (i=0; i<fh->f_size; i++) {
	if (count_others_req_per_proc[i]) {
	    others_req[i].count = count_others_req_per_proc[i];
	    others_req[i].offsets = (OMPI_MPI_OFFSET_TYPE *)
		malloc(count_others_req_per_proc[i]*sizeof(OMPI_MPI_OFFSET_TYPE));
	    others_req[i].lens = (int *)
		malloc(count_others_req_per_proc[i]*sizeof(int));
	    others_req[i].mem_ptrs = (MPI_Aint *)
		malloc(count_others_req_per_proc[i]*sizeof(MPI_Aint)); 
	    count_others_req_procs++;
	}
	else
	    others_req[i].count = 0;
    }
    
    
    requests = (MPI_Request *)
	malloc(1+2*(count_my_req_procs+count_others_req_procs)*
	       sizeof(MPI_Request)); 

    if ( NULL == requests ) {
	ret = OMPI_ERR_OUT_OF_RESOURCE;
	goto exit;
    }
    
    j = 0;
    for (i=0; i<fh->f_size; i++){
	if (others_req[i].count){
            ret = MCA_PML_CALL(irecv(others_req[i].offsets,
				     others_req[i].count,
				     MPI_LONG,
				     i,
				     i+fh->f_rank,
				     fh->f_comm,
				     &requests[j]));
	    if ( OMPI_SUCCESS != ret  ) {
		goto exit;
	    }
	    	    
	    j++;

	    ret = MCA_PML_CALL(irecv(others_req[i].lens,
				     others_req[i].count,
				     MPI_INT,
				     i,
				     i+fh->f_rank+1,
				     fh->f_comm,
				     &requests[j]));
	    if ( OMPI_SUCCESS != ret  ) {
		goto exit;
	    }
		       
	    j++;
	}
    }


    for (i=0; i < fh->f_size; i++) {
	if (my_req[i].count) {
	    ret = MCA_PML_CALL(isend(my_req[i].offsets,
				     my_req[i].count,
				     MPI_LONG,
				     i,
				     i+fh->f_rank,
				     MCA_PML_BASE_SEND_STANDARD, 
				     fh->f_comm,
				     &requests[j]));	
	    if ( OMPI_SUCCESS != ret  ) {
		goto exit;
	    }

	    j++;
	    ret = MCA_PML_CALL(isend(my_req[i].lens,
				     my_req[i].count,
				     MPI_INT,
				     i,
				     i+fh->f_rank+1,
				     MCA_PML_BASE_SEND_STANDARD, 
				     fh->f_comm,
				     &requests[j]));	
	    if ( OMPI_SUCCESS != ret  ) {
		goto exit;
	    }
    
	    j++;
	}
    }
    
    if (j) {
	ret = ompi_request_wait_all ( j, requests, MPI_STATUS_IGNORE );
	if ( OMPI_SUCCESS != ret  ) {
	    return ret;
	}
    }

    *count_others_req_procs_ptr = count_others_req_procs;

exit:
    if ( NULL != requests ) {
	free(requests);
    }
    if ( NULL != count_others_req_per_proc ) {
	free(count_others_req_per_proc);
    }


    return ret;
}


int ompi_io_ompio_calc_my_requests (mca_io_ompio_file_t *fh,
				    struct iovec *offset_len,
				    int contig_access_count,
				    OMPI_MPI_OFFSET_TYPE min_st_offset,
				    OMPI_MPI_OFFSET_TYPE *fd_start,
				    OMPI_MPI_OFFSET_TYPE *fd_end,
				    OMPI_MPI_OFFSET_TYPE fd_size,
				    int *count_my_req_procs_ptr,
				    int **count_my_req_per_proc_ptr,
				    mca_io_ompio_access_array_t **my_req_ptr,
				    int **buf_indices,
				    int striping_unit,
				    int num_aggregators,
				    int *aggregator_list)
{
    
    int *count_my_req_per_proc, count_my_req_procs;
    int *buf_idx;
    int i, l, proc;
    OMPI_MPI_OFFSET_TYPE fd_len, rem_len, curr_idx, off;
    mca_io_ompio_access_array_t *my_req;
    
    
    *count_my_req_per_proc_ptr = (int*)malloc(fh->f_size*sizeof(int)); 
    
    if ( NULL == count_my_req_per_proc_ptr ){
	return OMPI_ERR_OUT_OF_RESOURCE;
    }

    count_my_req_per_proc = *count_my_req_per_proc_ptr;
    
    for (i=0;i<fh->f_size;i++){
	count_my_req_per_proc[i] = 0;
    }
        
    buf_idx = (int *) malloc (fh->f_size * sizeof(int));
    
    if ( NULL == buf_idx ){ 
	return OMPI_ERR_OUT_OF_RESOURCE;
    }
    
    for (i=0; i < fh->f_size; i++) buf_idx[i] = -1;
    
    for (i=0;i<contig_access_count; i++){

	if (offset_len[i].iov_len==0)
	    continue;
	off = (OMPI_MPI_OFFSET_TYPE)offset_len[i].iov_base;
	fd_len = (OMPI_MPI_OFFSET_TYPE)offset_len[i].iov_len;
	proc = ompi_io_ompio_calc_aggregator(fh, off, min_st_offset, &fd_len, fd_size, 
					     fd_start, fd_end, striping_unit, num_aggregators,aggregator_list);
	count_my_req_per_proc[proc]++;
	rem_len = offset_len[i].iov_len - fd_len;
	
	while (rem_len != 0) {
	    off += fd_len; /* point to first remaining byte */
	    fd_len = rem_len; /* save remaining size, pass to calc */
	    proc = ompi_io_ompio_calc_aggregator(fh, off, min_st_offset, &fd_len, 
						 fd_size, fd_start, fd_end, striping_unit,
						 num_aggregators, aggregator_list);

	    count_my_req_per_proc[proc]++;
	    rem_len -= fd_len; /* reduce remaining length by amount from fd */
	}

    }
    
/*    printf("%d: fh->f_size : %d\n", fh->f_rank,fh->f_size);*/
    *my_req_ptr =  (mca_io_ompio_access_array_t *)
	malloc (fh->f_size * sizeof(mca_io_ompio_access_array_t));
    if ( NULL == *my_req_ptr ) {
	return OMPI_ERR_OUT_OF_RESOURCE;
    }
    my_req = *my_req_ptr;
    
    count_my_req_procs = 0;
    for (i = 0; i < fh->f_size; i++){
	if(count_my_req_per_proc[i]) {
	    my_req[i].offsets = (OMPI_MPI_OFFSET_TYPE *)
		malloc(count_my_req_per_proc[i] * sizeof(OMPI_MPI_OFFSET_TYPE));
	    
	    if ( NULL == my_req[i].offsets ) {
		return OMPI_ERR_OUT_OF_RESOURCE;
	    }

	    my_req[i].lens = (int *)
		malloc(count_my_req_per_proc[i] * sizeof(int));

	    if ( NULL == my_req[i].lens ) {
		return OMPI_ERR_OUT_OF_RESOURCE;
	    }
	    count_my_req_procs++;
	}
	my_req[i].count = 0; 
    }
    curr_idx = 0;
    for (i=0; i<contig_access_count; i++) { 
	if ((int)offset_len[i].iov_len == 0)
	    continue;
	off = (OMPI_MPI_OFFSET_TYPE)offset_len[i].iov_base;
	fd_len = (OMPI_MPI_OFFSET_TYPE)offset_len[i].iov_len;
 	proc = ompi_io_ompio_calc_aggregator(fh, off, min_st_offset, &fd_len,
					     fd_size, fd_start, fd_end,
					     striping_unit, num_aggregators,
					     aggregator_list);
	if (buf_idx[proc] == -1){
	    buf_idx[proc] = (int) curr_idx;
	}
	l = my_req[proc].count;
	curr_idx += fd_len;
	rem_len = offset_len[i].iov_len - fd_len;
	my_req[proc].offsets[l] = off;
	my_req[proc].lens[l] = (int)fd_len;
	my_req[proc].count++;

	while (rem_len != 0) {
	    off += fd_len;
	    fd_len = rem_len;
	    proc = ompi_io_ompio_calc_aggregator(fh, off, min_st_offset, &fd_len, 
						 fd_size, fd_start,
						 fd_end, striping_unit,
						 num_aggregators,
						 aggregator_list);
	    
	    if (buf_idx[proc] == -1){
		buf_idx[proc] = (int) curr_idx;
	    }
		    
	    l = my_req[proc].count;
	    curr_idx += fd_len;
	    rem_len -= fd_len;

	    my_req[proc].offsets[l] = off;
	    my_req[proc].lens[l] = (int) fd_len;
	    my_req[proc].count++;
 
	}
	
    }
    
  #if 0
    for (i=0; i<fh->f_size; i++) {
	if (count_my_req_per_proc[i] > 0) {
	    fprintf(stdout, "data needed from %d (count = %d):\n", i, 
		    my_req[i].count);
	    for (l=0; l < my_req[i].count; l++) {
		fprintf(stdout, " %d: off[%d] = %lld, len[%d] = %d\n", fh->f_rank, l,
		my_req[i].offsets[l], l, my_req[i].lens[l]);
	    }
	    fprintf(stdout, "%d: buf_idx[%d] = 0x%x\n", fh->f_rank, i, buf_idx[i]);
	}
    }
#endif
    
    
    *count_my_req_procs_ptr = count_my_req_procs;
    *buf_indices = buf_idx;
    
    return OMPI_SUCCESS;
}
/*Two-phase support functions ends here!*/				    

int ompi_io_ompio_break_file_view (mca_io_ompio_file_t *fh,
                                   struct iovec *iov,
                                   int count,
                                   int stripe_count,
                                   size_t stripe_size,
                                   struct iovec **broken_iov,
                                   int *broken_count)
{



    struct iovec *temp_iov = NULL;
    int i = 0;
    int k = 0;
    int block = 1;
    int broken = 0;
    size_t remaining = 0;
    size_t temp = 0;
    OPAL_PTRDIFF_TYPE current_offset = 0;


    /* allocate an initial iovec, will grow if needed */
    temp_iov = (struct iovec *) malloc 
        (count * sizeof (struct iovec));
    if (NULL == temp_iov) {
        opal_output(1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    while (i < count) {
        if (count*block <= k) {
            block ++;
            temp_iov = (struct iovec *)realloc
                (temp_iov, count * block *sizeof(struct iovec));
            if (NULL == temp_iov) {
                opal_output(1, "OUT OF MEMORY\n");
                return OMPI_ERR_OUT_OF_RESOURCE;
            }
        }
        if (0 == broken) {
            temp = (OPAL_PTRDIFF_TYPE)(iov[i].iov_base)%stripe_size;
            if ((stripe_size-temp) >= iov[i].iov_len) {
                temp_iov[k].iov_base = iov[i].iov_base;
                temp_iov[k].iov_len = iov[i].iov_len;
                i++;
                k++;
            }
            else {
                temp_iov[k].iov_base = iov[i].iov_base;
                temp_iov[k].iov_len = stripe_size-temp;
                current_offset = (OPAL_PTRDIFF_TYPE)(temp_iov[k].iov_base) + 
                    temp_iov[k].iov_len;
                remaining = iov[i].iov_len - temp_iov[k].iov_len;
                k++;
                broken ++;
            }
            continue;
        }
        temp = current_offset%stripe_size;
        if ((stripe_size-temp) >= remaining) {
            temp_iov[k].iov_base = (IOVBASE_TYPE *)current_offset;
            temp_iov[k].iov_len = remaining;
            i++;
            k++;
            broken = 0;
            current_offset = 0;
            remaining = 0;
        }
        else {
            temp_iov[k].iov_base = (IOVBASE_TYPE *)current_offset;
            temp_iov[k].iov_len = stripe_size-temp;
            current_offset += temp_iov[k].iov_len;
            remaining -= temp_iov[k].iov_len;
            k++;
            broken ++;
        }
    }
    *broken_iov = temp_iov;
    *broken_count = k;

    return 1;
}
int ompi_io_ompio_distribute_file_view (mca_io_ompio_file_t *fh,
                                        struct iovec *broken_iov,
                                        int broken_count,
                                        int num_aggregators,
                                        size_t stripe_size,
                                        int **fview_count,
                                        struct iovec **iov,
                                        int *count)
{


    int *num_entries = NULL;
    int *broken_index = NULL;
    int temp = 0;
    int *fview_cnt = NULL;
    int global_fview_count = 0;
    int i = 0;
    int *displs = NULL;
    int rc = OMPI_SUCCESS;
    struct iovec *global_fview = NULL;
    struct iovec **broken = NULL;
    MPI_Request *req=NULL, *sendreq=NULL;


    num_entries = (int *) malloc (sizeof (int) * num_aggregators);
    if (NULL == num_entries) {
        opal_output (1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    broken_index = (int *) malloc (sizeof (int) * num_aggregators);
    if (NULL == broken_index) {
        opal_output (1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    memset (num_entries, 0x0, num_aggregators * sizeof (int));
    memset (broken_index, 0x0, num_aggregators * sizeof (int));

    /* calculate how many entries in the broken iovec belong to each aggregator */
    for (i=0 ; i<broken_count ; i++) {
        temp = (int)((OPAL_PTRDIFF_TYPE)broken_iov[i].iov_base/stripe_size) % 
            num_aggregators;
        num_entries [temp] ++;
    }

    if (0 == fh->f_rank%fh->f_aggregator_index) {
        fview_cnt = (int *) malloc (sizeof (int) * fh->f_size);
        if (NULL == fview_cnt) {
            opal_output (1, "OUT OF MEMORY\n");
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
        req = (MPI_Request *)malloc (fh->f_size * sizeof(MPI_Request));
        if (NULL == req) {
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
    }

    sendreq = (MPI_Request *)malloc (num_aggregators * sizeof(MPI_Request));
    if (NULL == sendreq) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    /* gather at each aggregator how many entires from the broken file view it 
       expects from each process */
    if (0 == fh->f_rank%fh->f_aggregator_index) {
        for (i=0; i<fh->f_size ; i++) {
            rc = MCA_PML_CALL(irecv(&fview_cnt[i],
                                    1,
                                    MPI_INT,
                                    i,
                                    OMPIO_TAG_GATHER,
                                    fh->f_comm,
                                    &req[i]));
            if (OMPI_SUCCESS != rc) {
                goto exit;
            }
        }
    }

    for (i=0 ; i<num_aggregators ; i++) {
        rc = MCA_PML_CALL(isend(&num_entries[i],
                                1,
                                MPI_INT,
                                i*fh->f_aggregator_index,
                                OMPIO_TAG_GATHER,
                                MCA_PML_BASE_SEND_STANDARD, 
                                fh->f_comm,
                                &sendreq[i]));
        if (OMPI_SUCCESS != rc) {
            goto exit;
        }
    }

    if (0 == fh->f_rank%fh->f_aggregator_index) {
        rc = ompi_request_wait_all (fh->f_size, req, MPI_STATUSES_IGNORE);
        if (OMPI_SUCCESS != rc) {
            goto exit;
        }
    }
    rc = ompi_request_wait_all (num_aggregators, sendreq, MPI_STATUSES_IGNORE);
    if (OMPI_SUCCESS != rc) {
        goto exit;
    }

    /*
    for (i=0 ; i<num_aggregators ; i++) {
        fh->f_comm->c_coll.coll_gather (&num_entries[i],
                                        1,
                                        MPI_INT,
                                        fview_cnt,
                                        1,
                                        MPI_INT,
                                        i*fh->f_aggregator_index,
                                        fh->f_comm,
                                        fh->f_comm->c_coll.coll_gather_module);
    }
    */

    if (0 == fh->f_rank%fh->f_aggregator_index) {
        displs = (int*) malloc (fh->f_size * sizeof (int));
        if (NULL == displs) {
            opal_output (1, "OUT OF MEMORY\n");
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
        displs[0] = 0;
        global_fview_count = fview_cnt[0];
        for (i=1 ; i<fh->f_size ; i++) {
            global_fview_count += fview_cnt[i];
            displs[i] = displs[i-1] + fview_cnt[i-1];
        }

        if (global_fview_count) {
            global_fview = (struct iovec*)malloc (global_fview_count *
                                                  sizeof(struct iovec));
            if (NULL == global_fview) {
                opal_output (1, "OUT OF MEMORY\n");
                return OMPI_ERR_OUT_OF_RESOURCE;
            }
        }
    }

    broken = (struct iovec**)malloc (num_aggregators * sizeof(struct iovec *));
    if (NULL == broken) {
        opal_output (1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    for (i=0 ; i<num_aggregators ; i++) {
        broken[i] = NULL;
        if (0 != num_entries[i]) {
            broken[i] = (struct iovec*) malloc (num_entries[i] *
                                                sizeof (struct iovec));
            if (NULL == broken[i]) {
                opal_output (1, "OUT OF MEMORY\n");
                return OMPI_ERR_OUT_OF_RESOURCE;
            }
        }
    }

    for (i=0 ; i<broken_count ; i++) {
        temp = (int)((OPAL_PTRDIFF_TYPE)broken_iov[i].iov_base/stripe_size) % 
            num_aggregators;
        broken[temp][broken_index[temp]].iov_base = broken_iov[i].iov_base;
        broken[temp][broken_index[temp]].iov_len = broken_iov[i].iov_len;
        broken_index[temp] ++;
    }
    /*
    for (i=0 ; i<num_aggregators ; i++) {
        int j;
        for (j=0 ; j<num_entries[i] ; j++) {
            printf("%d->%d: OFFSET: %d   LENGTH: %d\n",
                   fh->f_rank,
                   i,
                   broken[i][j].iov_base,
                   broken[i][j].iov_len);
        }
    }
    sleep(1);
    */

    if (0 == fh->f_rank%fh->f_aggregator_index) {
        ptrdiff_t lb, extent;
        rc = ompi_datatype_get_extent(fh->f_iov_type, &lb, &extent);
        if (OMPI_SUCCESS != rc) {
                goto exit;
        }
        for (i=0; i<fh->f_size ; i++) {
            if (fview_cnt[i]) {
                char *ptmp;
                ptmp = ((char *) global_fview) + (extent * displs[i]);
                rc = MCA_PML_CALL(irecv(ptmp,
                                        fview_cnt[i],
                                        fh->f_iov_type,
                                        i,
                                        OMPIO_TAG_GATHERV,
                                        fh->f_comm,
                                        &req[i]));
                if (OMPI_SUCCESS != rc) {
                    goto exit;
                }
            }
        }
    }

    for (i=0 ; i<num_aggregators ; i++) {
        if (num_entries[i]) {
            rc = MCA_PML_CALL(isend(broken[i],
                                    num_entries[i],
                                    fh->f_iov_type,
                                    i*fh->f_aggregator_index,
                                    OMPIO_TAG_GATHERV,
                                    MCA_PML_BASE_SEND_STANDARD, 
                                    fh->f_comm,
                                    &sendreq[i]));
            if (OMPI_SUCCESS != rc) {
                goto exit;
            }
        }
    }

    if (0 == fh->f_rank%fh->f_aggregator_index) {
        for (i=0; i<fh->f_size ; i++) {
            if (fview_cnt[i]) {
                rc = ompi_request_wait (&req[i], MPI_STATUS_IGNORE);
                if (OMPI_SUCCESS != rc) {
                    goto exit;
                }
            }
        }
    }

    for (i=0; i<num_aggregators ; i++) {
        if (num_entries[i]) {
            rc = ompi_request_wait (&sendreq[i], MPI_STATUS_IGNORE);
            if (OMPI_SUCCESS != rc) {
                goto exit;
            }
        }
    }

    /*
    for (i=0 ; i<num_aggregators ; i++) {
        fh->f_comm->c_coll.coll_gatherv (broken[i],
                                         num_entries[i],
                                         fh->f_iov_type,
                                         global_fview,
                                         fview_cnt,
                                         displs,
                                         fh->f_iov_type,
                                         i*fh->f_aggregator_index,
                                         fh->f_comm,
                                         fh->f_comm->c_coll.coll_gatherv_module);
    }
    */
    /*
    for (i=0 ; i<global_fview_count ; i++) {
        printf("%d: OFFSET: %d   LENGTH: %d\n",
               fh->f_rank,
               global_fview[i].iov_base,
               global_fview[i].iov_len);
    }
    */
 exit:
    for (i=0 ; i<num_aggregators ; i++) {
        if (NULL != broken[i]) {
            free (broken[i]);
            broken[i] = NULL;
        }
    }
    if (NULL != req) {
        free (req);
    }
    if (NULL != sendreq) {
        free (sendreq);
    }
    if (NULL != broken) {
        free (broken);
        broken = NULL;
    }
    if (NULL != num_entries) {
        free (num_entries);
        num_entries = NULL;
    }
    if (NULL != broken_index) {
        free (broken_index);
        broken_index = NULL;
    }
    if (NULL != displs) {
        free (displs);
        displs = NULL;
    }

    *fview_count = fview_cnt;
    *iov = global_fview;    
    *count = global_fview_count;
    
    return rc;
}

int ompi_io_ompio_gather_data (mca_io_ompio_file_t *fh,
                               void *send_buf,
                               size_t total_bytes_sent,
                               int *bytes_sent,
                               struct iovec *broken_iovec,
                               int broken_index,
                               size_t partial,
                               void *global_buf,
                               int *bytes_per_process,
                               int *displs,
                               int num_aggregators,
                               size_t stripe_size)
{
    void **sbuf = NULL;
    size_t bytes_remaining;
    size_t *temp_position = NULL;
    size_t part;
    int current;
    int temp = 0;
    int i = 0;
    int rc = OMPI_SUCCESS;
    MPI_Request *req=NULL, *sendreq=NULL;


    current = broken_index;
    part = partial;

    sbuf = (void**) malloc (num_aggregators * sizeof(void *));
    if (NULL == sbuf) {
        opal_output (1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    temp_position = (size_t *) malloc (num_aggregators * sizeof(size_t));
    if (NULL == temp_position) {
        opal_output (1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    memset (temp_position, 0x0, num_aggregators * sizeof (size_t));

    for (i=0 ; i<num_aggregators ; i++) {
        sbuf[i] = NULL;
        if (0 != bytes_sent[i]) {
            sbuf[i] = (void *) malloc (bytes_sent[i]);
            if (NULL == sbuf[i]) {
                opal_output (1, "OUT OF MEMORY\n");
                return OMPI_ERR_OUT_OF_RESOURCE;
            }
        }
    }

    bytes_remaining = total_bytes_sent;

    while (bytes_remaining) {
        temp = (int)((OPAL_PTRDIFF_TYPE)broken_iovec[current].iov_base/stripe_size) 
            % num_aggregators;

        if (part) {
            if (bytes_remaining > part) {
                memcpy ((IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)sbuf[temp]+
                                         temp_position[temp]),
                        (IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)send_buf +
                                         (total_bytes_sent-bytes_remaining)),
                        part);
                bytes_remaining -= part;
                temp_position[temp] += part;
                part = 0;
                current ++;  
            }
            else  {
                memcpy ((IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)sbuf[temp]+
                                         temp_position[temp]),
                        (IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)send_buf +
                                         (total_bytes_sent-bytes_remaining)),
                        bytes_remaining);
                break;
            }
        }
        else {
            if (bytes_remaining > broken_iovec[current].iov_len) {
                memcpy ((IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)sbuf[temp]+
                                         temp_position[temp]),
                        (IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)send_buf +
                                         (total_bytes_sent-bytes_remaining)),
                        broken_iovec[current].iov_len);
                bytes_remaining -= broken_iovec[current].iov_len;
                temp_position[temp] += broken_iovec[current].iov_len;
                current ++;
            }
            else {
                memcpy ((IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)sbuf[temp]+
                                         temp_position[temp]),
                        (IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)send_buf +
                                         (total_bytes_sent-bytes_remaining)),
                        bytes_remaining);
                break;
            }
        }
    }

    sendreq = (MPI_Request *)malloc (num_aggregators * sizeof(MPI_Request));
    if (NULL == sendreq) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    if (0 == fh->f_rank%fh->f_aggregator_index) {
        req = (MPI_Request *)malloc (fh->f_size * sizeof(MPI_Request));
        if (NULL == req) {
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
        for (i=0; i<fh->f_size ; i++) {
            if (bytes_per_process[i]) {
                rc = MCA_PML_CALL(irecv((char *)global_buf + displs[i],
                                        bytes_per_process[i],
                                        MPI_BYTE,
                                        i,
                                        OMPIO_TAG_GATHERV,
                                        fh->f_comm,
                                        &req[i]));
                if (OMPI_SUCCESS != rc) {
                    goto exit;
                }
            }
        }
    }

    for (i=0 ; i<num_aggregators ; i++) {
        if (bytes_sent[i]) {
            rc = MCA_PML_CALL(isend(sbuf[i],
                                    bytes_sent[i],
                                    MPI_BYTE,
                                    i*fh->f_aggregator_index,
                                    OMPIO_TAG_GATHERV,
                                    MCA_PML_BASE_SEND_STANDARD, 
                                    fh->f_comm,
                                    &sendreq[i]));
            if (OMPI_SUCCESS != rc) {
                goto exit;
            }
        }
    }

    if (0 == fh->f_rank%fh->f_aggregator_index) {
        for (i=0; i<fh->f_size ; i++) {
            if (bytes_per_process[i]) {
                rc = ompi_request_wait (&req[i], MPI_STATUS_IGNORE);
                if (OMPI_SUCCESS != rc) {
                    goto exit;
                }
            }
        }
    }
    for (i=0; i<num_aggregators ; i++) {
        if (bytes_sent[i]) {
            rc = ompi_request_wait (&sendreq[i], MPI_STATUS_IGNORE);
            if (OMPI_SUCCESS != rc) {
                goto exit;
            }
        }
    }
    /*
    for (i=0 ; i<num_aggregators ; i++) {
        fh->f_comm->c_coll.coll_gatherv (sbuf[i],
                                         bytes_sent[i],
                                         MPI_BYTE,
                                         global_buf,
                                         bytes_per_process,
                                         displs,
                                         MPI_BYTE,
                                         i*fh->f_aggregator_index,
                                         fh->f_comm,
                                         fh->f_comm->c_coll.coll_gatherv_module);
    }
    */

 exit:
    for (i=0 ; i<num_aggregators ; i++) {
        if (NULL != sbuf[i]) {
            free (sbuf[i]);
            sbuf[i] = NULL;
        }
    }
    if (NULL != req) {
        free (req);
    }
    if (NULL != sendreq) {
        free (sendreq);
    }
    if (NULL != sbuf) {
        free (sbuf);
        sbuf = NULL;
    }
    if (NULL != temp_position) {
        free (temp_position);
        temp_position = NULL;
    }

    return rc;
}

int ompi_io_ompio_scatter_data (mca_io_ompio_file_t *fh,
                                void *receive_buf,
                                size_t total_bytes_recv,
                                int *bytes_received,
                                struct iovec *broken_iovec,
                                int broken_index,
                                size_t partial,
                                void *global_buf,
                                int *bytes_per_process,
                                int *displs,
                                int num_aggregators,
                                size_t stripe_size)
{
    void **rbuf = NULL;
    size_t bytes_remaining;
    size_t *temp_position = NULL;
    size_t part;
    int current;
    int temp = 0;
    int i = 0;
    int rc = OMPI_SUCCESS;
    MPI_Request *req=NULL, *recvreq=NULL;


    current = broken_index;
    part = partial;

    rbuf = (void**) malloc (num_aggregators * sizeof(void *));
    if (NULL == rbuf) {
        opal_output (1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    temp_position = (size_t *) malloc (num_aggregators * sizeof(size_t));
    if (NULL == temp_position) {
        opal_output (1, "OUT OF MEMORY\n");
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    memset (temp_position, 0x0, num_aggregators * sizeof (size_t));

    for (i=0 ; i<num_aggregators ; i++) {
        rbuf[i] = NULL;
        if (0 != bytes_received[i]) {
            rbuf[i] = (void *) malloc (bytes_received[i]);
            if (NULL == rbuf[i]) {
                opal_output (1, "OUT OF MEMORY\n");
                return OMPI_ERR_OUT_OF_RESOURCE;
            }
        }
    }

    recvreq = (MPI_Request *)malloc (num_aggregators * sizeof(MPI_Request));
    if (NULL == recvreq) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    for (i=0 ; i<num_aggregators ; i++) {
        if (bytes_received[i]) {
            rc = MCA_PML_CALL(irecv(rbuf[i],
                                    bytes_received[i],
                                    MPI_BYTE,
                                    i*fh->f_aggregator_index,
                                    OMPIO_TAG_SCATTERV,
                                    fh->f_comm,
                                    &recvreq[i]));
            if (OMPI_SUCCESS != rc) {
                goto exit;
            }
        }
    }

    if (0 == fh->f_rank%fh->f_aggregator_index) {
        req = (MPI_Request *)malloc (fh->f_size * sizeof(MPI_Request));
        if (NULL == req) {
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
        for (i=0; i<fh->f_size ; i++) {
            if (bytes_per_process[i]) {
                rc = MCA_PML_CALL(isend((char *)global_buf + displs[i],
                                        bytes_per_process[i],
                                        MPI_BYTE,
                                        i,
                                        OMPIO_TAG_SCATTERV,
                                        MCA_PML_BASE_SEND_STANDARD,
                                        fh->f_comm,
                                        &req[i]));
                if (OMPI_SUCCESS != rc) {
                    goto exit;
                }
            }
        }
    }

    for (i=0; i<num_aggregators ; i++) {
        if (bytes_received[i]) {
            rc = ompi_request_wait (&recvreq[i], MPI_STATUS_IGNORE);
            if (OMPI_SUCCESS != rc) {
                goto exit;
            }
        }
    }
    if (0 == fh->f_rank%fh->f_aggregator_index) {
        for (i=0; i<fh->f_size ; i++) {
            if (bytes_per_process[i]) {
                rc = ompi_request_wait (&req[i], MPI_STATUS_IGNORE);
                if (OMPI_SUCCESS != rc) {
                    goto exit;
                }
            }
        }
    }
    /*
    for (i=0 ; i<num_aggregators ; i++) {
        fh->f_comm->c_coll.coll_scatterv (global_buf,
                                          bytes_per_process,
                                          displs,
                                          MPI_BYTE,
                                          rbuf[i],
                                          bytes_received[i],
                                          MPI_BYTE,
                                          i*fh->f_aggregator_index,
                                          fh->f_comm,
                                          fh->f_comm->c_coll.coll_scatterv_module);
    }
    */
    bytes_remaining = total_bytes_recv;

    while (bytes_remaining) {
        temp = (int)((OPAL_PTRDIFF_TYPE)broken_iovec[current].iov_base/stripe_size) 
            % num_aggregators;

        if (part) {
            if (bytes_remaining > part) {
                memcpy ((IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)receive_buf +
                                         (total_bytes_recv-bytes_remaining)),
                        (IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)rbuf[temp]+
                                         temp_position[temp]),
                        part);
                bytes_remaining -= part;
                temp_position[temp] += part;
                part = 0;
                current ++;  
            }
            else  {
                memcpy ((IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)receive_buf +
                                         (total_bytes_recv-bytes_remaining)),
                        (IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)rbuf[temp]+
                                         temp_position[temp]),
                        bytes_remaining);
                break;
            }
        }
        else {
            if (bytes_remaining > broken_iovec[current].iov_len) {
                memcpy ((IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)receive_buf +
                                         (total_bytes_recv-bytes_remaining)),
                        (IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)rbuf[temp]+
                                         temp_position[temp]),
                        broken_iovec[current].iov_len);
                bytes_remaining -= broken_iovec[current].iov_len;
                temp_position[temp] += broken_iovec[current].iov_len;
                current ++;
            }
            else {
                memcpy ((IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)receive_buf +
                                         (total_bytes_recv-bytes_remaining)),
                        (IOVBASE_TYPE *)((OPAL_PTRDIFF_TYPE)rbuf[temp]+
                                         temp_position[temp]),
                        bytes_remaining);
                break;
            }
        }
    }

 exit:
    for (i=0 ; i<num_aggregators ; i++) {
        if (NULL != rbuf[i]) {
            free (rbuf[i]);
            rbuf[i] = NULL;
        }
    }
    if (NULL != req) {
        free (req);
    }
    if (NULL != recvreq) {
        free (recvreq);
    }
    if (NULL != rbuf) {
        free (rbuf);
        rbuf = NULL;
    }
    if (NULL != temp_position) {
        free (temp_position);
        temp_position = NULL;
    }

    return rc;
}


