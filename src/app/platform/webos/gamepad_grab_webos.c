#include "gamepad_grab_webos.h"

#include <SDL.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int xbox_fd = -1;
static Uint32 next_scan;

void webos_gamepad_grab_update(bool streaming) {
    if (!streaming) {
        if (xbox_fd >= 0) {
            ioctl(xbox_fd, EVIOCGRAB, 0);
            close(xbox_fd);
            xbox_fd = -1;
        }
        next_scan = 0;
        return;
    }

    Uint32 now = SDL_GetTicks();
    if (next_scan && !SDL_TICKS_PASSED(now, next_scan)) {
        return;
    }
    next_scan = now + 3000;

    if (xbox_fd >= 0) {
        struct input_id id;
        if (ioctl(xbox_fd, EVIOCGID, &id) == 0) {
            return;
        }
        close(xbox_fd);
        xbox_fd = -1;
    }

    for (int i = 0; i < 64; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) {
            continue;
        }
        struct input_id id;
        if (ioctl(fd, EVIOCGID, &id) == 0 &&
            id.vendor == 0x045e && id.product == 0x0b22 &&
            ioctl(fd, EVIOCGRAB, 1) == 0) {
            xbox_fd = fd;
            SDL_LogInfo(SDL_LOG_CATEGORY_INPUT, "Grabbed Xbox Elite Series 2 at %s", path);
            return;
        }
        close(fd);
    }
}
