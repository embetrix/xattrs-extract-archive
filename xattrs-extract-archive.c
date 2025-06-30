// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Embetrix
 * Author: ayoub.zaki@embetrix.com
 * 
 * License: GPL-2.0-or-later
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#define _GNU_SOURCE
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <utime.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int xattrs_extract_archive(const char *filename, const char *dest_path) {
    struct archive *a;
    struct archive_entry *entry;
    int r;

    a = archive_read_new();
    archive_read_support_format_tar(a);
    archive_read_support_filter_gzip(a);

    if ((r = archive_read_open_filename(a, filename, 10240))) {
        fprintf(stderr, "Failed to open archive: %s\n", archive_error_string(a));
        return 1;
    }

    while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        const char *relpath = archive_entry_pathname(entry);
        char fullpath[4096];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dest_path, relpath);

        mode_t mode = archive_entry_perm(entry);
        uid_t uid = (uid_t)archive_entry_uid(entry);
        gid_t gid = (gid_t)archive_entry_gid(entry);

        // check parent directory exists
        char *slash = strrchr(fullpath, '/');
        if (slash) {
            *slash = '\0';
            mkdir(fullpath, 0755);
            *slash = '/';
        }

        if (archive_entry_filetype(entry) == AE_IFDIR) {
            mkdir(fullpath, mode);
            if (chown(fullpath, uid, gid) < 0)
                perror("chown dir");
            continue;
        }

        // create the file node without opening
        if (mknod(fullpath, S_IFREG | mode, 0) < 0 && errno != EEXIST) {
            perror("mknod");
            continue;
        }

        // apply xattrs before open()
        archive_entry_xattr_reset(entry);
        const char *name;
        const void *value;
        size_t value_len;
        while (archive_entry_xattr_next(entry, &name, &value, &value_len) == ARCHIVE_OK) {
            if (setxattr(fullpath, name, value, value_len, 0) < 0) {
                fprintf(stderr, "setxattr(%s) failed: %s\n", name, strerror(errno));
            }
        }

        // open + write
        int fd = open(fullpath, O_WRONLY);
        if (fd < 0) {
            perror("open");
            continue;
        }

        const void *buff;
        size_t size;
        la_int64_t offset;
        while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
            if (pwrite(fd, buff, size, offset) != (ssize_t)size) {
                perror("pwrite");
                break;
            }
        }

        close(fd);

        // restore timestamps
        struct timespec times[2];
        times[0].tv_sec = archive_entry_atime(entry);
        times[0].tv_nsec = archive_entry_atime_nsec(entry);
        times[1].tv_sec = archive_entry_mtime(entry);
        times[1].tv_nsec = archive_entry_mtime_nsec(entry);
        utimensat(AT_FDCWD, fullpath, times, 0);

        // apply metadata
        if (chown(fullpath, uid, gid) < 0)
            perror("chown");
        if (chmod(fullpath, mode) < 0)
            perror("chmod");
    }

    archive_read_close(a);
    archive_read_free(a);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <archive.tar.gz> <destination_folder>\n", argv[0]);
        return 1;
    }

    return xattrs_extract_archive(argv[1], argv[2]);
}
