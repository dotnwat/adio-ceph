#include "ad_ceph.h"

void ADIOI_CEPH_Close(ADIO_File fd, int *error_code)
{
    struct ad_ceph_file *cfile;
    int ret;

    cfile = fd->fs_ptr;

    ret = ceph_close(cfile->cmount, cfile->fd);
    if (ret) {
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                MPIR_ERR_RECOVERABLE, "close", __LINE__,
                MPI_ERR_UNKNOWN, "ceph close", 0);
        return;
    }

    *error_code = MPI_SUCCESS;
}
