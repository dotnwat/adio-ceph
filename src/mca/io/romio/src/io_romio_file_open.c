/*
 *  $HEADER$
 */

#include "ompi_config.h"

#include "communicator/communicator.h"
#include "info/info.h"
#include "file/file.h"
#include "request/request.h"

#include "io_romio.h"


int
mca_io_romio_file_open (ompi_communicator_t *comm,
                        char *filename,
                        int amode,
                        ompi_info_t *info,
                        ompi_file_t *fh)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_open)(comm, filename, amode, info, 
                                      &data->romio_fh);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_close (ompi_file_t *fh)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;

    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_close) (&data->romio_fh);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_set_size (ompi_file_t *fh,
                            MPI_Offset size)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_set_size) (data->romio_fh, size);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}

int
mca_io_romio_file_preallocate (ompi_file_t *fh,
                               MPI_Offset size)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_preallocate) (data->romio_fh, size);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_get_size (ompi_file_t *fh,
                            MPI_Offset * size)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_get_size) (data->romio_fh, size);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_get_amode (ompi_file_t *fh,
                             int *amode)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_get_amode) (data->romio_fh, amode);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_set_info (ompi_file_t *fh,
                            ompi_info_t *info)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_set_info) (data->romio_fh, info);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_get_info (ompi_file_t *fh,
                            ompi_info_t ** info_used)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_get_info) (data->romio_fh, info_used);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_set_view (ompi_file_t *fh,
                            MPI_Offset disp,
                            ompi_datatype_t *etype,
                            ompi_datatype_t *filetype,
                            char *datarep,
                            ompi_info_t *info)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret =
        ROMIO_PREFIX(MPI_File_set_view) (data->romio_fh, disp, etype, filetype,
                                        datarep, info);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_get_view (ompi_file_t *fh,
                            MPI_Offset * disp,
                            ompi_datatype_t ** etype,
                            ompi_datatype_t ** filetype,
                            char *datarep)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret =
        ROMIO_PREFIX(MPI_File_get_view) (data->romio_fh, disp, etype, filetype,
                                        datarep);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;

}


int
mca_io_romio_file_get_type_extent (ompi_file_t *fh,
                                   ompi_datatype_t *datatype,
                                   MPI_Aint * extent)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret =
        ROMIO_PREFIX(MPI_File_get_type_extent) (data->romio_fh, datatype, extent);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_set_atomicity (ompi_file_t *fh,
                                 int flag)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_set_atomicity) (data->romio_fh, flag);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}

int
mca_io_romio_file_get_atomicity (ompi_file_t *fh,
                                 int *flag)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_get_atomicity) (data->romio_fh, flag);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}

int
mca_io_romio_file_sync (ompi_file_t *fh)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_sync) (data->romio_fh);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_seek_shared (ompi_file_t *fh,
                               MPI_Offset offset,
                               int whence)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_seek_shared) (data->romio_fh, offset, whence);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_get_position_shared (ompi_file_t *fh,
                                       MPI_Offset * offset)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_get_position_shared) (data->romio_fh, offset);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_seek (ompi_file_t *fh,
                        MPI_Offset offset,
                        int whence)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_seek) (data->romio_fh, offset, whence);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_get_position (ompi_file_t *fh,
                                MPI_Offset * offset)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_get_position) (data->romio_fh, offset);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}


int
mca_io_romio_file_get_byte_offset (ompi_file_t *fh,
                                   MPI_Offset offset,
                                   MPI_Offset * disp)
{
    int         ret;
    mca_io_romio_data_t *data;

    data = (mca_io_romio_data_t *) fh->f_io_selected_data;
    OMPI_THREAD_LOCK (&mca_io_romio_mutex);
    ret = ROMIO_PREFIX(MPI_File_get_byte_offset) (data->romio_fh, offset, disp);
    OMPI_THREAD_UNLOCK (&mca_io_romio_mutex);

    return ret;
}
