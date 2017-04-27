//
//  main.c
//  lab3a
//
//  Created by Xiao Yan on 11/14/16.
//  Copyright © 2016 Xiao Yan. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BLOCK_OFFSET 1024
#define BUFFER_SIZE 4096

struct super_block {
    uint32_t s_inodes_count;        /* Inodes count */
    uint32_t s_blocks_count;        /* Blocks count */
    uint32_t s_r_blocks_count;      /* Reserved blocks count */
    uint32_t s_free_blocks_count;   /* Free blocks count */
    uint32_t s_free_inodes_count;   /* Free inodes count */
    uint32_t s_first_data_block;    /* First Data Block */
    uint32_t s_log_block_size;      /* Block size */
    int32_t s_log_frag_size;       /* Fragment size */
    uint32_t s_blocks_per_group;    /* # Blocks per group */
    uint32_t s_frags_per_group;     /* # Fragments per group */
    uint32_t s_inodes_per_group;    /* # Inodes per group */
    uint32_t s_mtime;               /* Mount time */
    uint32_t s_wtime;               /* Write time */
    uint16_t s_mnt_count;           /* Mount count */
    uint16_t s_max_mnt_count;       /* Maximal mount count */
    uint16_t s_magic;               /* Magic signature */
    uint16_t s_state;               /* File system state */
} superblock;

struct group_descriptor
{
    uint32_t bg_block_bitmap;    /* Blocks bitmap block */
    uint32_t bg_inode_bitmap;    /* Inodes bitmap block */
    uint32_t bg_inode_table;     /* Inodes table block */
    uint16_t bg_free_blocks_count; /* Free blocks count */
    uint16_t bg_free_inodes_count; /* Free inodes count */
    uint16_t bg_used_dirs_count; /* Directories count */
    uint16_t bg_pad;
    uint32_t bg_reserved[3];
};

struct Inode {
    uint16_t i_mode;   /* File mode */
    uint16_t i_uid;    /* Low 16 bits of Owner Uid */
    uint32_t i_size;   /* Size in bytes */
    uint32_t i_atime;  /* Access time */
    uint32_t i_ctime;  /* Creation time */
    uint32_t i_mtime;  /* Modification time */
    uint32_t i_dtime;  /* Deletion Time */
    uint16_t i_gid;    /* Low 16 bits of Group Id */
    uint16_t i_links_count;  /* Links count */
    uint32_t i_blocks; /* Blocks count */
    uint32_t i_flags;  /* File flags */
    uint32_t i_osd1;       /* OS dependent 1 */
    uint32_t i_block[15];  /* Pointers to blocks */
    uint32_t i_generation; /* File version (for NFS) */
    uint32_t i_file_acl;   /* File ACL */
    uint32_t i_dir_acl;    /* Directory ACL */
    uint32_t i_faddr;      /* Fragment address */
    uint32_t i_osd2[3];    /* OS dependent 2 */
} inode;

struct directory {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char name [256];
} dir;

struct blocklist{
    uint32_t blocknum;
    struct blocklist * next;
};

int blockgroup;
struct group_descriptor * groupdestable;
uint32_t blocksize;

int write_superblock(int);
int write_groupdes(int);
int write_bitmap_entry(int);
int write_inodes(int);
int write_directentry(int);
int write_indirect(int);

uint32_t getblocknum(int, struct Inode, int);

int fd = 0;

int entrynumber = 0;

int main(int argc, char * argv[]) {
    if(argc < 2){
        printf("Usage: lab3a disk-image\n\n");
        exit(-1);
    }
    else if (argc > 2){
        printf("Usage: lab3a disk-image\n\n");
        exit(-1);
    }
    else{
        printf("%s",argv[1]);
    }
    
    fd = open(argv[1],O_RDONLY);
    if (fd < 0){
        printf("error opening the file\n\n");
        exit(-1);
    }
    
    write_superblock(fd);
    write_groupdes(fd);
    write_bitmap_entry(fd);
    write_inodes(fd);
    write_directentry(fd);
    write_indirect(fd);
    
    
    close(fd);
    return 0;
}

