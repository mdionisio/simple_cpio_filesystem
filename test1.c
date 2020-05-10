#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "cpiofs.h"

off_t fsize(const char *filename) {
    struct stat st; 

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1; 
}

void print_file(const struct header_old_cpio* d, long dsize) {
    if (cpio_valid(d, dsize)) 
    {
        uint16_t magic = cpio_get_magic(d);
        uint16_t dev = cpio_get_dev(d);
        uint16_t ino = cpio_get_ino(d);
        uint16_t mode = cpio_get_mode(d);
        uint16_t uid = cpio_get_uid(d);
        uint16_t gid = cpio_get_gid(d);
        uint16_t nlink = cpio_get_nlink(d);
        uint16_t rdev = cpio_get_rdev(d);
        uint32_t mtime = cpio_get_mtime(d);
        uint16_t namesize = cpio_get_namesize(d);
        uint32_t filesize = cpio_get_filesize(d);

        printf("magic 0%" PRIo16 "\n", magic);
        printf("dev %" PRIu16 "\n", dev);
        printf("ino %" PRIu16 "\n", ino);
        printf("mode %" PRIu16 "\n", mode);
        printf("uid %" PRIu16 "\n", uid);
        printf("gid %" PRIu16 "\n", gid);
        printf("nlink %" PRIu16 "\n", nlink);
        printf("rdev %" PRIu16 "\n", rdev);
        printf("mtime %" PRIu32 "\n", mtime);
        printf("namesize %" PRIu16 "\n", namesize);
        printf("filesize %" PRIu32 "\n", filesize);

        uint16_t filename_size;
        const char* filename;
        filename = get_filename(d, &filename_size);
        printf("filename %.*s\n", (int)filename_size, filename);

        uint32_t data_size;
        const uint8_t *data;
        data = get_filedata(d, &data_size);
        printf("data:");
        for(uint32_t i=0; i<data_size; i++) {
            if (i % 16 == 0) {
                printf("\n");
            }
            if ((i > 0) && (i % 8 == 0)) {
                printf("    ");
            }
            printf("%02" PRIx8 " ", data[i]);
        }
        printf("\n-------\n");
    }
}

static int test_cpiofs_stat(const cpiofs_t *cpiofs) {
    cpio_info_t info;
    if (cpiofs_stat(cpiofs, "file_empty.txt", &info) != CPIO_ERR_OK) {
        fprintf(stderr, "file_empty.txt has to exist\n");
        return -1;
    }
    if (info.size != 0) {
        fprintf(stderr, "file_empty.txt has to have size 0\n");
        return -1;
    }
    if ((info.type & CPIO_FILE_TYPE_MASK) == 0) {
        fprintf(stderr, "file_empty.txt has to be a regular file 0x%08o\n", (unsigned int)info.type);
        return -1;
    }

    if (cpiofs_stat(cpiofs, "./file_empty.txt", &info) != CPIO_ERR_OK) {
        fprintf(stderr, "file_empty.txt has to exist\n");
        return -1;
    }
    if (info.size != 0) {
        fprintf(stderr, "./file_empty.txt has to have size 0\n");
        return -1;
    }
    if ((info.type & CPIO_FILE_TYPE_MASK) == 0) {
        fprintf(stderr, "./file_empty.txt has to be a regular file 0x%08o\n", (unsigned int)info.type);
        return -1;
    }

    if (cpiofs_stat(cpiofs, "./", &info) != CPIO_ERR_OK) {
        fprintf(stderr, "./ has to exist\n");
        return -1;
    }
    if (info.size != 0) {
        fprintf(stderr, "./ has to have size 0\n");
        return -1;
    }
    if ((info.type & CPIO_DIR_TYPE_MASK) == 0) {
        fprintf(stderr, "./ has to be a directory file 0x%08o\n", (unsigned int)info.type);
        return -1;
    }

    if (cpiofs_stat(cpiofs, "dir1", &info) != CPIO_ERR_OK) {
        fprintf(stderr, "file_empty.txt has to exist\n");
        return -1;
    }
    if (info.size != 0) {
        fprintf(stderr, "dir1 has to have size 0\n");
        return -1;
    }
    if ((info.type & CPIO_DIR_TYPE_MASK) == 0) {
        fprintf(stderr, "dir1 has to be a directory file 0x%08o\n", (unsigned int)info.type);
        return -1;
    }

    if (cpiofs_stat(cpiofs, "./dir1", &info) != CPIO_ERR_OK) {
        fprintf(stderr, "file_empty.txt has to exist\n");
        return -1;
    }
    if (info.size != 0) {
        fprintf(stderr, "./dir1 has to have size 0\n");
        return -1;
    }
    if ((info.type & CPIO_DIR_TYPE_MASK) == 0) {
        fprintf(stderr, "./dir1 has to be a directory file 0x%08o\n", (unsigned int)info.type);
        return -1;
    }

    if (cpiofs_stat(cpiofs, "./dir1/file2.txt", &info) != CPIO_ERR_OK) {
        fprintf(stderr, "./dir1/file2.txt has to exist\n");
        return -1;
    }
    if (info.size != 10) {
        fprintf(stderr, "./dir1/file2.txt has to have size 10\n");
        return -1;
    }
    if ((info.type & CPIO_FILE_TYPE_MASK) == 0) {
        fprintf(stderr, "./dir1/file2.txt has to be a regular file 0x%08o\n", (unsigned int)info.type);
        return -1;
    }

    return 0;
}

