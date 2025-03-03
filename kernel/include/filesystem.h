/**
 * filesystem.h - File system interface for NeuroOS
 * 
 * This file contains the file system interface definitions and declarations.
 */

#ifndef NEUROOS_FILESYSTEM_H
#define NEUROOS_FILESYSTEM_H

#include <stddef.h>
#include <stdint.h>

// File system types
#define FS_TYPE_UNKNOWN  0
#define FS_TYPE_EXT2     1
#define FS_TYPE_EXT3     2
#define FS_TYPE_EXT4     3
#define FS_TYPE_FAT12    4
#define FS_TYPE_FAT16    5
#define FS_TYPE_FAT32    6
#define FS_TYPE_NTFS     7
#define FS_TYPE_EXFAT    8
#define FS_TYPE_HFS      9
#define FS_TYPE_HFSPLUS  10
#define FS_TYPE_APFS     11
#define FS_TYPE_UFS      12
#define FS_TYPE_XFS      13
#define FS_TYPE_JFS      14
#define FS_TYPE_REISERFS 15
#define FS_TYPE_BTRFS    16
#define FS_TYPE_ZFS      17
#define FS_TYPE_ISO9660  18
#define FS_TYPE_UDF      19
#define FS_TYPE_NFS      20
#define FS_TYPE_CIFS     21
#define FS_TYPE_SMB      22
#define FS_TYPE_SMBFS    23
#define FS_TYPE_NCPFS    24
#define FS_TYPE_CODA     25
#define FS_TYPE_AFS      26
#define FS_TYPE_PROC     27
#define FS_TYPE_SYSFS    28
#define FS_TYPE_DEVPTS   29
#define FS_TYPE_DEVTMPFS 30
#define FS_TYPE_TMPFS    31
#define FS_TYPE_RAMFS    32
#define FS_TYPE_SQUASHFS 33
#define FS_TYPE_CRAMFS   34
#define FS_TYPE_ROMFS    35
#define FS_TYPE_JFFS2    36
#define FS_TYPE_UBIFS    37
#define FS_TYPE_MINIX    38
#define FS_TYPE_VFAT     39
#define FS_TYPE_MSDOS    40
#define FS_TYPE_UMSDOS   41
#define FS_TYPE_AUTOFS   42
#define FS_TYPE_DEVFS    43
#define FS_TYPE_DEBUGFS  44
#define FS_TYPE_SECURITYFS 45
#define FS_TYPE_SELINUXFS 46
#define FS_TYPE_FUSECTL  47
#define FS_TYPE_FUSE     48
#define FS_TYPE_FUSEBLK  49
#define FS_TYPE_FUSECTL  50
#define FS_TYPE_OVERLAY  51
#define FS_TYPE_ECRYPTFS 52
#define FS_TYPE_ENCFS    53
#define FS_TYPE_CEPH     54
#define FS_TYPE_GLUSTERFS 55
#define FS_TYPE_GPFS     56
#define FS_TYPE_OCFS2    57
#define FS_TYPE_NILFS2   58
#define FS_TYPE_F2FS     59
#define FS_TYPE_AUFS     60
#define FS_TYPE_UNIONFS  61
#define FS_TYPE_HOSTFS   62
#define FS_TYPE_USERFS   63

