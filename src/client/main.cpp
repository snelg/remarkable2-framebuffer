#include <dlfcn.h>
#include <iostream>
#include <libgen.h>
#include <linux/fb.h>
#include <linux/ioctl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

#include "../shared/ipc.cpp"

#define FB_ID "mxcfb"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

using namespace std;
int msg_q_id = 0x2257c;
swtfb::ipc::Queue MSGQ(msg_q_id);

uint16_t *SHARED_BUF;

extern "C" {
struct stat;
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() {
  std::ios_base::Init i;

  SHARED_BUF = swtfb::ipc::get_shared_buffer();
}

int open64(const char *pathname, int flags, mode_t mode = 0) {
  static int (*func_open)(const char *, int, mode_t) = NULL;

  if (!func_open) {
    func_open = (int (*)(const char *, int, mode_t))dlsym(RTLD_NEXT, "open64");
  }

  auto r = func_open(pathname, flags, mode);
  if (pathname == string("/dev/fb0")) {
    std::cerr << "SETTING FB FD" << ' ' << r << ' ' << swtfb::ipc::SWTFB_FD
              << std::endl;
    return swtfb::ipc::SWTFB_FD;
  }

  return r;
}

int open(const char *pathname, int flags, mode_t mode = 0) {
  static int (*func_open)(const char *, int, mode_t) = NULL;

  if (!func_open) {
    func_open = (int (*)(const char *, int, mode_t))dlsym(RTLD_NEXT, "open");
  }

  auto r = func_open(pathname, flags, mode);
  if (pathname == string("/dev/fb0")) {
    std::cerr << "SETTING FB FD" << ' ' << r << ' ' << swtfb::ipc::SWTFB_FD
              << std::endl;
    return swtfb::ipc::SWTFB_FD;
  }

  return r;
}

int close(int fd) {
  static int (*func_close)(int) = NULL;

  if (!func_close) {
    func_close = (int (*)(int))dlsym(RTLD_NEXT, "close");
  }

  return func_close(fd);
}

int ioctl(int fd, unsigned long request, char *ptr) {
  static int (*func_ioctl)(int, unsigned long, ...) = NULL;

  if (fd == swtfb::ipc::SWTFB_FD) {
    if (request == MXCFB_SEND_UPDATE) {

      mxcfb_update_data *update = (mxcfb_update_data *)ptr;
      MSGQ.send(*update);
      return 0;
    } else if (request == MXCFB_SET_AUTO_UPDATE_MODE) {

      return 0;
    } else if (request == MXCFB_WAIT_FOR_UPDATE_COMPLETE) {

      return 0;
    }

    else if (request == FBIOGET_VSCREENINFO) {

      fb_var_screeninfo *screeninfo = (fb_var_screeninfo *)ptr;
      screeninfo->xres = 1404;
      screeninfo->yres = 1872;
      screeninfo->grayscale = 0;
      screeninfo->bits_per_pixel = 16;
      return 0;
    }

    else if (request == FBIOPUT_VSCREENINFO) {

      return 0;
    } else if (request == FBIOGET_FSCREENINFO) {

      fb_fix_screeninfo *screeninfo = (fb_fix_screeninfo *)ptr;
      screeninfo->smem_len = swtfb::ipc::BUF_SIZE;
      screeninfo->smem_start = (unsigned long)SHARED_BUF;
      screeninfo->line_length = swtfb::WIDTH * sizeof(uint16_t);
      memcpy(screeninfo->id, FB_ID, sizeof(FB_ID));
      screeninfo->id[sizeof(FB_ID)] = 0;
      return 0;
    } else {
      std::cerr << "UNHANDLED IOCTL" << ' ' << request << std::endl;
      return 0;
    }
  }

  if (!func_ioctl) {
    func_ioctl =
        (int (*)(int, unsigned long request, ...))dlsym(RTLD_NEXT, "ioctl");
  }

  return func_ioctl(fd, request, ptr);
}
};

int main() { (void)0; };
