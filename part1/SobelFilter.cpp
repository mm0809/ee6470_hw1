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
  while (true) {
    unsigned int i = 0;
    val_r = val_g = val_b = 0;
    for (unsigned int v = 0; v < MASK_Y; ++v) {
      for (unsigned int u = 0; u < MASK_X; ++u) {
        val_r += i_r.read() * mask[i][u][v];
        val_g += i_g.read() * mask[i][u][v];
        val_b += i_b.read() * mask[i][u][v];
        //unsigned char grey = (i_r.read() + i_g.read() + i_b.read()) / 3;
        //for (unsigned int i = 0; i != MASK_N; ++i) {
        //  val[i] += grey * mask[i][u][v];
        //}
      }
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
