#include "../include/newfs.h"

extern struct newfs_super super;
extern struct custom_options newfs_options;

int newfs_mount(struct custom_options options) {
    super.fd = ddriver_open(newfs_options.device);

    int sz_disk;
    ddriver_ioctl(super.fd, IOC_REQ_DEVICE_SIZE,  &sz_disk);
    ddriver_ioctl(super.fd, IOC_REQ_DEVICE_IO_SZ, &super.sz_io);
    super.sz_blks = super.sz_io * 2;
    super.blks_nums = sz_disk / super.sz_blks;

    struct sfs_dentry*  root_dentry;
    root_dentry = new_dentry("/", NEWFS_TYPE_DIR); 
    newfs_driver_read()
}


int newfs_driver_read(int offset, void *out_content, int size) {
    int offset_aligned_down = NEWFS_ROUND_DOWN(offset, super.sz_io);
    int offset_aligned_up = NEWFS_ROUND_UP(offset + size, super.sz_io);
    int bias = offset - offset_aligned_down;
    
    ddriver_seek(super.fd, offset_aligned_down, SEEK_SET);
    int blk_num = (offset_aligned_up - offset_aligned_down) / super.sz_io;
    char *buf = (char *)malloc(super.sz_io * blk_num);
    for (int i = 0; i < blk_num; i++) {
        ddriver_read(super.fd, buf + i * super.sz_io, super.sz_io);
    }
    memcpy(out_content, buf + bias, size);
    free(buf);
    return size;
}

int newfs_driver_write(int offset, void *in_content, int size) {
    int offset_aligned_down = NEWFS_ROUND_DOWN(offset, super.sz_io);
    int offset_aligned_up = NEWFS_ROUND_UP(offset + size, super.sz_io);
    int bias = offset - offset_aligned_down;
    int blk_num = (offset_aligned_up - offset_aligned_down) / super.sz_io;

    char *buf = (char *)malloc(super.sz_io * blk_num);
    newfs_driver_read(offset_aligned_down, buf, super.sz_io * blk_num);
    memcpy(buf + bias, in_content, size);
    ddriver_seek(super.fd, offset_aligned_down, SEEK_SET);
    for (int i = 0; i < blk_num; i++) {
        ddriver_write(super.fd, buf + i * super.sz_io, super.sz_io);
    }
    free(buf);
    return size;
}