int write_superblock(int fd){
    int num = pread(fd, &superblock, 60, BLOCK_OFFSET);
    if (num < 60){
        fprintf(stderr, "Error reading Superblock\n");
        exit(-1);
    }
    //sanity check
    if(superblock.s_magic!= 0xEF53){
        fprintf(stderr, "Superblock - invalid magic:%" PRIu16 "\n", superblock.s_magic );
        exit(-1);
    }
    
    blocksize = 1024 << superblock.s_log_block_size;
    if((blocksize & (blocksize - 1)) || blocksize <= 512 || blocksize >= 64 * 1024){
        fprintf(stderr, "Superblock - invalid block size:%" PRIu32 "\n", blocksize );
        exit(-1);
    }
    
    //calculate fragment size
    uint32_t fragsize;
    if(superblock.s_log_frag_size > 0){
        fragsize = 1024 << superblock.s_log_frag_size;
    }
    else {
        fragsize = 1024 >> -(superblock.s_log_frag_size);
    }
    
    uint32_t totalblock = superblock.s_blocks_count;
    int imagesize = 200 * 1024 * 1024 / blocksize;
    if(totalblock > imagesize){
        fprintf(stderr, "Superblock - invalid block count:%" PRIu32 "> image size %d\n", totalblock,imagesize);
        exit(-1);
    }
    
    //check blocksize and firstblock
    if (blocksize == 1024 && superblock.s_first_data_block != 1){
        fprintf(stderr, "Superblock - invalid first block: %" PRIu32 "\n", superblock.s_first_data_block);
        exit(-1);
    }
    if (blocksize > 1024 && superblock.s_first_data_block != 0){
        fprintf(stderr, "Superblock - invalid first block: %" PRIu32 "\n", superblock.s_first_data_block);
        exit(-1);
    }
    
    if(totalblock % superblock.s_blocks_per_group != 0){
        fprintf(stderr, "Superblock - %" PRIu32 "blocks, %" PRIu32 "blocks/group", totalblock, superblock.s_blocks_per_group);
        exit(-1);
    }
    if(superblock.s_inodes_count % superblock.s_inodes_per_group != 0){
        fprintf(stderr, "Superblock - %" PRIu32 "Inodes, %" PRIu32 "Inodes/group", superblock.s_inodes_count, superblock.s_inodes_per_group);
        exit(-1);
    }
    
    int rfd = creat("super.csv", 0666);
    if (rfd < 0){
        fprintf(stderr, "unable to write data\n");
        exit(-1);
    }
    
    char outbuf[BUFFER_SIZE] = {0};
    num = sprintf(outbuf, "%x,%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32"\n",
                  superblock.s_magic, superblock.s_inodes_count, superblock.s_blocks_count,
                  blocksize, fragsize, superblock.s_blocks_per_group,
                  superblock.s_inodes_per_group, superblock.s_frags_per_group, superblock.s_first_data_block);
 
    write(rfd,outbuf,num);
    
    close(rfd);
    return 0;
}

int write_groupdes(int fd){
    int first = superblock.s_first_data_block + 1;//find the location of the first block group
    blockgroup = superblock.s_blocks_count/superblock.s_blocks_per_group;
    groupdestable = (struct group_descriptor *) malloc(sizeof(struct group_descriptor) * blockgroup);
    int size = sizeof(struct group_descriptor) * blockgroup;
    int num = pread(fd, groupdestable, size, first * blocksize);
    if (num < size){
        fprintf(stderr,"error reading block\n");
        exit(-1);
    }
    
    int rfd = creat("group.csv", 0666);
    
    if (rfd < 0){
        fprintf(stderr, "unable to write data\n");
        exit(-1);
    }
    
    char outbuf[BUFFER_SIZE] = {0};
    int i;
    int start;
    int finish;
    int num2 = 0;
    for (i = 0; i < blockgroup; i++){
        start = superblock.s_first_data_block + i * superblock.s_blocks_per_group;
        finish = superblock.s_first_data_block -1 + (i + 1) * superblock.s_blocks_per_group;
        if (groupdestable[i].bg_inode_bitmap < start || groupdestable[i].bg_inode_bitmap > finish){
            fprintf(stderr, "Group %d: blocks %d-%d, free Inode map starts at %" PRIu32 "\n",i,start,finish,groupdestable[i].bg_inode_bitmap);
            continue;
        }
        else if (groupdestable[i].bg_block_bitmap < start || groupdestable[i].bg_block_bitmap > finish){
            fprintf(stderr, "Group %d: blocks %d-%d, free block map starts at %" PRIu32 "\n",i,start,finish,groupdestable[i].bg_block_bitmap);
            continue;
        }
        else if (groupdestable[i].bg_inode_table < start || groupdestable[i].bg_inode_table > finish){
            fprintf(stderr, "Group %d: blocks %d-%d, Inode table starts at %" PRIu32 "\n",i,start,finish,groupdestable[i].bg_inode_table);
            continue;
        }
        
        else {
            num2 = sprintf(outbuf, "%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32",%x,%x,%x\n",
                      superblock.s_blocks_per_group, groupdestable[i].bg_free_blocks_count, groupdestable[i].bg_free_inodes_count,
                      groupdestable[i].bg_used_dirs_count, groupdestable[i].bg_inode_bitmap, groupdestable[i].bg_block_bitmap,
                      groupdestable[i].bg_inode_table);
            write(rfd,outbuf,num2);
        }
    }
    

    close(rfd);
    return 0;
    
}

