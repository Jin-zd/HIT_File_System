#include "../include/newfs.h"

extern struct newfs_super super;

struct newfs_inode* newfs_alloc_inode(struct newfs_dentry * dentry) {
    struct newfs_inode* inode;
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int ino_cursor  = 0;
    int is_find_free = 0;

    for (byte_cursor = 0; byte_cursor < super.ino_map_blks * super.sz_blks; byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < 8; bit_cursor++) {
            if((super.inode_map[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                super.inode_map[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free = 1;           
                break;
            }
            ino_cursor++;
        }
        if (is_find_free) {
            break;
        }
    }

    if (!is_find_free || ino_cursor == super.ino_max) {
        return NULL;
    }
        
    inode = (struct newfs_inode*)malloc(sizeof(struct newfs_inode));
    inode->ino  = ino_cursor; 
    inode->size = 0;
                                                      /* dentry指向inode */
    dentry->inode = inode;
    dentry->ino   = inode->ino;
                                                      /* inode指回dentry */
    inode->dentry = dentry;

    inode->dir_cnt = 0;
    inode->dentrys = NULL;
    
    if (inode->dentry->ftype == NEWFS_TYPE_FILE) {
        inode->data = (int *)malloc(NEWFS_DATA_PER_FILE * super.sz_blks);
    }

    return inode;
}


int newfs_sync_inode(struct newfs_inode * inode) {
    struct newfs_inode_d  inode_d;
    struct newfs_dentry*  dentry_cursor;
    struct newfs_dentry_d dentry_d;
    inode_d.ino   = inode->ino;
    inode_d.dno   = inode->dno;
    inode_d.size  = inode->size;
    inode_d.dir_cnt  = inode->dir_cnt;
    int offset;
    newfs_driver_write(super.ino_offset + inode->ino * sizeof(struct newfs_inode_d), &inode_d, sizeof(struct newfs_inode_d));
    if (inode->dentry->ftype == NEWFS_TYPE_DIR) {                          
        dentry_cursor = inode->dentrys;
        if (dentry_cursor != NULL) {
            offset = super.data_offset + dentry_cursor->dno * super.sz_blks;
        }
        while (dentry_cursor != NULL)
        {
            
            memcpy(dentry_d.name, dentry_cursor->name, MAX_NAME_LEN);
            dentry_d.ftype = dentry_cursor->ftype;
            dentry_d.ino = dentry_cursor->ino;
            dentry_d.dno = dentry_cursor->dno;
            newfs_driver_write(offset, &dentry_d, sizeof(struct newfs_dentry_d));
            
            if (dentry_cursor->inode != NULL) {
                newfs_sync_inode(dentry_cursor->inode);
            }

            dentry_cursor = dentry_cursor->brother;
            offset += sizeof(struct newfs_dentry_d);
        }
    } else if (inode->dentry->ftype == NEWFS_TYPE_FILE) {
        
    }
    return 0;
}


void newfs_read_inode_remount(struct newfs_dentry * dentry, int ino) {
    struct newfs_inode* inode = (struct newfs_inode*)malloc(sizeof(struct newfs_inode));
    struct newfs_inode_d inode_d;
    struct newfs_dentry* sub_dentry;
    struct newfs_dentry_d dentry_d;
    int    dir_cnt = 0, i, offset;

    offset = super.ino_offset + ino * sizeof(struct newfs_inode_d);
    newfs_driver_read(offset, &inode_d, sizeof(struct newfs_inode_d));
    inode->dir_cnt = 0;
    inode->ino = inode_d.ino;
    inode->dno = inode_d.dno;
    inode->size = inode_d.size;
    inode->dentry = dentry;
    inode->dentrys = NULL;
    
    if (inode->dentry->ftype == NEWFS_TYPE_DIR) {
        dir_cnt = inode_d.dir_cnt;
        offset = super.data_offset + inode->dno * super.sz_blks;
        for (i = 0; i < dir_cnt; i++) {
            int read_offset = offset + i * sizeof(struct newfs_dentry_d);
            newfs_driver_read(read_offset, &dentry_d, sizeof(struct newfs_dentry_d));
            sub_dentry = new_dentry(dentry_d.name, dentry_d.ftype);
            sub_dentry->parent = inode->dentry;
            sub_dentry->ino    = dentry_d.ino; 
            sub_dentry->dno    = dentry_d.dno;
            if (inode->dentrys == NULL) {
                inode->dentrys = sub_dentry;
            } else {
                sub_dentry->brother = inode->dentrys;
                inode->dentrys = sub_dentry;
            }
            inode->dir_cnt++;
            newfs_read_inode_remount(sub_dentry, sub_dentry->ino);
        }
    } else if (inode->dentry->ftype == NEWFS_TYPE_FILE) {

    }
    dentry->inode = inode;
}

struct newfs_inode* newfs_read_inode(struct newfs_dentry * dentry, int ino) {
    struct newfs_inode* inode = (struct newfs_inode*)malloc(sizeof(struct newfs_inode));
    struct newfs_inode_d inode_d;
    struct newfs_dentry* sub_dentry;
    struct newfs_dentry_d dentry_d;
    int    dir_cnt = 0, i, offset;

    offset = super.ino_offset + ino * sizeof(struct newfs_inode_d);
    newfs_driver_read(offset, &inode_d, sizeof(struct newfs_inode_d));
    inode->dir_cnt = 0;
    inode->ino = inode_d.ino;
    inode->dno = inode_d.dno;
    inode->size = inode_d.size;
    inode->dentry = dentry;
    inode->dentrys = NULL;
    
    if (inode->dentry->ftype == NEWFS_TYPE_DIR) {
        dir_cnt = inode_d.dir_cnt;
        offset = super.data_offset + dentry->dno * super.sz_blks;
        for (i = 0; i < dir_cnt; i++) {
            newfs_driver_read(offset + i * sizeof(struct newfs_dentry_d), &dentry_d, sizeof(struct newfs_dentry_d));
            sub_dentry = new_dentry(dentry_d.name, dentry_d.ftype);
            sub_dentry->parent = inode->dentry;
            sub_dentry->ino    = dentry_d.ino; 
            sub_dentry->dno    = dentry_d.dno;
            if (inode->dentrys == NULL) {
                inode->dentrys = sub_dentry;
            } else {
                sub_dentry->brother = inode->dentrys;
                inode->dentrys = sub_dentry;
            }
            inode->dir_cnt++;
        }
    } else if (inode->dentry->ftype == NEWFS_TYPE_FILE) {

    }
    return inode;
}


int newfs_alloc_dentry(struct newfs_inode* inode, struct newfs_dentry* dentry) {
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int dno_cursor  = 0;
    int is_find_free = 0;

    if (inode->dir_cnt == 0) {
    	for (byte_cursor = 0; byte_cursor < super.data_map_blks * super.sz_blks; byte_cursor++) {
        	for (bit_cursor = 0; bit_cursor < 8; bit_cursor++) {
            	if((super.data_map[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                	super.data_map[byte_cursor] |= (0x1 << bit_cursor);
                	is_find_free = 1;           
                	break;
            	}
            	dno_cursor++;
        	}
        	if (is_find_free) {
        	    break;
        	}
    	}
    	dentry->dno = dno_cursor;
    } else {
        dentry->dno = inode->dentrys->dno;
    }
    inode->dno = dentry->dno;

    if (inode->dentrys == NULL) {
        inode->dentrys = dentry;
    } else {
        dentry->brother = inode->dentrys;
        inode->dentrys = dentry;
    }
    inode->dir_cnt++;
    return inode->dir_cnt;
}

int newfs_driver_read(int offset, void *out_content, int size) {
    int offset_aligned_down = NEWFS_ROUND_DOWN(offset, super.sz_io);
    int bias = offset - offset_aligned_down;
    int offset_aligned_up = NEWFS_ROUND_UP(offset_aligned_down + size + bias, super.sz_io);
    
    int blk_num = (offset_aligned_up - offset_aligned_down) / super.sz_io;
    char *buf = (char *)malloc(super.sz_io * blk_num);

    ddriver_seek(super.fd, offset_aligned_down, SEEK_SET);
    for (int i = 0; i < blk_num; i++) {
        ddriver_read(super.fd, buf + i * super.sz_io, super.sz_io);
    }
    memcpy(out_content, buf + bias, size);
    free(buf);
    return size;
}

int newfs_driver_write(int offset, void *in_content, int size) {
    int offset_aligned_down = NEWFS_ROUND_DOWN(offset, super.sz_io);
    int bias = offset - offset_aligned_down;
    int offset_aligned_up = NEWFS_ROUND_UP(offset_aligned_down + size + bias, super.sz_io);
    
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

struct newfs_dentry* newfs_lookup(const char * path, int* is_find, int* is_root) {
    struct newfs_dentry* dentry_cursor = super.root_dentry;
    struct newfs_dentry* dentry_ret = NULL;
    struct newfs_inode*  inode; 
    int total_lvl = newfs_calc_lvl(path);
    int lvl = 0;
    int is_hit;
    char* name = NULL;
    char* path_cpy = (char*)malloc(sizeof(path));
    *is_root = 0;
    strcpy(path_cpy, path);

    if (total_lvl == 0) {                           /* 根目录 */
        *is_find = 1;
        *is_root = 1;
        dentry_ret = super.root_dentry;
    }
    name = strtok(path_cpy, "/");       
    while (name)
    {   
        lvl++;
        if (dentry_cursor->inode == NULL) {           /* Cache机制 */
            newfs_read_inode(dentry_cursor, dentry_cursor->ino);
        }

        inode = dentry_cursor->inode;

        if (inode->dentry->ftype == NEWFS_TYPE_FILE && lvl < total_lvl) {
            dentry_ret = inode->dentry;
            break;
        }
        if (inode->dentry->ftype == NEWFS_TYPE_DIR) {
            dentry_cursor = inode->dentrys;
            is_hit        = 0;

            while (dentry_cursor)   /* 遍历子目录项 */
            {
                if (memcmp(dentry_cursor->name, name, strlen(name)) == 0) {
                    is_hit = 1;
                    break;
                }
                dentry_cursor = dentry_cursor->brother;
            }
            
            if (!is_hit) {
                *is_find = 0;
                dentry_ret = inode->dentry;
                break;
            }

            if (is_hit && lvl == total_lvl) {
                *is_find = 1;
                dentry_ret = dentry_cursor;
                break;
            }
        }
        name = strtok(NULL, "/"); 
    }

    if (dentry_ret->inode == NULL) {
        dentry_ret->inode = newfs_read_inode(dentry_ret, dentry_ret->ino);
    }
    
    return dentry_ret;
}

int newfs_calc_lvl(const char * path) {
    // char* path_cpy = (char *)malloc(strlen(path));
    // strcpy(path_cpy, path);
    char* str = path;
    int   lvl = 0;
    if (strcmp(path, "/") == 0) {
        return lvl;
    }
    while (*str != NULL) {
        if (*str == '/') {
            lvl++;
        }
        str++;
    }
    return lvl;
}


char* newfs_get_fname(const char* path) {
    char ch = '/';
    char *q = strrchr(path, ch) + 1;
    return q;
}

struct newfs_dentry* newfs_get_dentry(struct newfs_inode * inode, int dir) {
    struct newfs_dentry* dentry_cursor = inode->dentrys;
    int    cnt = 0;
    while (dentry_cursor)
    {
        if (dir == cnt) {
            return dentry_cursor;
        }
        cnt++;
        dentry_cursor = dentry_cursor->brother;
    }
    return NULL;
}

int newfs_mount(struct custom_options options) {
    super.fd = ddriver_open(options.device);

    int sz_disk;
    ddriver_ioctl(super.fd, IOC_REQ_DEVICE_SIZE,  &sz_disk);
    ddriver_ioctl(super.fd, IOC_REQ_DEVICE_IO_SZ, &super.sz_io);
    super.sz_blks = super.sz_io * 2;
    super.blks_nums = sz_disk / super.sz_blks;
    super.ino_max = 614;

    struct newfs_dentry*  root_dentry;
    root_dentry = new_dentry("/", NEWFS_TYPE_DIR); 

    struct newfs_super_d super_d;
    newfs_driver_read(0, &super_d, sizeof(struct newfs_super_d));

    int is_init = 0;
    if (super_d.magic != NEWFS_MAGIC) {
        super_d.sz_usage = 0;

        super_d.sb_blks = 1;
        super_d.sb_offset = 0;
        
        super_d.ino_map_blks = 1;
        super_d.ino_map_offset =super_d.sb_offset + super_d.sb_blks * super.sz_blks;

        super_d.data_map_blks = 1;
        super_d.data_map_offset = super_d.ino_map_offset + super_d.ino_map_blks * super.sz_blks;

        super_d.ino_blks = 12;
        super_d.ino_offset = super_d.data_map_offset + super_d.data_map_blks * super.sz_blks;

        super_d.data_blks = super.blks_nums - 16;
        super_d.data_offset = super_d.ino_offset + super_d.ino_blks * super.sz_blks;

        is_init = 1;
    }

    super.sz_usage = super_d.sz_usage;
    super.sb_offset = super_d.sb_offset;
    super.sb_blks = super_d.sb_blks;
    super.ino_offset = super_d.ino_offset;
    super.ino_blks = super_d.ino_blks;
    super.ino_map_offset = super_d.ino_map_offset;
    super.ino_map_blks = super_d.ino_map_blks;
    super.data_map_offset = super_d.data_map_offset;
    super.data_map_blks = super_d.data_map_blks;
    super.data_offset = super_d.data_offset;
    super.data_blks = super_d.data_blks;

    super.inode_map = (uint8_t *)malloc(super.ino_map_blks * super.sz_blks);
    super.data_map = (uint8_t *)malloc(super.data_map_blks * super.sz_blks);

    newfs_driver_read(super.ino_map_offset, super.inode_map, super.ino_map_blks * super.sz_blks);
    newfs_driver_read(super.data_map_offset, super.data_map, super.data_map_blks * super.sz_blks);

    struct newfs_inode* root_inode;
    if (is_init == 1) {
        root_inode = newfs_alloc_inode(root_dentry);
        root_inode = newfs_read_inode(root_dentry, 0);
        root_inode = newfs_read_inode(root_dentry, 0);
        root_dentry->inode = root_inode;
    } else {
        newfs_read_inode_remount(root_dentry, 0);
    }
    super.root_dentry = root_dentry;

    return 0;
}

int newfs_umount() {
    struct newfs_super_d  super_d; 

    newfs_sync_inode(super.root_dentry->inode);     /* 从根节点向下刷写节点 */
                                                    
    super_d.magic           = NEWFS_MAGIC;
    super_d.sz_io           = super.sz_io;    
    super_d.sz_blks         = super.sz_blks;
    super_d.blks_nums       = super.blks_nums;
    super_d.sz_usage        = super.sz_usage;
    super_d.sb_offset       = super.sb_offset;
    super_d.sb_blks         = super.sb_blks;
    super_d.ino_offset      = super.ino_offset;
    super_d.ino_blks        = super.ino_blks;
    super_d.ino_map_offset  = super.ino_map_offset;
    super_d.ino_map_blks    = super.ino_map_blks;
    super_d.data_map_offset = super.data_map_offset;
    super_d.data_map_blks   = super.data_map_blks;
    super_d.data_offset     = super.data_offset;
    super_d.data_blks       = super.data_blks;


    newfs_driver_write(0, &super_d, sizeof(struct newfs_super_d));
    newfs_driver_write(super_d.ino_map_offset, super.inode_map, super.ino_map_blks * super.sz_blks);
    newfs_driver_write(super_d.data_map_offset, super.data_map, super.data_map_blks * super.sz_blks);

    free(super.inode_map);
    free(super.data_map);
    ddriver_close(super.fd);

    return 0;
}