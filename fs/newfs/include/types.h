#ifndef _TYPES_H_
#define _TYPES_H_

#define MAX_NAME_LEN    128     
#define INODE_NUM 12
#define NEWFS_DATA_PER_FILE 6

#define NEWFS_ROUND_DOWN(value, round)    ((value) % (round) == 0 ? (value) : ((value) / (round)) * (round))
#define NEWFS_ROUND_UP(value, round)      ((value) % (round) == 0 ? (value) : ((value) / (round) + 1) * (round))

struct custom_options {
	const char*        device;
};

typedef enum newfs_file_type {
    NEWFS_TYPE_FILE,
    NEWFS_TYPE_DIR,
} NEWFS_FILE_TYPE;

struct newfs_super {
    uint32_t magic;
    int      fd;
    /* TODO: Define yourself */
    int      sz_io;
    int      sz_blks;
    int      blks_nums;
    int      sz_usage;

    int      sb_offset;
    int      sb_blks;

    int      ino_offset;
    int      ino_blks;

    int      ino_map_offset;
    int      ino_map_blks;

    int      data_map_offset;
    int      data_map_blks;

    int      data_offset;
    int      data_blks;

    uint8_t* inode_map;
    uint8_t* data_map;

    int ino_max;

    struct newfs_dentry *root_dentry;
};

struct newfs_inode {
    uint32_t        ino;
    /* TODO: Define yourself */
    int             size;
    int             dir_cnt;

    struct newfs_dentry* dentry;
    struct newfs_dentry* dentrys;

    int* data;
};

struct newfs_dentry {
    char                 name[MAX_NAME_LEN];
    uint32_t             dno;
    uint32_t             ino;
    /* TODO: Define yourself */
    struct newfs_dentry* parent;
    struct newfs_dentry* brother;
    struct newfs_inode*  inode;
    NEWFS_FILE_TYPE      ftype;
};

static inline struct newfs_dentry* new_dentry(char * name, NEWFS_FILE_TYPE ftype) {
    struct newfs_dentry * dentry = (struct newfs_dentry *)malloc(sizeof(struct newfs_dentry));
    memset(dentry, 0, sizeof(struct newfs_dentry));
    memcpy(dentry->name, name, MAX_NAME_LEN);
    dentry->ftype    = ftype;
    dentry->dno      = -1;
    dentry->ino      = -1;
    dentry->inode    = NULL;
    dentry->parent   = NULL;
    dentry->brother = NULL;      
    return dentry;                                      
}


struct newfs_super_d {
    uint32_t magic;
    int      sz_io;
    int      sz_blks;
    int      blks_nums;
    int      sz_usage;

    int      sb_offset;
    int      sb_blks;

    int      ino_offset;
    int      ino_blks;

    int      ino_map_offset;
    int      ino_map_blks;

    int      data_map_offset;
    int      data_map_blks;

    int      data_offset;
    int      data_blks;
};


struct newfs_inode_d {
    uint32_t        ino;
    int             size;
    int             dir_cnt;
    NEWFS_FILE_TYPE ftype;
};

struct newfs_dentry_d {
    char                name[MAX_NAME_LEN];
    uint32_t            dno;
    uint32_t            ino;
    NEWFS_FILE_TYPE     ftype;
};

#endif /* _TYPES_H_ */