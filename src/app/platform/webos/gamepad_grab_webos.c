#include "gamepad_grab_webos.h"

#include <SDL.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_EVENT_DEVICES 64

static int xbox_fds[MAX_EVENT_DEVICES];
static bool initialized;
static Uint32 next_scan;

void webos_gamepad_grab_update(bool streaming) {
    if (!initialized) {
        for (int i = 0; i < MAX_EVENT_DEVICES; ++i) {
            xbox_fds[i] = -1;
        }
        initialized = true;
    }

    if (!streaming) {
        for (int i = 0; i < MAX_EVENT_DEVICES; ++i) {
            if (xbox_fds[i] >= 0) {
                ioctl(xbox_fds[i], EVIOCGRAB, 0);
                close(xbox_fds[i]);
                xbox_fds[i] = -1;
            }
        }
        next_scan = 0;
        return;
    }

    Uint32 now = SDL_GetTicks();
    if (next_scan && !SDL_TICKS_PASSED(now, next_scan)) {
        return;
    }
    next_scan = now + 3000;

    for (int i = 0; i < MAX_EVENT_DEVICES; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        if (xbox_fds[i] >= 0) {
            struct stat path_stat, fd_stat;
            if (stat(path, &path_stat) == 0 &&
                fstat(xbox_fds[i], &fd_stat) == 0 &&
                path_stat.st_dev == fd_stat.st_dev &&
                path_stat.st_ino == fd_stat.st_ino) {
                continue;
            }
            ioctl(xbox_fds[i], EVIOCGRAB, 0);
            close(xbox_fds[i]);
            xbox_fds[i] = -1;
        }

        int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) {
            continue;
        }
        struct input_id id;
        if (ioctl(fd, EVIOCGID, &id) == 0 &&
            id.vendor == 0x045e && id.product == 0x0b22 &&
            ioctl(fd, EVIOCGRAB, 1) == 0) {
            xbox_fds[i] = fd;
            SDL_LogInfo(SDL_LOG_CATEGORY_INPUT, "Grabbed Xbox Elite Series 2 at %s", path);
            continue;
        }
        close(fd);
    }
}
