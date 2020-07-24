#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <NickelHook.h>

static typeof(readdir)     *o_readdir;
static typeof(readdir64)   *o_readdir64;
static typeof(readdir_r)   *o_readdir_r;
static typeof(readdir64_r) *o_readdir64_r;

#ifndef NO_USE_FULL_PATH
static char *dirpaths[65535];
static typeof(opendir)     *o_opendir;
static typeof(fdopendir)   *o_fdopendir;
static typeof(closedir)    *o_closedir;
#endif

static struct nh_info KoboDotfileHack = {
    .name           = "kobo-dotfile-hack",
    .desc           = "Prevents Nickel from discovering and importing dotfiles/folders.",
    .uninstall_flag = "/mnt/onboard/kdh_uninstall",
};

static struct nh_hook KoboDotfileHackHook[] = {
    {.sym = "readdir",     .sym_new = "w_readdir",     .lib = "libQt5Core.so.5", .out = nh_symoutptr(o_readdir),     .desc = NULL, .optional = true},
    {.sym = "readdir64",   .sym_new = "w_readdir64",   .lib = "libQt5Core.so.5", .out = nh_symoutptr(o_readdir64),   .desc = NULL, .optional = true},
    {.sym = "readdir_r",   .sym_new = "w_readdir_r",   .lib = "libQt5Core.so.5", .out = nh_symoutptr(o_readdir_r),   .desc = NULL, .optional = true},
    {.sym = "readdir64_r", .sym_new = "w_readdir64_r", .lib = "libQt5Core.so.5", .out = nh_symoutptr(o_readdir64_r), .desc = NULL, .optional = true},
#ifndef NO_USE_FULL_PATH
    {.sym = "opendir",     .sym_new = "w_opendir",     .lib = "libQt5Core.so.5", .out = nh_symoutptr(o_opendir),     .desc = NULL, .optional = true},
    {.sym = "fdopendir",   .sym_new = "w_fdopendir",   .lib = "libQt5Core.so.5", .out = nh_symoutptr(o_fdopendir),   .desc = NULL, .optional = true},
    {.sym = "closedir",    .sym_new = "w_closedir",    .lib = "libQt5Core.so.5", .out = nh_symoutptr(o_closedir),    .desc = NULL, .optional = true},
#endif
    {0},
};

NickelHook(
    .init  = NULL,
    .info  = &KoboDotfileHack,
    .hook  = KoboDotfileHackHook,
    .dlsym = NULL,
)

// should_hide returns true for **/.* (but not everything underneath).
static bool should_hide(DIR *dir, const char *name);

#define WRAP_READDIR(dirent, readdir)                 \
extern "C" struct dirent *w_##readdir(DIR *dir) {     \
    struct dirent *res;                               \
    int err;                                          \
    for (;;) {                                        \
        res = o_##readdir(dir);                       \
        err = errno;                                  \
        if (!res || !should_hide(dir, res->d_name)) { \
            errno = err;                              \
            return res;                               \
        }                                             \
    }                                                 \
    errno = 0;                                        \
    return NULL;                                      \
}

WRAP_READDIR(dirent,   readdir)
WRAP_READDIR(dirent64, readdir64)

#define WRAP_READDIR_R(dirent, readdir_r)                                         \
extern "C" int w_##readdir_r(DIR *dir, struct dirent *ent, struct dirent **res) { \
    int ret, err;                                                                 \
    for (;;) {                                                                    \
        ret = o_##readdir_r(dir, ent, res);                                       \
        err = errno;                                                              \
        if (ret || !*res || !should_hide(dir, (*res)->d_name)) {                  \
            errno = err;                                                          \
            return ret;                                                           \
        }                                                                         \
    }                                                                             \
    *res = NULL;                                                                  \
    errno = 0;                                                                    \
    return 0;                                                                     \
}

WRAP_READDIR_R(dirent,   readdir_r)
WRAP_READDIR_R(dirent64, readdir64_r)

#ifdef NO_USE_FULL_PATH

static bool should_hide(DIR *dir __attribute__((unused)), const char *name) {
    if (name[0] != '.')
        return false;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) // show **/. **/..
        return false;
    if (strncasecmp(name, ".kobo", 5) == 0) // show **/.kobo*
        return false;
    if (strcasecmp(name, ".adobe-digital-editions") == 0) // show **/.adobe-digital-editions
        return false;
    // hide **/.* (but not everything underneath)
    nh_log("hid '%s'", name);
    return true;
}

#else

static bool should_hide(DIR *dir, const char *name) {
    int fd;
    char *path;
    if (name[0] != '.')
        return false;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) // show **/. **/..
        return false;
    if ((fd = dirfd(dir)) > 0 && dirpaths[fd] && (path = dirpaths[fd])) {
        if (strncasecmp(path, "/mnt/", 5) != 0) // show !/mnt/**
            return false;
        if (strncasecmp(name, ".kobo", 5) == 0 || strcasestr(path, "/.kobo")) // show **/.kobo*/**
            return false;
        if (strcasecmp(name, ".adobe-digital-editions") == 0 || strcasestr(path, "/.adobe-digital-editions")) // show **/.adobe-digital-editions/**
            return false;
    }
    nh_log("hid '%s'", name);
    return true;
}

extern "C" DIR *w_opendir(const char *name) {
    int err, fd;
    DIR *dir = o_opendir(name);
    err = errno;
    if (dir && (fd = dirfd(dir)) > 0)
        if (!(dirpaths[fd] = realpath(name, NULL)))
            dirpaths[fd] = strdup(name);
    errno = err;
    return dir;
}

extern "C" DIR *w_fdopendir(int fd) {
    // if Kobo uses this (they don't currently), most of the should_hide stuff won't work
    dirpaths[fd] = NULL;
    return o_fdopendir(fd);
}

extern "C" int w_closedir(DIR* dir) {
    int fd;
    if ((fd = dirfd(dir)) > 0 && dirpaths[fd]) {
        free(dirpaths[fd]);
        dirpaths[fd] = NULL;
    }
    return o_closedir(dir);
}

#endif
