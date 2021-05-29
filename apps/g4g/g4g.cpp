#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include "../syscall.h"

const uint64_t kG4GDataOffset = 16;

struct G4GHeader {
  uint32_t w, h;
  uint64_t frames;
  uint64_t NumBytes() const;
  uint64_t FrameDataOffset(int f) const;
};

uint64_t G4GHeader::NumBytes() const {
  return kG4GDataOffset + (w * h / 4) * frames;
}

uint64_t G4GHeader::FrameDataOffset(int f) const {
  return kG4GDataOffset + (w * h / 4) * f;
}

uint32_t g4g_palette[4] = {
  0x000000, // 0: black
  0x666666, // 1: dark gray
  0xbbbbbb, // 2: light gray
  0xffffff, // 3: white
};

uint32_t ColorAt(const G4GHeader& g4g_hdr, uint8_t* g4g_data, int x, int y) {
  auto pix = x + y * g4g_hdr.w;
  auto colnum = (g4g_data[pix / 4] >> ((3 - pix % 4) * 2)) & 0b11;
  return g4g_palette[colnum];
}

void WaitQuit() {
  AppEvent events[1];
  while (true) {
    auto [n, err] = SyscallReadEvent(events, 1);
    if (err) {
      fprintf(stderr, "ReadEvent failed: %s\n", strerror(err));
      return;
    }
    if (events[0].type == AppEvent::kQuit) {
      return;
    }
  }
}

void ClearWindow(uint64_t layer_id_flags, int w, int h) {
  SyscallWinFillRectangle(layer_id_flags, 4, 24, w, h, 0xffffff);
}

void DrawFrame(uint64_t layer_id, const G4GHeader& g4g_hdr, uint64_t g4g_addr, int frame_id, int scale) {
  ClearWindow(layer_id | LAYER_NO_REDRAW, g4g_hdr.w*scale, g4g_hdr.h*scale);

  uint8_t* g4g_data = reinterpret_cast<uint8_t*>(g4g_addr + g4g_hdr.FrameDataOffset(frame_id));
  for (int dy = 0; dy < g4g_hdr.h; ++dy) {
    for (int dx = 0; dx < g4g_hdr.w; ++dx) {
      SyscallWinFillRectangle(layer_id | LAYER_NO_REDRAW, 
                              4 + dx*scale, 24 + dy*scale, scale, scale, 
                              ColorAt(g4g_hdr, g4g_data, dx, dy));
    }
  }
  SyscallWinRedraw(layer_id);
}

void DrawLoop(uint64_t layer_id, const G4GHeader& g4g_hdr, uint64_t g4g_addr, int scale, unsigned long ms_per_frame) {
  auto create_timer = [ms_per_frame]() {
    SyscallCreateTimer(TIMER_ONESHOT_REL, 1, ms_per_frame);
  };
  int curr_frame_id = 1;

  create_timer();

  AppEvent events[1];
  while (true) {
    auto res = SyscallReadEvent(events, 1);
    if (res.error) {
      fprintf(stderr, "failed to read event: %s\n", strerror(res.error));
      break;
    }
    switch (events[0].type) {
    case AppEvent::kTimerTimeout:
      create_timer();
      DrawFrame(layer_id, g4g_hdr, g4g_addr, curr_frame_id, scale);
      curr_frame_id = (curr_frame_id + 1) % g4g_hdr.frames;
      break;
    case AppEvent::kQuit:
      return;
    }
  }
}

extern "C" void main (int argc, char** argv) {
  auto print_help = [argv]() {
    fprintf(stderr,
            "Usage: %s [-s SCALE (default: 1)] [-f MSEC_PER_FRAME (default: 250)] <file>\n",
            argv[0]);
  };

  // parse options
  int opt;
  int scale = 1, ms_per_frame = 250;
  while ((opt = getopt(argc, argv, "s:f:")) != -1) {
    switch (opt) {
    case 's': scale = atoi(optarg);        break;
    case 'f': ms_per_frame = atoi(optarg); break;
    default:
      print_help();
      exit(1);
    }
  }
  if (optind >= argc) {
    print_help();
    exit(1);
  }
  bool invalid_args = false;
  if (scale <= 0) {
    fprintf(stderr, "-s (scale) must be an positive integer\n");
    invalid_args = true;
  }
  if (ms_per_frame <= 0) {
    fprintf(stderr, "-f (msec per frame) must be an positive integer\n");
    invalid_args = true;
  }
  if (invalid_args) {
    exit(1);
  }

  // read image file
  auto filename = argv[optind];
  auto [fd, err] = SyscallOpenFile(filename, O_RDONLY);
  if (err) {
    fprintf(stderr, "%s: %s\n", strerror(err), filename);
    exit(1);
  }
  size_t filesize;
  auto [g4g_addr, err2] = SyscallMapFile(fd, &filesize, 0);
  if (err2) {
    fprintf(stderr, "%s\n", strerror(err2));
    exit(1);
  }

  // read image info
  auto g4g_hdr = reinterpret_cast<G4GHeader*>(g4g_addr);

  // open window
  auto [layer_id, err3] = SyscallOpenWindow(8 + g4g_hdr->w * scale, 28 + g4g_hdr->h * scale, 10, 10, "g4g");
  if (err3) {
    fprintf(stderr, "%s\n", strerror(err3));
    exit(1);
  }

  // draw first frame
  DrawFrame(layer_id, *g4g_hdr, g4g_addr, 0, scale);

  // start animation if there are multi frames
  // quit when kQuit event arrives
  if (g4g_hdr->frames > 1) {
    DrawLoop(layer_id, *g4g_hdr, g4g_addr, scale, ms_per_frame);
  } else {
    WaitQuit();
  }

  SyscallCloseWindow(layer_id);
  exit(0);
}