int write_bitmap_entry(int fd){
    int rfd = creat("bitmap.csv", 0666);
    if (rfd < 0){
        fprintf(stderr, "unable to write data\n");
        exit(-1);
    }
    
    char outbuf[BUFFER_SIZE] = {0};
    int i, j, k; //i for blockgroup, j for each byte, k for each bit,
    int num;
    
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint8_t * block_bitmap_buf = (uint8_t *)malloc(sizeof(uint8_t) * blocksize);
    uint8_t * inode_bitmap_buf = (uint8_t *)malloc(sizeof(uint8_t) * blocksize);
    uint8_t bitnum;
    
    int blockindex = 0;
    int inodeindex = 0;
    
    for (i = 0 ; i < blockgroup; i++){
        block_bitmap = groupdestable[i].bg_block_bitmap;
        inode_bitmap = groupdestable[i].bg_inode_bitmap;
        pread(fd, block_bitmap_buf, blocksize, block_bitmap * blocksize);
        pread(fd, inode_bitmap_buf, blocksize, inode_bitmap * blocksize);
        
        
        for (j = 0; j < blocksize; j++){
            
            for (k = 0; k < 8; k++){
                if (blockindex <= (i + 1) * superblock.s_blocks_per_group){
                    blockindex++;
                    bitnum = ((block_bitmap_buf[j] >> k) & 1);
                    if (bitnum == 0){
                        num = sprintf(outbuf,"%x,%d\n",groupdestable[i].bg_block_bitmap,blockindex);
                        write(rfd,outbuf,num);
                    }
                }
                
                
            }
        }
        
        for (j = 0; j < blocksize; j++){
            for (k = 0; k < 8; k++){
                if (inodeindex < (i + 1)*superblock.s_inodes_per_group){
                    inodeindex++;
                    bitnum = ((inode_bitmap_buf[j] >> k) & 1);
                    if (bitnum == 0){
                        num = sprintf(outbuf,"%x,%d\n",groupdestable[i].bg_inode_bitmap, inodeindex);
                        write(rfd,outbuf,num);
                    }
                    
                }
                
            }
        }
        
        
    }
    
    free(block_bitmap_buf);
    free(inode_bitmap_buf);
    close(rfd);
    return 0;
}

