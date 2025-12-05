/*
 * Stubs newlib bare-metal, aucune I/O POSIX, aucune heap, conforme à la politique no-malloc.
 */

#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

__attribute__((weak)) int _write(int file, const char *ptr, int len) {
    (void)file;
    (void)ptr;
    (void)len;
    errno = ENOSYS;
    return -1;
}

__attribute__((weak)) int _read(int file, char *ptr, int len) {
    (void)file;
    (void)ptr;
    (void)len;
    errno = ENOSYS;
    return -1;
}

__attribute__((weak)) int _close(int file) {
    (void)file;
    errno = ENOSYS;
    return -1;
}

__attribute__((weak)) int _lseek(int file, int ptr, int dir) {
    (void)file;
    (void)ptr;
    (void)dir;
    errno = ENOSYS;
    return -1;
}

__attribute__((weak)) int _fstat(int file, struct stat *st) {
    (void)file;
    if (st != NULL) {
        st->st_mode = S_IFCHR;
    }
    return 0;
}

__attribute__((weak)) int _isatty(int file) {
    (void)file;
    return 1;
}

__attribute__((weak)) int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    errno = ENOSYS;
    return -1;
}

__attribute__((weak)) int _getpid(void) {
    return 1;
}

__attribute__((weak)) void _exit(int status) {
    (void)status;
    while (1) {
    }
}

__attribute__((weak)) caddr_t _sbrk(int incr) {
    (void)incr;
    errno = ENOMEM;
    return (caddr_t)-1;
}

