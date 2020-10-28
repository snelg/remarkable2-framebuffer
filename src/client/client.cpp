#include "../shared/ipc.cpp"

#include <stdlib.h>

int msg_q_id = 0x2257c;
ipc::Queue MSGQ(msg_q_id);

int WIDTH = 1404;
int HEIGHT = 1872;

class SwtFB {
public:
  uint16_t *fbmem;
  ipc::msg_rect dirty_area;

  SwtFB() {
    fbmem = ipc::get_shared_buffer();
    reset_dirty();
  }

  void reset_dirty() {
    dirty_area.x = WIDTH;
    dirty_area.y = HEIGHT;
    dirty_area.w = -WIDTH;
    dirty_area.h = -HEIGHT;
  }

  void mark_dirty(ipc::msg_rect &&rect) { mark_dirty(rect); }

  void mark_dirty(ipc::msg_rect &rect) {
    int x1 = dirty_area.x + dirty_area.w;
    int y1 = dirty_area.y + dirty_area.h;

    x1 = max(x1, rect.x + rect.w);
    y1 = max(y1, rect.y + rect.h);

    if (x1 > WIDTH) {
      x1 = WIDTH-1;
    }
    if (y1 > HEIGHT) {
      y1 = HEIGHT-1;
    }

    dirty_area.x = min(rect.x, dirty_area.x);
    dirty_area.y = min(rect.y, dirty_area.y);

    if (dirty_area.x < 0) { dirty_area.x = 0; }
    if (dirty_area.y < 0) { dirty_area.y = 0; }

    dirty_area.w = x1 - dirty_area.x;
    dirty_area.h = y1 - dirty_area.y;
  }

  void redraw_screen(bool full_refresh = false) {
    if (full_refresh || dirty_area.w <= 0 || dirty_area.h <= 0) {
      ipc::msg_rect buf = {};
      buf.x = WIDTH;
      buf.y = HEIGHT;
      buf.w = 0;
      buf.h = 0;
      MSGQ.send(buf);
    } else {
      MSGQ.send(dirty_area);
    }
    reset_dirty();
  }
};

int main() {
  srand(time(NULL));
  printf("SENDING MSG UPDATE\n");

  SwtFB fb;

  for (int i = 0; i < WIDTH*HEIGHT; i++) {
    fb.fbmem[i] = i;
  }

  int x = (rand() % WIDTH);
  int y = (rand() % HEIGHT);
  if (x > WIDTH) { x -= WIDTH; };
  if (y > HEIGHT) { y -= HEIGHT; };
  int w = 200 + (rand() % 10+1) * 50;
  int h = 200 + (rand() % 10+1) * 50;

  cout << x << " " << y << " " << w << " " << h << endl;
  fb.mark_dirty({x, y, w, h});
  fb.redraw_screen();
}