int write_inodes(int fd){
    int rfd = creat("inode.csv", 0666);
    if (rfd < 0){
        fprintf(stderr, "unable to write data\n");
        exit(-1);
    }
    
    char outbuf[BUFFER_SIZE] = {0};
    int i, j, k, l; //i for blockgroup, j for each byte, k for each bit, l for block pointers
    int num;
    
    uint32_t inode_bitmap;
    uint8_t * inode_bitmap_buf = (uint8_t *)malloc(sizeof(uint8_t) * blocksize);
    uint8_t bitnum;
    
    char filetype;
    int inodeindex = 0;
    int mode;
    
    for (i = 0 ; i < blockgroup; i++){
        inode_bitmap = groupdestable[i].bg_inode_bitmap;
        pread(fd, inode_bitmap_buf, blocksize, inode_bitmap * blocksize);
        
        for (j = 0; j < blocksize; j++){
            for (k = 0; k < 8; k++){
                if (inodeindex < (i + 1) * superblock.s_inodes_per_group){
                    inodeindex++;
                    bitnum = ((inode_bitmap_buf[j] >> k) & 1);
                    if (bitnum == 1){
                        pread(fd, &inode, sizeof(struct Inode), groupdestable[i].bg_inode_table * blocksize + sizeof(struct Inode) * (j * 8 + k));
                        mode = inode.i_mode >> 12;
                        if (mode == 8) {
                            filetype = 'f';
                        }
                        else if (mode == 12) {
                            filetype = 's';
                        }
                        else if (mode == 4) {
                            filetype = 'd';
                        }
                        else {
                            filetype = '?';
                        }
                        
                        uint32_t i_blocks = inode.i_blocks / (2 << superblock.s_log_block_size);
                        num = sprintf(outbuf, "%u,%c,%o,%u,%u,%u,%x,%x,%x,%u,%u",
                                      inodeindex, filetype, inode.i_mode, inode.i_uid, inode.i_gid,
                                      inode.i_links_count, inode.i_ctime, inode.i_mtime, inode.i_atime,
                                      inode.i_size, i_blocks);
                        write(rfd,outbuf,num);
                        
                        for (l = 0 ; l < 15; l++){
                            if (inode.i_block[l] < superblock.s_first_data_block || inode.i_block[l] > i_blocks){
                                fprintf(stderr, "Inode %d - invalid block pointer[%d]: %x\n",inodeindex,l,inode.i_block[l]);
                            }
                            num = sprintf(outbuf, ",%x",inode.i_block[l]);
                            write(rfd, outbuf,num);
                        }
                        write(rfd,"\n",1);
                    }
                }
            }
        }
    }

    free(inode_bitmap_buf);
    close(rfd);
    return 0;
}

int write_directentry(int fd){
    int rfd = creat("directory.csv", 0666);
    if (rfd < 0){
        fprintf(stderr, "unable to write data\n");
        exit(-1);
    }
    
    char outbuf[BUFFER_SIZE] = {0};
    int i, j, k; //i for blockgroup, j for each byte, k for each bit
    int num;
    
    uint32_t inode_bitmap;
    uint8_t * inode_bitmap_buf = (uint8_t *)malloc(sizeof(uint8_t) * blocksize);
    uint8_t bitnum;
    
    int inodeindex = 0;
    int mode;
    
    for (i = 0 ; i < blockgroup; i++){
        inode_bitmap = groupdestable[i].bg_inode_bitmap;
        pread(fd, inode_bitmap_buf, blocksize, inode_bitmap * blocksize);
        
        for (j = 0; j < blocksize; j++){
            for (k = 0; k < 8; k++){
                if (inodeindex < (i + 1) * superblock.s_inodes_per_group){
                    inodeindex++;
                    bitnum = ((inode_bitmap_buf[j] >> k) & 1);
                    if (bitnum == 1){
                        pread(fd, &inode, sizeof(struct Inode), groupdestable[i].bg_inode_table * blocksize + sizeof(struct Inode) * (j * 8 + k));
                        mode = inode.i_mode >> 12;
                        if (mode == 4) {//it is a directory
                            uint32_t i_blocks = inode.i_blocks / (2 << superblock.s_log_block_size);
                            int blockindex = 0;
                            int blocknum;
                            int position;
                            entrynumber = 0;
                            while (blockindex < i_blocks){
                                
                                blocknum = getblocknum(fd, inode, blockindex);
                                if (blocknum == 0){
                                    break;
                                }
                                position = 0;
                                
                                
                                while (position < blocksize){
                                    memset(dir.name,0,256);
                                    pread(fd, &dir, 8, blocknum * blocksize + position);
                                    pread(fd, dir.name, dir.name_len, blocknum * blocksize + position + 8);
                                    
                                    if (dir.rec_len > 1024){
                                        fprintf(stderr, "Inode %d, block - bad dirent: Entrylen = %d\n",inodeindex,dir.rec_len);
                                        break;
                                    }
                                    else if (dir.name_len > dir.rec_len){
                                        fprintf(stderr,"Inode %d, block - bad dirent: len = %d, namelen = %d\n", inodeindex, dir.rec_len,dir.name_len);
                                        break;
                                    }
                                    else if(dir.inode > superblock.s_inodes_count){
                                        fprintf(stderr,"Inode %d, block - bad dirent: Inode = %d\n",inodeindex,dir.inode);
                                        break;
                                    }
                                    if (dir.inode != 0){
                                        num = sprintf(outbuf, "%d,%d,%d,%d,%d,\"%s\"\n",inodeindex,entrynumber,dir.rec_len,dir.name_len,dir.inode,dir.name);
                                        write(rfd,outbuf,num);
                                    }
                                   
                                    entrynumber++;
                                    position += dir.rec_len;
                                }
                                
                                blockindex++;
                                
                            }
                        }
                    }
                }
            }
        }
    }

    free(inode_bitmap_buf);
    close(rfd);
    return 0;

}