// File system flags
#define FS_FLAG_READ_ONLY      (1 << 0)
#define FS_FLAG_SYNCHRONOUS    (1 << 1)
#define FS_FLAG_NO_EXEC        (1 << 2)
#define FS_FLAG_NO_SUID        (1 << 3)
#define FS_FLAG_NO_DEV         (1 << 4)
#define FS_FLAG_NO_ATIME       (1 << 5)
#define FS_FLAG_NODIRATIME     (1 << 6)
#define FS_FLAG_RELATIME       (1 << 7)
#define FS_FLAG_STRICTATIME    (1 << 8)
#define FS_FLAG_LAZYTIME       (1 << 9)
#define FS_FLAG_MANDLOCK       (1 << 10)
#define FS_FLAG_DIRSYNC        (1 << 11)
#define FS_FLAG_SYNCHRONOUS    (1 << 12)
#define FS_FLAG_NO_TRIM        (1 << 13)
#define FS_FLAG_JOURNAL_DATA   (1 << 14)
#define FS_FLAG_ORDERED_DATA   (1 << 15)
#define FS_FLAG_WRITEBACK_DATA (1 << 16)
#define FS_FLAG_ACLS           (1 << 17)
#define FS_FLAG_XATTR          (1 << 18)
#define FS_FLAG_POSIX_ACL      (1 << 19)
#define FS_FLAG_USER_XATTR     (1 << 20)
#define FS_FLAG_TRUSTED_XATTR  (1 << 21)
#define FS_FLAG_SECURITY_XATTR (1 << 22)
#define FS_FLAG_CASE_INSENSITIVE (1 << 23)
#define FS_FLAG_CASE_SENSITIVE (1 << 24)
#define FS_FLAG_UTF8           (1 << 25)
#define FS_FLAG_ASCII          (1 << 26)
#define FS_FLAG_UNICODE        (1 << 27)
#define FS_FLAG_SHORTNAME      (1 << 28)
#define FS_FLAG_LONGNAME       (1 << 29)
#define FS_FLAG_MIXED          (1 << 30)
#define FS_FLAG_RESERVED       (1 << 31)

// File types
#define FILE_TYPE_UNKNOWN      0
#define FILE_TYPE_REGULAR      1
#define FILE_TYPE_DIRECTORY    2
#define FILE_TYPE_SYMLINK      3
#define FILE_TYPE_BLOCK_DEVICE 4
#define FILE_TYPE_CHAR_DEVICE  5
#define FILE_TYPE_FIFO         6
#define FILE_TYPE_SOCKET       7

// File flags
#define FILE_FLAG_READ         (1 << 0)
#define FILE_FLAG_WRITE        (1 << 1)
#define FILE_FLAG_APPEND       (1 << 2)
#define FILE_FLAG_CREATE       (1 << 3)
#define FILE_FLAG_TRUNCATE     (1 << 4)
#define FILE_FLAG_EXCL         (1 << 5)
#define FILE_FLAG_NONBLOCK     (1 << 6)
#define FILE_FLAG_SYNC         (1 << 7)
#define FILE_FLAG_ASYNC        (1 << 8)
#define FILE_FLAG_DIRECT       (1 << 9)
#define FILE_FLAG_LARGEFILE    (1 << 10)
#define FILE_FLAG_DIRECTORY    (1 << 11)
#define FILE_FLAG_NOFOLLOW     (1 << 12)
#define FILE_FLAG_NOATIME      (1 << 13)
#define FILE_FLAG_CLOEXEC      (1 << 14)
#define FILE_FLAG_PATH         (1 << 15)
#define FILE_FLAG_TMPFILE      (1 << 16)
#define FILE_FLAG_NDELAY       (1 << 17)
#define FILE_FLAG_DSYNC        (1 << 18)
#define FILE_FLAG_RSYNC        (1 << 19)
#define FILE_FLAG_NONBLOCK     (1 << 20)
#define FILE_FLAG_NOCTTY       (1 << 21)
#define FILE_FLAG_BINARY       (1 << 22)
#define FILE_FLAG_TEXT         (1 << 23)
#define FILE_FLAG_RANDOM       (1 << 24)
#define FILE_FLAG_SEQUENTIAL   (1 << 25)
#define FILE_FLAG_TEMPORARY    (1 << 26)
#define FILE_FLAG_BACKUP       (1 << 27)
#define FILE_FLAG_POSIX        (1 << 28)
#define FILE_FLAG_NOINHERIT    (1 << 29)
#define FILE_FLAG_DELETEONCLOSE (1 << 30)
#define FILE_FLAG_RESERVED     (1 << 31)

