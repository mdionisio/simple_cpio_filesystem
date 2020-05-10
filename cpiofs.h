#ifndef _CPIO_H_
#define _CPIO_H_

#include <stdint.h>
#include <stddef.h>

struct header_old_cpio;

#define DEF_GETTER16(x) \
    uint16_t cpio_get_ ## x (const struct header_old_cpio* d)

#define DEF_GETTER32(x) \
    uint32_t cpio_get_ ## x (const struct header_old_cpio* d)

DEF_GETTER16(magic);
DEF_GETTER16(dev);
DEF_GETTER16(ino);
DEF_GETTER16(mode);
DEF_GETTER16(uid);
DEF_GETTER16(gid);
DEF_GETTER16(nlink);
DEF_GETTER16(rdev);
DEF_GETTER32(mtime);
DEF_GETTER16(namesize);
DEF_GETTER32(filesize);

#undef DEF_GETTER16
#undef DEF_GETTER32

int cpio_valid(const struct header_old_cpio* d, unsigned long dsize);

const struct header_old_cpio* cpio_init_iter(const void *d, long dsize);
const struct header_old_cpio* cpio_goto_next(const struct header_old_cpio* d, unsigned long *pdsize);

const char* get_filename(const struct header_old_cpio* d, uint16_t *len);

const uint8_t* get_filedata(const struct header_old_cpio* d, uint32_t *len);

#define CPIO_TYPE_MASK          (0170000)  /* This masks the file type bits. */
#define CPIO_SOCKET_TYPE_MASK   (0140000)  /* File type value for sockets. */
#define CPIO_SYMLINK_TYPE_MASK  (0120000)  /* File type value for symbolic links.  For symbolic links, the link body is stored as file data. */
#define CPIO_FILE_TYPE_MASK     (0100000)  /* File type value for regular files. */
#define CPIO_BLOCK_TYPE_MASK    (0060000)  /* File type value for block special devices. */
#define CPIO_DIR_TYPE_MASK      (0040000)  /* File type value for directories. */
#define CPIO_CHAR_TYPE_MASK     (0020000)  /* File type value for character special devices. */
#define CPIO_PIPE_TYPE_MASK     (0010000)  /* File type value for named pipes or FIFOs. */

#define CPIO_FILEDIR_TYPE       (CPIO_FILE_TYPE_MASK | CPIO_DIR_TYPE_MASK)

#define CPIO_MODE_MASK          (0007777)
#define CPIO_SUID_MODE_MASK     (0004000)  /* SUID bit. */
#define CPIO_SGID_MODE_MASK     (0002000)  /* SGID bit. */
#define CPIO_SBIT_MODE_MASK     (0001000)  /* Sticky bit.  On some systems, this modifies the behavior of executables and/or directories. */
#define CPIO_PERM_MODE_MASK     (0000777)  /* The lower 9 bits specify read/write/execute permissions for world, group, and user following standard POSIX con-ventions. */

/* */

// Possible error codes, these are negative to allow
// valid positive return values
enum cpio_error {
    CPIO_ERR_OK          = 0,    // No error
    CPIO_ERR_NEXIST      = -1,   // File NOT EXISTS
    CPIO_ERR_SEEK_OUT    = -2,   // seek command is out of the file
    CPIO_ERR_PARAM       = -3,   // parameter error
    CPIO_ERR_UNKNOWN     = -4,   // general error
};

typedef uint32_t cpio_size_t;
typedef int32_t cpio_ssize_t;
typedef int32_t cpio_soff_t;
typedef uint32_t cpio_off_t;

typedef struct cpiofs {
    const struct header_old_cpio *head;
    cpio_size_t size;
    unsigned int resource_count;
} cpiofs_t;

typedef struct cpio_info {
    uint16_t type;
    uint16_t mode;
    cpio_size_t size;
    const char * filename;
    uint16_t filenames;
    const char * filepath;
    uint16_t filepaths;
} cpio_info_t;

typedef struct cpio_file {
    cpiofs_t *fs;
    const struct header_old_cpio *head;
    cpio_off_t pos;
} cpio_file_t;

typedef struct cpio_dir {
    cpiofs_t *fs;
    const struct header_old_cpio *head;
    const struct header_old_cpio *pos;
    long size;
} cpio_dir_t;

// File seek flags
typedef enum cpio_whence_flags {
    CPIO_SEEK_SET = 0,   // Seek relative to an absolute position
    CPIO_SEEK_CUR = 1,   // Seek relative to the current file position
    CPIO_SEEK_END = 2,   // Seek relative to the end of the file
} cpio_whence_flags_t;

// Find info about a file or directory
//
// Fills out the info structure, based on the specified file or directory.
// Returns a negative error code on failure.
int cpiofs_stat(const cpiofs_t *fs, const char *path, cpio_info_t *info);

// Open a file
//
// Returns a negative error code on failure.
int cpiofs_file_open(cpiofs_t *fs, cpio_file_t *file, const char *path);

// Close a file
//
// Returns a negative error code on failure.
int cpiofs_file_close(cpio_file_t *file);

// Read data from file
//
// Takes a buffer and size indicating where to store the read data.
// Returns the number of bytes read, or a negative error code on failure.
cpio_ssize_t cpiofs_file_read(cpio_file_t *file, void *buffer, cpio_size_t size);

// Change the position of the file
//
// The change in position is determined by the offset and whence flag.
// Returns the new position of the file, or a negative error code on failure.
cpio_soff_t cpiofs_file_seek(cpio_file_t *file, cpio_soff_t off, cpio_whence_flags_t whence);

// Return the position of the file
//
// Returns the position of the file, or a negative error code on failure.
static inline cpio_soff_t cpiofs_file_tell(cpio_file_t *file) {
    return cpiofs_file_seek(file, 0, CPIO_SEEK_CUR);
}

// Change the position of the file to the beginning of the file
//
// Returns a negative error code on failure.
static inline int cpiofs_file_rewind(cpio_file_t *file) {
    return cpiofs_file_seek(file, 0, CPIO_SEEK_SET);
}

// Return the size of the file
//
// Returns the size of the file, or a negative error code on failure.
static inline cpio_soff_t cpiofs_file_size(cpio_file_t *file) {
    return cpiofs_file_seek(file, 0, CPIO_SEEK_END);
}

// Open a directory
//
// Once open a directory can be used with read to iterate over files.
// Returns a negative error code on failure.
int cpiofs_dir_open(cpiofs_t *fs, cpio_dir_t *dir, const char *path);

// Close a directory
//
// Releases any allocated resources.
// Returns a negative error code on failure.
int cpiofs_dir_close(cpio_dir_t *dir);

// Read an entry in the directory
//
// Fills out the info structure, based on the specified file or directory.
// Returns a positive value on success, 0 at the end of directory,
// or a negative error code on failure.
int cpiofs_dir_read(cpio_dir_t *dir, cpio_info_t *info);

#endif