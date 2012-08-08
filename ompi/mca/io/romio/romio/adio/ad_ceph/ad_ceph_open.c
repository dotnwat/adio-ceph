#include "ad_ceph.h"
#include "cephfs/libcephfs.h"

void ADIOI_CEPH_Open(ADIO_File fd, int *error_code)
{
    struct ad_ceph_file *cfile;
    char *conf_path;
    int ret, amode;

    cfile = ADIOI_Malloc(sizeof(*cfile));
    if (!cfile) {
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                MPIR_ERR_RECOVERABLE, "open", __LINE__,
                MPI_ERR_UNKNOWN, "Out of memory", 0);
        return;
    }

    ret = ceph_create(&cfile->cmount, NULL);
    if (ret) {
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                MPIR_ERR_RECOVERABLE, "open", __LINE__,
                MPI_ERR_UNKNOWN, "ceph create", 0);
        return;
    }

    /* FIXME */
    conf_path = "/home/nwatkins/Projects/ceph/src/ceph.conf";

    ret = ceph_conf_read_file(cfile->cmount, conf_path);
    if (ret) {
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                MPIR_ERR_RECOVERABLE, "open", __LINE__,
                MPI_ERR_UNKNOWN, "ceph read conf file", 0);
        return;
    }

    ret = ceph_mount(cfile->cmount, NULL);
    if (ret) {
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                MPIR_ERR_RECOVERABLE, "open", __LINE__,
                MPI_ERR_UNKNOWN, "ceph mount", 0);
        return;
    }

    amode = 0;
    if (fd->access_mode & ADIO_RDONLY)
        amode |= O_RDONLY;
    if (fd->access_mode & ADIO_WRONLY)
        amode |= O_WRONLY;
    if (fd->access_mode & ADIO_RDWR)
        amode |= O_RDWR;
    if (fd->access_mode & ADIO_EXCL)
        amode |= O_EXCL;
    if (fd->access_mode & ADIO_CREATE)
        amode |= O_CREAT;

    ret = ceph_open(cfile->cmount, fd->filename, amode, 0666);
    if (ret < 0) {
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                MPIR_ERR_RECOVERABLE, "open", __LINE__,
                MPI_ERR_UNKNOWN, "ceph open", 0);
        return;
    }

    cfile->fd = ret;

    fd->fs_ptr = cfile;

    *error_code = MPI_SUCCESS;
}
