#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <linux/input.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

struct keyboard {
    int fd;
    bool left_ctrl;
    bool right_ctrl;
    bool left_alt;
    bool right_alt;
};

static bool has_bit(const unsigned long *bits, unsigned int bit)
{
    const unsigned int word_bits = sizeof(*bits) * 8;
    return (bits[bit / word_bits] & (1UL << (bit % word_bits))) != 0;
}

static bool is_keyboard(int fd)
{
    unsigned long event_bits[(EV_MAX / (sizeof(unsigned long) * 8)) + 1] = {0};
    unsigned long key_bits[(KEY_MAX / (sizeof(unsigned long) * 8)) + 1] = {0};

    if (ioctl(fd, EVIOCGBIT(0, sizeof(event_bits)), event_bits) < 0 ||
        !has_bit(event_bits, EV_KEY) ||
        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
        return false;
    }

    return has_bit(key_bits, KEY_LEFTCTRL) &&
           has_bit(key_bits, KEY_RIGHTCTRL) &&
           has_bit(key_bits, KEY_LEFTALT) &&
           has_bit(key_bits, KEY_RIGHTALT) &&
           (has_bit(key_bits, KEY_7) || has_bit(key_bits, KEY_KP7));
}

static void close_keyboards(struct keyboard *keyboards, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (keyboards[i].fd >= 0) {
            close(keyboards[i].fd);
        }
    }
}

int main(int argc, char **argv)
{
    int timeout_seconds = 5;
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("Usage: nonoos-recovery-hotkey-listen [--timeout-seconds 1..30]");
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--timeout-seconds") == 0) {
        char *end = NULL;
        long parsed = strtol(argv[2], &end, 10);
        if (end == argv[2] || *end != '\0' || parsed < 1 || parsed > 30) {
            fprintf(stderr, "Le délai doit être compris entre 1 et 30 secondes.\n");
            return 2;
        }
        timeout_seconds = (int)parsed;
    } else if (argc != 1) {
        fprintf(stderr, "Usage: nonoos-recovery-hotkey-listen [--timeout-seconds 1..30]\n");
        return 2;
    }

    glob_t paths = {0};
    int glob_result = glob("/dev/input/event*", 0, NULL, &paths);
    if (glob_result != 0 && glob_result != GLOB_NOMATCH) {
        fprintf(stderr, "Impossible de rechercher les périphériques clavier.\n");
        globfree(&paths);
        return 2;
    }

    struct keyboard *keyboards = calloc(paths.gl_pathc, sizeof(*keyboards));
    struct pollfd *poll_fds = calloc(paths.gl_pathc, sizeof(*poll_fds));
    if (paths.gl_pathc > 0 && (keyboards == NULL || poll_fds == NULL)) {
        fprintf(stderr, "Mémoire insuffisante pour surveiller les claviers.\n");
        free(keyboards);
        free(poll_fds);
        globfree(&paths);
        return 2;
    }

    size_t count = 0;
    for (size_t i = 0; i < paths.gl_pathc; ++i) {
        int fd = open(paths.gl_pathv[i], O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) {
            continue;
        }
        if (!is_keyboard(fd)) {
            close(fd);
            continue;
        }
        keyboards[count].fd = fd;
        poll_fds[count].fd = fd;
        poll_fds[count].events = POLLIN;
        ++count;
    }
    globfree(&paths);

    if (count == 0) {
        fprintf(stderr, "Aucun clavier compatible n'a été trouvé dans /dev/input.\n");
        free(keyboards);
        free(poll_fds);
        return 2;
    }

    struct timespec deadline;
    if (clock_gettime(CLOCK_MONOTONIC, &deadline) < 0) {
        perror("clock_gettime");
        close_keyboards(keyboards, count);
        free(keyboards);
        free(poll_fds);
        return 2;
    }
    deadline.tv_sec += timeout_seconds;

    int result = 1;
    while (true) {
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
            perror("clock_gettime");
            result = 2;
            break;
        }
        long long remaining_ns =
            (long long)(deadline.tv_sec - now.tv_sec) * 1000000000LL +
            deadline.tv_nsec - now.tv_nsec;
        if (remaining_ns <= 0) {
            break;
        }
        int remaining_ms = (int)((remaining_ns + 999999LL) / 1000000LL);
        int ready = poll(poll_fds, count, remaining_ms);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll");
            result = 2;
            break;
        }
        if (ready == 0) {
            break;
        }

        for (size_t i = 0; i < count; ++i) {
            if ((poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
                close(keyboards[i].fd);
                keyboards[i].fd = -1;
                poll_fds[i].fd = -1;
                continue;
            }
            if ((poll_fds[i].revents & POLLIN) == 0) {
                continue;
            }

            struct input_event event;
            ssize_t bytes;
            while ((bytes = read(keyboards[i].fd, &event, sizeof(event))) ==
                   (ssize_t)sizeof(event)) {
                if (event.type == EV_SYN && event.code == SYN_DROPPED) {
                    keyboards[i].left_ctrl = false;
                    keyboards[i].right_ctrl = false;
                    keyboards[i].left_alt = false;
                    keyboards[i].right_alt = false;
                    continue;
                }
                if (event.type != EV_KEY) {
                    continue;
                }

                bool pressed = event.value != 0;
                switch (event.code) {
                case KEY_LEFTCTRL:
                    keyboards[i].left_ctrl = pressed;
                    break;
                case KEY_RIGHTCTRL:
                    keyboards[i].right_ctrl = pressed;
                    break;
                case KEY_LEFTALT:
                    keyboards[i].left_alt = pressed;
                    break;
                case KEY_RIGHTALT:
                    keyboards[i].right_alt = pressed;
                    break;
                case KEY_7:
                case KEY_KP7:
                    if (event.value == 1 &&
                        (keyboards[i].left_ctrl || keyboards[i].right_ctrl) &&
                        (keyboards[i].left_alt || keyboards[i].right_alt)) {
                        result = 0;
                    }
                    break;
                default:
                    break;
                }
                if (result == 0) {
                    goto finished;
                }
            }
            if (bytes < 0 && errno != EAGAIN && errno != EINTR) {
                perror("read");
                result = 2;
                goto finished;
            }
        }
    }

finished:
    close_keyboards(keyboards, count);
    free(keyboards);
    free(poll_fds);
    return result;
}
