#include <cmath>

#include "SobelFilter.h"

SobelFilter::SobelFilter(sc_module_name n) : sc_module(n) {
  SC_THREAD(do_filter);
  sensitive << i_clk.pos();
  dont_initialize();
  reset_signal_is(i_rst, false);
}

// sobel mask
const int mask[MASK_N][MASK_X][MASK_Y] = {{{1, 2, 1}, {2, 4, 2}, {1, 2, 1}}};

void SobelFilter::do_filter() {
  int pixels[3][256 + 2][3];    // pixels[row][width + 2][rgb]
  unsigned int i = 0;
  int count = 3;
  int next_row = 0;
  while (true) {
    for (int x = -1; x < 256 + 1; x ++) {
        if (i_r.num_available() == 0)
            wait(i_r.data_written_event());
        pixels[next_row][x + 1][0] = i_r.read();

        if (i_g.num_available() == 0)
            wait(i_g.data_written_event());
        pixels[next_row][x + 1][1] = i_g.read();

        if (i_b.num_available() == 0)
            wait(i_b.data_written_event());
        pixels[next_row][x + 1][2] = i_b.read();
    }
    next_row = (next_row + 1) % 3;
    count--;

    if (count == 0) {
      int c_x;
      count++;

      for (c_x = 0; c_x < 256; c_x++) {
        int pixel_row = next_row;
        val_r = val_g = val_b = 0;

        for (unsigned int v = 0; v < MASK_Y; v++) {
          for (unsigned int u = 0; u < MASK_X; u++) {
            val_r += mask[0][v][u] * pixels[pixel_row][c_x + u][0];
            val_g += mask[0][v][u] * pixels[pixel_row][c_x + u][1];
            val_b += mask[0][v][u] * pixels[pixel_row][c_x + u][2];
          }
          pixel_row = (pixel_row + 1) % 3;
        }

        val_r = std::min(abs(int(factor * val_r)), 255);
        val_g = std::min(abs(int(factor * val_g)), 255);
        val_b = std::min(abs(int(factor * val_b)), 255);
        o_result_r.write(val_r);
        o_result_g.write(val_g);
        o_result_b.write(val_b);
        wait(10); //emulate module delay
      }
    }
  }
}