// File modes
#define FILE_MODE_READ         0x01
#define FILE_MODE_WRITE        0x02
#define FILE_MODE_EXECUTE      0x04
#define FILE_MODE_USER_READ    0x0100
#define FILE_MODE_USER_WRITE   0x0080
#define FILE_MODE_USER_EXEC    0x0040
#define FILE_MODE_GROUP_READ   0x0020
#define FILE_MODE_GROUP_WRITE  0x0010
#define FILE_MODE_GROUP_EXEC   0x0008
#define FILE_MODE_OTHER_READ   0x0004
#define FILE_MODE_OTHER_WRITE  0x0002
#define FILE_MODE_OTHER_EXEC   0x0001
#define FILE_MODE_SETUID       0x0800
#define FILE_MODE_SETGID       0x0400
#define FILE_MODE_STICKY       0x0200
#define FILE_MODE_DEFAULT      0x0666

// File seek modes
#define FILE_SEEK_SET          0
#define FILE_SEEK_CUR          1
#define FILE_SEEK_END          2

// File system information structure
typedef struct {
    uint32_t type;
    uint32_t flags;
    uint64_t total_blocks;
    uint64_t free_blocks;
    uint64_t available_blocks;
    uint64_t total_inodes;
    uint64_t free_inodes;
    uint64_t available_inodes;
    uint64_t block_size;
    uint64_t fragment_size;
    uint64_t inode_size;
    uint64_t max_filename_length;
    uint64_t max_path_length;
    uint64_t max_symlink_length;
    uint64_t max_file_size;
    uint64_t max_file_count;
    uint64_t max_dir_size;
    uint64_t max_dir_count;
    uint64_t max_open_files;
    uint64_t max_links;
    uint64_t max_read_size;
    uint64_t max_write_size;
    uint64_t max_io_size;
    uint64_t max_seek_size;
    uint64_t max_file_locks;
    uint64_t max_file_locks_per_file;
    uint64_t max_file_locks_per_process;
    uint64_t max_file_locks_per_user;
    uint64_t max_file_locks_per_group;
    uint64_t max_file_locks_per_system;
    uint64_t max_file_locks_per_inode;
    uint64_t max_file_locks_per_superblock;
    uint64_t max_file_locks_per_filesystem;
    uint64_t max_file_locks_per_mount;
    uint64_t max_file_locks_per_namespace;
    uint64_t max_file_locks_per_domain;
    uint64_t max_file_locks_per_realm;
    uint64_t max_file_locks_per_universe;
    char name[256];
    char label[256];
    char uuid[256];
    char device[256];
    char mount_point[256];
    char options[1024];
} filesystem_info_t;

// File information structure
typedef struct {
    uint32_t type;
    uint32_t flags;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t blocks;
    uint64_t block_size;
    uint64_t atime;
    uint64_t mtime;
    uint64_t ctime;
    uint64_t btime;
    uint64_t inode;
    uint64_t device;
    uint64_t rdev;
    uint64_t nlink;
    uint64_t blksize;
    uint64_t attributes;
    uint64_t generation;
    uint64_t flags_mask;
    uint64_t acl;
    uint64_t selinux;
    uint64_t capabilities;
    uint64_t xattr;
    uint64_t extents;
    uint64_t fragments;
    uint64_t reserved[8];
    char name[256];
    char path[1024];
    char symlink[1024];
} file_info_t;

// Directory entry structure
typedef struct {
    uint32_t type;
    uint64_t inode;
    char name[256];
} directory_entry_t;

// File system initialization and shutdown
void filesystem_init(void);
void filesystem_shutdown(void);

// File system operations
int filesystem_mount(const char* source, const char* target, const char* type, uint32_t flags, const void* data);
int filesystem_unmount(const char* target, int flags);
int filesystem_remount(const char* target, uint32_t flags);
int filesystem_sync(void);
int filesystem_sync_fs(const char* target);
int filesystem_get_info(const char* path, filesystem_info_t* info);
int filesystem_get_stats(const char* path, void* stats);
int filesystem_format(const char* device, const char* type, uint32_t flags, const void* data);
int filesystem_check(const char* device, const char* type, uint32_t flags, const void* data);
int filesystem_repair(const char* device, const char* type, uint32_t flags, const void* data);
int filesystem_resize(const char* device, uint64_t size);
int filesystem_tune(const char* device, const char* type, uint32_t flags, const void* data);
int filesystem_label(const char* device, const char* label);
int filesystem_uuid(const char* device, const char* uuid);

