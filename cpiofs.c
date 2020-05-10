#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <byteswap.h>

#include "cpiofs.h"

// #define CPIO_DEBUG

#ifdef CPIO_DEBUG
#include <stdio.h>
#define DEBUG(fmt, args...) \
    printf("[%d]: " fmt "\n", __LINE__, ##args)
#else
#define DEBUG(fmt, ...)
#endif

struct header_old_cpio {
        uint16_t c_magic;
        uint16_t c_dev;
        uint16_t c_ino;
        uint16_t c_mode;
        uint16_t c_uid;
        uint16_t c_gid;
        uint16_t c_nlink;
        uint16_t c_rdev;
        uint16_t c_mtime[2];
        uint16_t c_namesize;
        uint16_t c_filesize[2];
} __attribute__((packed));


#define C_MAGIC 070707

#define IMP_GETTER16(x) \
    uint16_t cpio_get_ ## x (const struct header_old_cpio* d) { \
        if (d->c_magic == C_MAGIC) {                            \
            return d->c_ ## x;                                  \
        }                                                       \
        return bswap_16(d->c_ ## x);                            \
    }

#define IMP_GETTER32(x) \
    uint32_t cpio_get_ ## x (const struct header_old_cpio* d) { \
        uint16_t l;                                             \
        uint16_t h;                                             \
        if (d->c_magic == C_MAGIC) {                            \
            l = d->c_ ## x[1] ;                                 \
            h = d->c_ ## x[0] ;                                 \
        } else {                                                \
            l = bswap_16(d->c_ ## x[1]) ;                       \
            h = bswap_16(d->c_ ## x[0]) ;                       \
        }                                                       \
        return ((uint32_t)h << 16) + l;                         \
    }

IMP_GETTER16(magic)
IMP_GETTER16(dev)
IMP_GETTER16(ino)
IMP_GETTER16(mode)
IMP_GETTER16(uid)
IMP_GETTER16(gid)
IMP_GETTER16(nlink)
IMP_GETTER16(rdev)
IMP_GETTER32(mtime)
IMP_GETTER16(namesize)
IMP_GETTER32(filesize)

#undef IMP_GETTER16
#undef IMP_GETTER32

const char* get_filename(const struct header_old_cpio* data, uint16_t *len) {
    const char * ret = (const char*)data + sizeof(struct header_old_cpio);
    if (NULL != len) {
        *len = cpio_get_namesize(data);
    }
    return ret;
}

int cpio_valid(const struct header_old_cpio* d, unsigned long dsize) {
    if ((d != NULL) && (dsize > sizeof(struct header_old_cpio))) {
        dsize -= sizeof(struct header_old_cpio);
        uint16_t namelen = cpio_get_namesize(d);
        if (namelen % 2U == 1U) {
            namelen ++;
        }
        if (dsize >= namelen) {
            dsize -= namelen;

            uint32_t datalen = cpio_get_filesize(d);
            if (datalen % 2 == 1) {
                datalen ++;
            }
            if (dsize >= datalen) {
                return 1;
            }
        }
    }
    return 0;
}

const struct header_old_cpio* cpio_init_iter(const void *d, long dsize) {
    if (cpio_valid((const struct header_old_cpio*) d, dsize)) {
        uint16_t filename_size;
        const char * filename;
        filename = get_filename(d, &filename_size);
        if ((filename_size != 11) || (strncmp(filename, "TRAILER!!!", filename_size) != 0)) {
            return (const struct header_old_cpio*)d;
        }
    }
    return NULL;
}

const struct header_old_cpio* cpio_goto_next(const struct header_old_cpio* d, unsigned long *pdsize) {
    unsigned long dsize = *pdsize;
    const uint8_t *dnext = (uint8_t *)d;
    if (dsize > sizeof(struct header_old_cpio)) {
        dsize -= sizeof(struct header_old_cpio);
        dnext += sizeof(struct header_old_cpio);
        uint16_t namelen = cpio_get_namesize(d);
        if (namelen % 2U == 1U) {
            namelen ++;
        }
        if (dsize >= namelen) {
            dsize -= namelen;
            dnext += namelen;

            uint32_t datalen = cpio_get_filesize(d);
            if (datalen % 2U == 1U) {
                datalen ++;
            }
            if (dsize >= datalen) {
                dsize -= datalen;
                dnext += datalen;
                if (dsize > 0) {
                    const struct header_old_cpio* retval = cpio_init_iter(dnext, dsize);
                    if (NULL != retval) {
                        *pdsize = dsize;
                        return retval;
                    }
                }
            }
        }
    }
    *pdsize = 0;
    return NULL;
}

const uint8_t* get_filedata(const struct header_old_cpio* d, uint32_t *len) {
    const uint8_t * ret = (const uint8_t*)d + sizeof(struct header_old_cpio);
    uint16_t nlen = cpio_get_namesize(d);
    if (nlen % 2 == 1) {
        nlen ++;
    }
    ret += nlen;
    if (NULL != len) {
        *len = cpio_get_filesize(d);
    }
    return ret;
}

/* ** */
typedef int (*strncmp_t) (const char *s1, const char *s2, size_t n);

static const struct header_old_cpio* cpiofs_find(const cpiofs_t *fs, const char *path, uint16_t mask, strncmp_t cmp, unsigned long *pdsize) {
    if ((path[0] == '.') && (path[1] == '/')) {
        path += 2;
    } else if (path[0] == '/') {
        path += 1;
    }
    unsigned long fsize = fs->size;
    for (const struct header_old_cpio* pdata = cpio_init_iter(fs->head, fsize); 
         pdata != NULL; 
         pdata = cpio_goto_next(pdata, &fsize)) {
        uint16_t mode = cpio_get_mode(pdata);
        if ((mode & mask) != 0) {
            uint16_t filename_size;
            const char* filename;
            filename = get_filename(pdata, &filename_size);
            if (filename != NULL) {
                if ((filename_size >= 2) && (filename[0] == '.') && (filename[1] == '/')) {
                    filename += 2;
                    filename_size -= 2;
                } else if ((filename_size >= 2) && (filename[0] == '/')) {
                    filename += 1;
                    filename_size -= 1;
                }
                if ((filename_size > 0) || (path[0] == '\0')) {
                    if (((filename_size == 0) && (path[0] == '\0')) || 
                        (cmp(filename, path, filename_size) == 0)) {
                        if (pdsize != NULL) {
                            *pdsize = fsize;
                        }
                        return pdata;
                    }
                }
            }
        }
    }
    if (pdsize != NULL) {
        *pdsize = 0;
    }
    return NULL;
}


int cpiofs_stat(const cpiofs_t *fs, const char *path, cpio_info_t *info) {
    const struct header_old_cpio* pdata = cpiofs_find(fs, path, CPIO_FILEDIR_TYPE, strncmp, NULL);
    if (pdata != NULL) {
        if (NULL != info) {
            uint16_t mode = cpio_get_mode(pdata);
            info->type = mode & CPIO_TYPE_MASK;
            info->mode = mode & (CPIO_MODE_MASK);
            info->size = cpio_get_filesize(pdata);
            info->filename = NULL;
            info->filenames = 0;
            info->filepath = NULL;
            info->filepaths = 0;
        }
        return (int)CPIO_ERR_OK;
    }
    return (int)CPIO_ERR_NEXIST;
}

int cpiofs_file_open(cpiofs_t *fs, cpio_file_t *file, const char *path) {
    const struct header_old_cpio* pdata = cpiofs_find(fs, path, CPIO_FILE_TYPE_MASK, strncmp, NULL);
    if (pdata != NULL) {
        file->fs = fs;
        file->head = pdata;
        file->pos = 0;
        fs->resource_count ++;
        return (int)CPIO_ERR_OK;
    }
    return (int)CPIO_ERR_NEXIST;
}

int cpiofs_file_close(cpio_file_t *file) {
    if (file->fs != NULL) {
        // todo add assert if file->fs->resource_count == 0
        file->fs->resource_count --;
        file->fs = NULL;
        file->head = NULL;
        file->pos = 0;
        return (int)CPIO_ERR_OK;
    }
    return (int)CPIO_ERR_NEXIST;
}

cpio_ssize_t cpiofs_file_read(cpio_file_t *file, void *buffer, cpio_size_t size) {
    if (file->head != NULL) {
        if (size > 0) {
            uint32_t fsize;
            const uint8_t* data = get_filedata(file->head, &fsize);
            if (file->pos > fsize) {
                return CPIO_ERR_UNKNOWN;
            }
            cpio_size_t data_read = fsize - file->pos;
            if (size < data_read) {
                data_read = size;
            }
            memcpy(buffer, &data[file->pos], data_read);
            file->pos += data_read;
            return data_read;
        }
    }
    return (cpio_ssize_t)CPIO_ERR_NEXIST;
}

cpio_soff_t cpiofs_file_seek(cpio_file_t *file, cpio_soff_t off, cpio_whence_flags_t whence) {
    if (file->head != NULL) {
        uint32_t fsize = cpio_get_filesize(file->head);
        switch (whence) {
            case CPIO_SEEK_END:
                file->pos = (cpio_off_t)fsize;
                __attribute__ ((fallthrough));
            case CPIO_SEEK_CUR:
                off += (cpio_soff_t)file->pos;
                __attribute__ ((fallthrough));
            case CPIO_SEEK_SET:
                if ((off >= 0) && ((cpio_off_t)off <= fsize)) {
                    file->pos = (cpio_off_t)off;
                    return file->pos;
                } else {
                    return CPIO_ERR_SEEK_OUT;
                }
                break;
            default:
                return CPIO_ERR_PARAM;
        }
    }
    return (cpio_ssize_t)CPIO_ERR_NEXIST;
}

int cpiofs_dir_open(cpiofs_t *fs, cpio_dir_t *dir, const char *path) {
    const struct header_old_cpio* pdata = cpiofs_find(fs, path, CPIO_DIR_TYPE_MASK, strncmp, NULL);
    if (pdata != NULL) {
        dir->fs = fs;
        dir->head = pdata;
        dir->pos = fs->head;
        dir->size = fs->size;
        fs->resource_count ++;
        return (int)CPIO_ERR_OK;
    }
    return (int)CPIO_ERR_NEXIST;
}

int cpiofs_dir_close(cpio_dir_t *dir) {
    if (dir->fs != NULL) {
        // todo add assert if file->fs->resource_count == 0
        dir->fs->resource_count --;
        dir->fs = NULL;
        dir->head = NULL;
        dir->pos = NULL;
        return (int)CPIO_ERR_OK;
    }
    return (int)CPIO_ERR_NEXIST;
}


static int nprefix_nosub(const char *s1, const char *s2, size_t n) {
    size_t i = 0;
    if (s2[0] != '\0') {
        for (i = 0; (i < n) && s2[i] != '\0'; i++) {
            if (s1[i] > s2[i]) {
                return 1;
            } else if (s1[i] < s2[i]) {
                return -1;
            }
        }
        i++;
    }
    for(; (i < n) &&  (s1[i] != '\0'); i++) {
        if (s1[i] == '/') {
            return 1;
        }
    }

    return 0;
}

int cpiofs_dir_read(cpio_dir_t *dir, cpio_info_t *info) {
    if (dir->fs != NULL) {
        if (NULL == dir->pos) {
            return 0;
        }
        cpiofs_t cpiofs = {
            .head = dir->pos,
            .size = dir->size,
            .resource_count = 0,
        };
        unsigned long dsize = 0;
        const struct header_old_cpio* pdata = cpiofs_find(&cpiofs, get_filename(dir->head, NULL), CPIO_FILEDIR_TYPE, nprefix_nosub, &dsize);
        if (pdata == dir->head) {
            pdata = cpio_goto_next(pdata, &dsize);
            if (pdata != NULL) {
                cpiofs.head = pdata;
                cpiofs.size = dsize;
                pdata = cpiofs_find(&cpiofs, get_filename(dir->head, NULL), CPIO_FILEDIR_TYPE, nprefix_nosub, &dsize);
            }
        }
        if (pdata != NULL) {
            if (info != NULL) {
                uint16_t mode = cpio_get_mode(pdata);
                info->type = mode & CPIO_TYPE_MASK;
                info->mode = mode & (CPIO_MODE_MASK);
                info->size = cpio_get_filesize(pdata);
                info->filepath = get_filename(pdata, &info->filepaths);

                info->filename = &info->filepath[info->filepaths - 1];
                info->filenames = 0;
                while ((info->filename != info->filepath) && (info->filename[0] != '/')) {
                    info->filename --;
                    info->filenames ++;
                }
                if (info->filename[0] == '/') {
                    info->filename ++;
                    info->filenames --;
                }
            }
            pdata = cpio_goto_next(pdata, &dsize);
            if (pdata != NULL) {
                dir->pos = pdata;
                dir->size = dsize;
            }
            return 1;
        }
        return 0;
    }
    return (int)CPIO_ERR_NEXIST;
}
