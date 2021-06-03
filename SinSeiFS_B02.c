#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

char dirpath[50] = "/home/sheinna/Documents";

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .access = xmp_access,
    .readlink = xmp_readlink,
    .readdir = xmp_readdir,
    .mknod = xmp_mknod,
    .mkdir = xmp_mkdir,
    .symlink = xmp_symlink,
    .unlink = xmp_unlink,
    .rmdir = xmp_rmdir,
    .rename = xmp_rename,
    .link = xmp_link,
    .chmod = xmp_chmod,
    .chown = xmp_chown,
    .truncate = xmp_truncate,
    .utimens = xmp_utimens,
    .open = xmp_open,
    .read = xmp_read,
    .write = xmp_write,
    .statfs = xmp_statfs,
    .create = xmp_create,
    .release = xmp_release,
    .fsync = xmp_fsync,
};

static int xmp_getattr(const char *path, struct stat *stbuf)
{
    int res;
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    res = lstat(cekPath(fpath), stbuf);
    writeI("LS", fpath);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_access(const char *path, int mask)
{
    int res;
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    res = access(cekPath(fpath), mask);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
    int res;
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    res = readlink(cekPath(fpath), buf, size - 1);
    if (res == -1)
        return -errno;
    buf[res] = '\0';
    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    int res = 0;

    DIR *dp;
    struct dirent *de;
    (void)offset;
    (void)fi;
    dp = opendir(cekPath(fpath));
    if (dp == NULL)
        return -errno;

    int flag = encrFolder(fpath);
    while ((de = readdir(dp)) != NULL)
    {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        char nama[1000000];
        strcpy(nama, de->d_name);
        if (flag == 1)
        {
            if (de->d_type == DT_REG)
                decrypt(nama, 1);
            else if (de->d_type == DT_DIR && strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0)
                decrypt(nama, 0);
            res = (filler(buf, nama, &st, 0));
            if (res != 0)
                break;
        }
        else
        {
            res = (filler(buf, nama, &st, 0));
            if (res != 0)
                break;
        }
    }
    closedir(dp);
    writeI("CD", fpath);
    return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    cekPath(fpath);
    int res;

    if (S_ISREG(mode))
    {
        res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0)
            res = close(res);
    }
    else if (S_ISFIFO(mode))
        res = mkfifo(fpath, mode);
    else
        res = mknod(fpath, mode, rdev);
    if (res == -1)
        return -errno;

    writeI("CREATE", fpath);
    return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);

    int res;

    res = mkdir(cekPath(fpath), mode);
    if (res == -1)
        return -errno;

    char cek_substr[1024];
    if (lastPart(fpath) == 0)
        return 0;
    char filePath[1000000];
    strcpy(filePath, lastPart(fpath));
    substring(filePath, cek_substr, 0, 6);
    if (strcmp(cek_substr, "encv1_") == 0) //folder encrypt1
    {
        encrypt1(fpath, 1);
    }
    else if (strcmp(cek_substr, "encv2_") == 0) //folder encrypt2
    {
        encrypt2(fpath, 1);
    }
    writeI("MKDIR", fpath);
    return 0;
}

static int xmp_unlink(const char *path)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    int res;

    res = unlink(cekPath(fpath));
    writeW("REMOVE", fpath);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_rmdir(const char *path)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    int res;

    res = rmdir(cekPath(fpath));
    writeW("RMDIR", fpath);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
    int res;

    res = symlink(from, to);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_rename(const char *from, const char *to)
{
    char ffrom[1000];
    mixPath(ffrom, dirpath, from);

    char fto[1000];
    mixPath(fto, dirpath, to);

    int res;

    res = rename(cekPath(ffrom), cekPath(fto));

    if (res == -1)
        return -errno;

    int fromm = 0, too = 0;
    char cek_substr[1024], cek2[1024];
    if (lastPart(ffrom) == 0)
        return 0;
    char filePath[1000000];
    strcpy(filePath, lastPart(ffrom));
    substring(filePath, cek_substr, 0, 6);
    if (strcmp(cek_substr, "encv1_") == 0) //folder encrypt1
    {
        fromm = 1;
    }
    else if (strcmp(cek_substr, "encv2_") == 0) //folder encrypt2
    {
        fromm = 2;
    }

    if (lastPart(fto) == 0)
        return 0;
    strcpy(filePath, lastPart(fto));
    substring(filePath, cek_substr, 0, 6);
    if (strcmp(cek2, "encv1_") == 0) //folder decrypt1
    {
        too = 1;
    }
    else if (strcmp(cek2, "encv2_") == 0) //folder decrypt2
    {
        too = 2;
    }

    if (fromm == 0 && too == 1)
        encrypt1(fto, 1);
    else if (fromm == 0 && too == 2)
        encrypt2(fto, 1);
    else if (fromm == 1 && too != 1)
        encrypt1(fto, -1);
    else if (fromm == 1 && too == 2)
        encrypt2(fto, 1);
    else if (fromm == 2 && too != 1)
        encrypt1(fto, -1);
    else if (fromm == 2 && too == 2)
        encrypt2(fto, 1);
    writeI("MOVE", ffrom);

    return 0;
}

static int xmp_link(const char *from, const char *to)
{
    int res;

    res = link(from, to);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    int res;

    res = chmod(cekPath(fpath), mode);
    writeI("CHMOD", fpath);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    int res;

    res = lchown(cekPath(fpath), uid, gid);
    writeI("CHOWN", fpath);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    int res;

    res = truncate(cekPath(fpath), size);
    writeI("TRUNCATE", fpath);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    int res;
    struct timeval tv[2];

    tv[0].tv_sec = ts[0].tv_sec;
    tv[0].tv_usec = ts[0].tv_nsec / 1000;
    tv[1].tv_sec = ts[1].tv_sec;
    tv[1].tv_usec = ts[1].tv_nsec / 1000;

    res = utimes(cekPath(fpath), tv);
    writeI("UTIMENS", fpath);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    int res;

    res = open(cekPath(fpath), fi->flags);
    writeI("OPEN", fpath);
    if (res == -1)
        return -errno;
    close(res);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    int fd = 0;
    int res = 0;

    (void)fi;
    fd = open(cekPath(fpath), O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;
    close(fd);

    writeI("CAT", fpath);
    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    int fd;
    int res;

    (void)fi;
    fd = open(cekPath(fpath), O_WRONLY);
    if (fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    writeI("WRITE", fpath);
    close(fd);
    return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    int res;

    res = statvfs(cekPath(fpath), stbuf);
    if (res == -1)
        return -errno;
    return 0;
}

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    char fpath[1000];
    mixPath(fpath, dirpath, path);
    (void)fi;

    int res;
    res = creat(cekPath(fpath), mode);
    if (res == -1)
        return -errno;

    writeI("CREAT", fpath);
    close(res);
    return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
    (void)path;
    (void)fi;
    return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
                     struct fuse_file_info *fi)
{
    (void)path;
    (void)isdatasync;
    (void)fi;
    return 0;
}