// File operations
int filesystem_open(const char* path, uint32_t flags, uint32_t mode);
int filesystem_close(int fd);
int filesystem_read(int fd, void* buffer, size_t size);
int filesystem_write(int fd, const void* buffer, size_t size);
int filesystem_seek(int fd, int64_t offset, int whence);
int filesystem_tell(int fd);
int filesystem_truncate(int fd, uint64_t size);
int filesystem_flush(int fd);
int filesystem_ioctl(int fd, int request, void* arg);
int filesystem_fcntl(int fd, int cmd, void* arg);
int filesystem_fstat(int fd, file_info_t* info);
int filesystem_fchmod(int fd, uint32_t mode);
int filesystem_fchown(int fd, uint32_t uid, uint32_t gid);
int filesystem_fsync(int fd);
int filesystem_fdatasync(int fd);
int filesystem_ftruncate(int fd, uint64_t size);
int filesystem_fallocate(int fd, int mode, uint64_t offset, uint64_t len);
int filesystem_fadvise(int fd, uint64_t offset, uint64_t len, int advice);
int filesystem_flock(int fd, int operation);
int filesystem_readahead(int fd, uint64_t offset, size_t count);
int filesystem_sync_file_range(int fd, uint64_t offset, uint64_t nbytes, unsigned int flags);
int filesystem_copy_file_range(int fd_in, uint64_t* off_in, int fd_out, uint64_t* off_out, size_t len, unsigned int flags);

// Directory operations
int filesystem_mkdir(const char* path, uint32_t mode);
int filesystem_rmdir(const char* path);
int filesystem_chdir(const char* path);
int filesystem_getcwd(char* buf, size_t size);
int filesystem_opendir(const char* path);
int filesystem_closedir(int fd);
int filesystem_readdir(int fd, directory_entry_t* entry);
int filesystem_rewinddir(int fd);
int filesystem_seekdir(int fd, long offset);
int filesystem_telldir(int fd);
int filesystem_scandir(const char* path, directory_entry_t** entries, int (*filter)(const directory_entry_t*), int (*compar)(const directory_entry_t**, const directory_entry_t**));
int filesystem_alphasort(const directory_entry_t** a, const directory_entry_t** b);
int filesystem_versionsort(const directory_entry_t** a, const directory_entry_t** b);

// File system path operations
int filesystem_access(const char* path, int mode);
int filesystem_chmod(const char* path, uint32_t mode);
int filesystem_chown(const char* path, uint32_t uid, uint32_t gid);
int filesystem_lchown(const char* path, uint32_t uid, uint32_t gid);
int filesystem_stat(const char* path, file_info_t* info);
int filesystem_lstat(const char* path, file_info_t* info);
int filesystem_readlink(const char* path, char* buf, size_t size);
int filesystem_symlink(const char* target, const char* linkpath);
int filesystem_link(const char* oldpath, const char* newpath);
int filesystem_unlink(const char* path);
int filesystem_rename(const char* oldpath, const char* newpath);
int filesystem_truncate(const char* path, uint64_t size);
int filesystem_utime(const char* path, const void* times);
int filesystem_utimes(const char* path, const void* times);
int filesystem_lutimes(const char* path, const void* times);
int filesystem_futimes(int fd, const void* times);
int filesystem_futimens(int fd, const void* times);
int filesystem_utimensat(int dirfd, const char* path, const void* times, int flags);
int filesystem_mknod(const char* path, uint32_t mode, uint64_t dev);
int filesystem_mkfifo(const char* path, uint32_t mode);
int filesystem_mkstemp(char* template);
int filesystem_mkostemp(char* template, int flags);
int filesystem_mkostemps(char* template, int suffixlen, int flags);
int filesystem_mkdtemp(char* template);