int write_indirect(int fd){
    int rfd = creat("indirect.csv", 0666);
    if (rfd < 0){
        fprintf(stderr, "unable to write data\n");
        exit(-1);
    }
    
    char outbuf[BUFFER_SIZE] = {0};
    int i, j, k; //i for blockgroup, j for each byte, k for each bit
    int num;
    
    uint32_t inode_bitmap;
    uint8_t * inode_bitmap_buf = (uint8_t *)malloc(sizeof(uint8_t) * blocksize);
    uint8_t bitnum;
    
    int inodeindex = 0;
    
    for (i = 0 ; i < blockgroup; i++){
        inode_bitmap = groupdestable[i].bg_inode_bitmap;
        pread(fd, inode_bitmap_buf, blocksize, inode_bitmap * blocksize);
        
        for (j = 0; j < blocksize; j++){
            for (k = 0; k < 8; k++){
                if (inodeindex < (i + 1) * superblock.s_inodes_per_group){
                    inodeindex++;
                    bitnum = ((inode_bitmap_buf[j] >> k) & 1);
                    if (bitnum == 1){
                        pread(fd, &inode, sizeof(struct Inode), groupdestable[i].bg_inode_table * blocksize + sizeof(struct Inode) * (j * 8 + k));
                        uint32_t i_blocks = inode.i_blocks / (2 << superblock.s_log_block_size);
                        if (inode.i_block[12] != 0){
                            int i;
                            int blocknum = inode.i_block[12];
                            int indirect;
                            int position = 0;
                            for (i = 0; i < 256; i++){
                                pread(fd, &indirect, 4, blocknum * blocksize + position * 4);
                                if (indirect == 0){
                                    break;
                                }
                                if (indirect > superblock.s_blocks_count){
                                    fprintf(stderr, "Indirect block %x - invalid entry[%d] = %x\n", blocknum,position,indirect);
                                    break;
                                }
                                num = sprintf(outbuf, "%x,%d,%x\n",blocknum, position, indirect);
                                write(rfd,outbuf,num);
                                position++;
                            }
                        }
                        if (inode.i_block[13] != 0){
                            int i, j;
                            uint32_t doublenum;
                            int indirect;
                            int doubleindirect;
    
                            
                            doublenum = inode.i_block[13];
                            
                            for (i = 0; i < 256; i++){
                                pread(fd, &doubleindirect, 4, doublenum + i * 4);
                                if (doubleindirect == 0){
                                    break;
                                }
                                if (doubleindirect > superblock.s_blocks_count){
                                    fprintf(stderr, "Indirect block %x - invalid entry[%d] = %x\n", doublenum, i, doubleindirect);
                                    break;
                                }
                                num = sprintf(outbuf, "%x,%d,%x\n",doublenum, i, doubleindirect);
                                write(rfd,outbuf,num);
                                
                                for (j = 0; j < 256; j++){
                                    pread(fd, &indirect, 4, doubleindirect + j * 4);
                                    if (indirect == 0){
                                        break;
                                    }
                                    if (indirect > superblock.s_blocks_count){
                                        fprintf(stderr, "Indirect block %x - invalid entry[%d] = %x\n", doubleindirect, j, indirect);
                                        break;
                                    }
                                    num = sprintf(outbuf, "%x,%d,%x\n",doubleindirect, j, indirect);
                                    write(rfd,outbuf,num);
                                }
                            }
                        }
                        if (inode.i_block[14] != 0){
                            int i, j, k;
                            uint32_t triplynum;
                            int indirect;
                            int doubleindirect;
                            int triplyindirect;
                            
                            triplynum = inode.i_block[14];
                            
                            for (i = 0; i < 256; i++){
                                pread(fd, &triplyindirect, 4, triplynum + i * 4);
                                if (triplyindirect == 0){
                                    break;
                                }
                                if (triplyindirect > superblock.s_blocks_count){
                                    fprintf(stderr, "Indirect block %x - invalid entry[%d] = %x\n", triplynum, i, triplyindirect);
                                    break;
                                }
                                
                                num = sprintf(outbuf, "%x,%d,%x\n",triplynum, i, triplyindirect);
                                write(rfd,outbuf,num);
                                
                                for (j = 0; j < 256; j++){
                                    pread(fd, &doubleindirect, 4, triplyindirect + j * 4);
                                    if (doubleindirect == 0){
                                        break;
                                    }
                                    if (doubleindirect > superblock.s_blocks_count){
                                        fprintf(stderr, "Indirect block %x - invalid entry[%d] = %x\n", triplyindirect, j, doubleindirect);
                                        break;
                                    }
                                    num = sprintf(outbuf, "%x,%d,%x\n",triplyindirect, j, doubleindirect);
                                    write(rfd,outbuf,num);
                                    
                                    for (k = 0; k < 256; k++){
                                        pread(fd, &indirect, 4, doubleindirect + 4 * k);
                                        if (indirect == 0){
                                            break;
                                        }
                                        if (indirect > superblock.s_blocks_count){
                                            fprintf(stderr, "Indirect block %x - invalid entry[%d] = %x\n", doubleindirect, k, indirect);
                                            break;
                                        }
                                        num = sprintf(outbuf, "%x,%d,%x\n",doubleindirect, k, indirect);
                                        write(rfd,outbuf,num);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    free(inode_bitmap_buf);
    close(rfd);
    return 0;
    
}

uint32_t getblocknum(int fd, struct Inode inode, int blockindex){
    uint32_t blocknum = 0;
    uint32_t indirectnum = 0;
    uint32_t doublenum = 0;
    int indirectoff = 0;
    int doubleoff = 0;
    int triplyoff = 0;
    if (blockindex < 12){
        blocknum = inode.i_block[blockindex];
    }
    else if (blockindex < 268){
        //block 13 to 268 stores in the i_block[12] indrect
        indirectoff = (blockindex - 12) * sizeof(uint32_t);
        pread(fd, &blocknum, sizeof(uint32_t), inode.i_block[12] * blocksize + indirectoff);
    }
    else if (blockindex < 65804){
        //block stores in the i_block[13] double indirect
        
        doubleoff = (blockindex -268)/256 * sizeof(uint32_t);
        pread(fd, &indirectnum, sizeof(uint32_t), inode.i_block[13] * blocksize + doubleoff);
        
        indirectoff = (blockindex-268)%256 * sizeof(uint32_t);
        pread(fd, &blocknum, sizeof(uint32_t), indirectnum * blocksize + indirectoff);
    }
    else if (blockindex < 16843020){
        
        triplyoff = (blockindex-65804)/65536 * sizeof(uint32_t);
        pread(fd, &doublenum, sizeof(uint32_t), inode.i_block[14] * blocksize + triplyoff);
        
        doubleoff = ((blockindex-65804)%65536)/256 * sizeof(uint32_t);
        pread(fd, &indirectnum, sizeof(uint32_t), doublenum * blocksize + doubleoff);
        
        indirectoff = ((blockindex-65804)%65536)%256 * sizeof(uint32_t);
        pread(fd, &blocknum, sizeof(uint32_t), indirectnum * blocksize + indirectoff);
    }
    
    else{
        exit(-1);
    }
    return blocknum;
}




































