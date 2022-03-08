# EE6470 HW1

## Gaussian blur with FIFO

### How to run

```bash
$ cd part1
$ mkdir build && cd build
$ cmake .. && make run
```

### How it work
I use 6 FIFO to transmit pixel.
```c
sc_fifo<unsigned char> r;
sc_fifo<unsigned char> g;
sc_fifo<unsigned char> b;
sc_fifo<int> result_r;
sc_fifo<int> result_g;
sc_fifo<int> result_b;
```

In `Testbench::do_sobel` transmit 9 pixel one time for convolution and then get one pixel for result.
```c
void Testbench::do_sobel() {
  int x, y, v, u;        // for loop counter
  unsigned char R, G, B; // color of R, G, B
  int adjustX, adjustY, xBound, yBound;
  int result_r, result_g, result_b;

  o_rst.write(false);
  o_rst.write(true);
  for (y = 0; y != height; ++y) {
    for (x = 0; x != width; ++x) {
      adjustX = (MASK_X % 2) ? 1 : 0; // 1
      adjustY = (MASK_Y % 2) ? 1 : 0; // 1
      xBound = MASK_X / 2;            // 1
      yBound = MASK_Y / 2;            // 1

      for (v = -yBound; v != yBound + adjustY; ++v) {   //-1, 0, 1
        for (u = -xBound; u != xBound + adjustX; ++u) { //-1, 0, 1
          if (x + u >= 0 && x + u < width && y + v >= 0 && y + v < height) {
            R = *(source_bitmap +
                  bytes_per_pixel * (width * (y + v) + (x + u)) + 2);
            G = *(source_bitmap +
                  bytes_per_pixel * (width * (y + v) + (x + u)) + 1);
            B = *(source_bitmap +
                  bytes_per_pixel * (width * (y + v) + (x + u)) + 0);
          } else {
            R = 0;
            G = 0;
            B = 0;
          }
          o_r.write(R);
          o_g.write(G);
          o_b.write(B);
          wait(1); //emulate channel delay
        }
      }

      if(i_result_r.num_available()==0) 
          wait(i_result_r.data_written_event());
      result_r = i_result_r.read();

      if(i_result_g.num_available()==0) 
          wait(i_result_g.data_written_event());
      result_g = i_result_g.read();

      if(i_result_b.num_available()==0) 
          wait(i_result_b.data_written_event());
      result_b = i_result_b.read();
      //cout << "Now at " << sc_time_stamp() << endl; //print current sc_time

      // write data
      *(target_bitmap + bytes_per_pixel * (width * y + x) + 2) = result_r;
      *(target_bitmap + bytes_per_pixel * (width * y + x) + 1) = result_g;
      *(target_bitmap + bytes_per_pixel * (width * y + x) + 0) = result_b;
    }
  }
  sc_stop();
}
```

`SobelFilter::do_filter` use the received pixel do convolution and transmit the result.
```c
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
```

## Gaussian blur with row buffers

### How to run

```bash
$ cd part2
$ mkdir build && cd build
$ cmake .. && make run
```

### How it work

Use 6 FIFO to transmit pixel like above.

```c
  int count = 3;
  for (y = -1; y <= height; y++) {
    for (x = -1; x <= width; x++) {
      if (y == -1 || y == height || x == -1 || x == width) {
        R = 0;
        G = 0;
        B = 0;
      } else {
        R = *(source_bitmap + bytes_per_pixel * (width * y + x) + 2);
        G = *(source_bitmap + bytes_per_pixel * (width * y + x) + 1);
        B = *(source_bitmap + bytes_per_pixel * (width * y + x) + 0);
      }
      o_r.write(R);
      o_g.write(G);
      o_b.write(B);
      wait(1);
    }
    count--;

    if (count == 0) {
      int r_x;
      int r_y = y - 1;
      count++;
      for (r_x = 0; r_x < width; r_x++) {
        if(i_result_r.num_available()==0) 
            wait(i_result_r.data_written_event());
        result_r = i_result_r.read();

        if(i_result_g.num_available()==0) 
            wait(i_result_g.data_written_event());
        result_g = i_result_g.read();

        if(i_result_b.num_available()==0) 
            wait(i_result_b.data_written_event());
        result_b = i_result_b.read();

        //cout << "Now at " << sc_time_stamp() << endl; //print current sc_time

        // write data
        *(target_bitmap + bytes_per_pixel * (width * r_y + r_x) + 2) = result_r;
        *(target_bitmap + bytes_per_pixel * (width * r_y + r_x) + 1) = result_g;
        *(target_bitmap + bytes_per_pixel * (width * r_y + r_x) + 0) = result_b;
      }
      
    }
  }
```
In `do_sobel` it write pixels to FIFO and then read 1 row of result pixels. Only write 3 rows of pixels at begin, the other time only write 1 rows of pixels to FIFO.


```c
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
```
`pixels[3][256 + 2][3]` is a buffer for 3 rows of pixels. Each row has 256 pixel and the additional 2 pixel is used for padding. Before doing convolution it read necessary pixels from FIFO to `pixels[3][256+2][3]` buffer, so at begin it read 3 row of pixels in other time it read 1 rows.
```
0 : padding for one pixel (r=0, g=0, b=0)
This is the data being input to the FIFO.
+--------------------+
| 0 0 0 0 0 0 0 0 0 0|
|0+----------------+0|
|0|                |0|
|0|                |0|
|0|Orignal Image's |0|
|0|     pixels     |0|
|0|                |0|
|0|                |0|
|0+----------------+0|
| 0 0 0 0 0 0 0 0 0 0|
+--------------------+
```

`next_row ` is used to decide which row of buffer would be replace by new row of pixels.
| Iteration                       | 1   | 2   | 3   | 4   | ... | 256 |
| ------------------------------- | --- | --- | --- | --- | --- | --- |
| **# row write in to FIFO**      | 3   | 1   | 1   | 1   | ... | 1   |
| **# row read from result FIFO** | 1   | 1   | 1   | 1   | ... | 1   |