int test_cpiofs_dir(cpiofs_t *cpiofs) {
    cpio_dir_t dir;
    int count = 0;
    cpio_info_t listdir_fs_entry;

    if (CPIO_ERR_OK != cpiofs_dir_open(cpiofs, &dir, "./")) {
        fprintf(stderr, "./ error opendir\n");
        return -1;
    }

    count = 0;
    while (cpiofs_dir_read(&dir, &listdir_fs_entry) > 0) {
        // printf("%.*s [%.*s]\n", (int)listdir_fs_entry.filepaths, listdir_fs_entry.filepath, (int)listdir_fs_entry.filenames, listdir_fs_entry.filename);
        count ++;
    }
    cpiofs_dir_close(&dir);

    if (count != 4) {
        fprintf(stderr, "./ has to contain 4 subfiles\n");
        return -1;
    }

    if (CPIO_ERR_OK != cpiofs_dir_open(cpiofs, &dir, "./dir1")) {
        fprintf(stderr, "./ error opendir\n");
        return -1;
    }

    count = 0;
    while (cpiofs_dir_read(&dir, &listdir_fs_entry) > 0) {
        // printf("%.*s [%.*s]\n", (int)listdir_fs_entry.filepaths, listdir_fs_entry.filepath, (int)listdir_fs_entry.filenames, listdir_fs_entry.filename);
        count ++;
    }
    cpiofs_dir_close(&dir);

    if (count != 1) {
        fprintf(stderr, "./dir1 has to contain 1 subfiles\n");
        return -1;
    }

    return 0;
}

int main(int argc, char** argv) {
    int result = 0;
    FILE *fp = NULL;
    uint8_t *data = NULL;

    if (argc < 2) {
        result = -1;
        fprintf(stderr, "required 1 parameter\n");
        goto end;
    }
    
    fp = fopen(argv[1], "r");
    if (NULL == fp) {
        result = -2;
        fprintf(stderr, "impossible to read file: %s\n", argv[1]);
        goto end;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);
    if (fsize <= 0) {
        result = -3;
        fprintf(stderr, "impossible to have good filesize for: %s\n", argv[1]);
        goto end;
    }

    data = malloc(fsize);
    if (NULL == data) {
        result = -4;
        fprintf(stderr, "impossible to have take memory for: %s\n", argv[1]);
        goto end;
    }

    if (fread(data, 1, fsize, fp) != (size_t)fsize) {
        result = -4;
        fprintf(stderr, "impossible to read all data from: %s\n", argv[1]);
        goto end;
    }

    cpiofs_t cpiofs = {
        .head = (const struct header_old_cpio *)data,
        .size = fsize,
        .resource_count = 0,
    };

#if 1
    {
        unsigned long ufsize = (unsigned long)fsize;
        for (const struct header_old_cpio* pdata = cpio_init_iter(data, ufsize); 
            pdata != NULL; 
            pdata = cpio_goto_next(pdata, &ufsize)) {
            print_file(pdata, ufsize);
            printf("\n+++++++++++++++++++\n");
        }
    }
#endif

    if (test_cpiofs_stat(&cpiofs) == -1) {
        result = -5;
        fprintf(stderr, "failed test_cpiofs: %s\n", argv[1]);
        goto end;
    }

    if (test_cpiofs_dir(&cpiofs) == -1) {
        result = -5;
        fprintf(stderr, "failed test_cpiofs_dir: %s\n", argv[1]);
        goto end;
    }


end:
    if (NULL != data) {
        free(data);
        data = NULL;
    }
    if (fp != NULL) {
        fclose(fp);
        fp = NULL;
    }
    return result;
}