// File system extended attribute operations
int filesystem_getxattr(const char* path, const char* name, void* value, size_t size);
int filesystem_lgetxattr(const char* path, const char* name, void* value, size_t size);
int filesystem_fgetxattr(int fd, const char* name, void* value, size_t size);
int filesystem_setxattr(const char* path, const char* name, const void* value, size_t size, int flags);
int filesystem_lsetxattr(const char* path, const char* name, const void* value, size_t size, int flags);
int filesystem_fsetxattr(int fd, const char* name, const void* value, size_t size, int flags);
int filesystem_listxattr(const char* path, char* list, size_t size);
int filesystem_llistxattr(const char* path, char* list, size_t size);
int filesystem_flistxattr(int fd, char* list, size_t size);
int filesystem_removexattr(const char* path, const char* name);
int filesystem_lremovexattr(const char* path, const char* name);
int filesystem_fremovexattr(int fd, const char* name);

// File system ACL operations
int filesystem_acl_get_file(const char* path, int type, void* acl);
int filesystem_acl_set_file(const char* path, int type, const void* acl);
int filesystem_acl_get_fd(int fd, void* acl);
int filesystem_acl_set_fd(int fd, const void* acl);
int filesystem_acl_delete_def_file(const char* path);
int filesystem_acl_get_entry(const void* acl, int entry_id, void** entry);
int filesystem_acl_valid(const void* acl);
int filesystem_acl_copy_entry(void* dest, const void* src);
int filesystem_acl_create_entry(void** acl, void** entry);
int filesystem_acl_delete_entry(void* acl, void* entry);
int filesystem_acl_get_tag_type(const void* entry, int* tag_type);
int filesystem_acl_set_tag_type(void* entry, int tag_type);
int filesystem_acl_get_qualifier(const void* entry);
int filesystem_acl_set_qualifier(void* entry, const void* qualifier);
int filesystem_acl_get_permset(const void* entry, void** permset);
int filesystem_acl_set_permset(void* entry, const void* permset);
int filesystem_acl_add_perm(void* permset, int perm);
int filesystem_acl_calc_mask(void** acl);
int filesystem_acl_clear_perms(void* permset);
int filesystem_acl_delete_perm(void* permset, int perm);
int filesystem_acl_get_perm(const void* permset, int perm);
int filesystem_acl_from_text(const char* text);
char* filesystem_acl_to_text(const void* acl, size_t* len);
int filesystem_acl_free(void* acl);
int filesystem_acl_to_any_text(const void* acl, const char* prefix, char separator, int options);

