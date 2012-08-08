#ifndef AD_CEPH_H
#define AD_CEPH_H

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "adio.h"

struct ad_ceph_file {
	struct ceph_mount_info *cmount;
	int fd;
};

extern void ADIOI_CEPH_Open(ADIO_File fd, int *error_code);
extern void ADIOI_CEPH_Close(ADIO_File fd, int *error_code);

#endif