// File system path manipulation
char* filesystem_basename(const char* path);
char* filesystem_dirname(const char* path);
char* filesystem_realpath(const char* path, char* resolved_path);
int filesystem_canonicalize_path(const char* path, char* resolved_path, size_t size);
int filesystem_normalize_path(const char* path, char* normalized_path, size_t size);
int filesystem_is_absolute_path(const char* path);
int filesystem_is_relative_path(const char* path);
int filesystem_is_root_path(const char* path);
int filesystem_is_parent_path(const char* path);
int filesystem_is_current_path(const char* path);
int filesystem_is_home_path(const char* path);
int filesystem_is_valid_path(const char* path);
int filesystem_is_directory_path(const char* path);
int filesystem_is_file_path(const char* path);
int filesystem_is_symlink_path(const char* path);
int filesystem_is_device_path(const char* path);
int filesystem_is_socket_path(const char* path);
int filesystem_is_fifo_path(const char* path);
int filesystem_is_hidden_path(const char* path);
int filesystem_is_dot_path(const char* path);
int filesystem_is_dotdot_path(const char* path);
int filesystem_is_dot_or_dotdot_path(const char* path);
int filesystem_is_empty_path(const char* path);
int filesystem_is_same_path(const char* path1, const char* path2);
int filesystem_is_parent_of_path(const char* parent, const char* path);
int filesystem_is_child_of_path(const char* child, const char* path);
int filesystem_is_ancestor_of_path(const char* ancestor, const char* path);
int filesystem_is_descendant_of_path(const char* descendant, const char* path);
int filesystem_is_subpath_of_path(const char* subpath, const char* path);
int filesystem_is_prefix_of_path(const char* prefix, const char* path);
int filesystem_is_suffix_of_path(const char* suffix, const char* path);
int filesystem_is_component_of_path(const char* component, const char* path);
int filesystem_is_extension_of_path(const char* extension, const char* path);
int filesystem_get_path_components(const char* path, char** components, int* count);
int filesystem_get_path_component(const char* path, int index, char* component, size_t size);
int filesystem_get_path_component_count(const char* path);
int filesystem_get_path_extension(const char* path, char* extension, size_t size);
int filesystem_get_path_basename(const char* path, char* basename, size_t size);
int filesystem_get_path_dirname(const char* path, char* dirname, size_t size);
int filesystem_get_path_root(const char* path, char* root, size_t size);
int filesystem_get_path_drive(const char* path, char* drive, size_t size);
int filesystem_get_path_device(const char* path, char* device, size_t size);
int filesystem_get_path_parent(const char* path, char* parent, size_t size);
int filesystem_get_path_ancestors(const char* path, char** ancestors, int* count);
int filesystem_get_path_descendants(const char* path, char** descendants, int* count);
int filesystem_get_path_siblings(const char* path, char** siblings, int* count);
int filesystem_get_path_children(const char* path, char** children, int* count);
int filesystem_get_path_depth(const char* path);
int filesystem_get_path_length(const char* path);
int filesystem_get_path_size(const char* path);
int filesystem_get_path_type(const char* path);
int filesystem_get_path_flags(const char* path);
int filesystem_get_path_mode(const char* path);
int filesystem_get_path_owner(const char* path);
int filesystem_get_path_group(const char* path);
int filesystem_get_path_atime(const char* path);
int filesystem_get_path_mtime(const char* path);
int filesystem_get_path_ctime(const char* path);
int filesystem_get_path_btime(const char* path);
int filesystem_get_path_inode(const char* path);
int filesystem_get_path_device(const char* path);
int filesystem_get_path_rdev(const char* path);
int filesystem_get_path_nlink(const char* path);
int filesystem_get_path_blksize(const char* path);
int filesystem_get_path_blocks(const char* path);
int filesystem_get_path_attributes(const char* path);
int filesystem_get_path_generation(const char* path);
int filesystem_get_path_flags_mask(const char* path);
int filesystem_get_path_acl(const char* path);
int filesystem_get_path_selinux(const char* path);
int filesystem_get_path_capabilities(const char* path);
int filesystem_get_path_xattr(const char* path);
int filesystem_get_path_extents(const char* path);
int filesystem_get_path_fragments(const char* path);

// File system debug functions
void filesystem_dump_info(const char* path);
void filesystem_dump_stats(const char* path);
void filesystem_dump_super(const char* path);
void filesystem_dump_inode(const char* path);
void filesystem_dump_dentry(const char* path);
void filesystem_dump_file(const char* path);
void filesystem_dump_dir(const char* path);
void filesystem_dump_symlink(const char* path);
void filesystem_dump_block(const char* path, uint64_t block);
void filesystem_dump_extent(const char* path);
void filesystem_dump_journal(const char* path);
void filesystem_dump_bitmap(const char* path);
void filesystem_dump_tree(const char* path);
void filesystem_dump_xattr(const char* path);
void filesystem_dump_acl(const char* path);
void filesystem_dump_selinux(const char* path);
void filesystem_dump_capabilities(const char* path);
void filesystem_dump_quota(const char* path);
void filesystem_dump_mount(const char* path);
void filesystem_dump_namespace(const char* path);
void filesystem_dump_vfs(void);
void filesystem_dump_dcache(void);
void filesystem_dump_icache(void);
void filesystem_dump_buffers(void);
void filesystem_dump_page_cache(void);
void filesystem_dump_writeback(void);
void filesystem_dump_locks(void);
void filesystem_dump_lease(void);
void filesystem_dump_aio(void);
void filesystem_dump_bdi(void);
void filesystem_dump_sb(void);
void filesystem_dump_fs(void);

#endif // NEUROOS_FILESYSTEM_H
