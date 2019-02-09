#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define CLAMP(val, min, max) \
    ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

/* all fields are little endian */
static unsigned char bmp_header[] = {
  'B', 'M',
  0x36, 0, 3, 0,  /* total size */
  0, 0,           /* reserved */
  0, 0,           /* reserved */
  54, 0, 0, 0,    /* offset */
  40, 0, 0, 0,    /* header size */

  0, 1, 0, 0, /* width */
  0, 1, 0, 0, /* height */
  1, 0,       /* nbr planes */
  24, 0,      /* bpp */
  0, 0, 0, 0, /* compression */
  0, 0, 0, 0, /* bmp_data size */
  0, 0, 0, 0, /* X ppm */
  0, 0, 0, 0, /* Y ppm */
  0, 0, 0, 0, /* nbr colors */
  0, 0, 0, 0, /* nbr important  colors */
};

static const float edge[9] = {
  -1.0, -1.0, -1.0,
  -1.0,  8.0, -1.0,
  -1.0, -1.0, -1.0,
};

static const float sharpen[9] = {
    0.0, -1.0,  0.0,
   -1.0,  5.0, -1.0,
    0.0, -1.0,  0.0,
};

static const float box_blur[9] = {
  1/9.0, 1/9.0, 1/9.0,
  1/9.0, 1/9.0, 1/9.0,
  1/9.0, 1/9.0, 1/9.0,
};

static const float gaussian_blur[9] = {
  1/16.0, 2/16.0, 1/16.0,
  2/16.0, 4/16.0, 2/16.0,
  1/16.0, 2/16.0, 1/16.0,
};

/* bitmap data buffers */
static unsigned char bmp_data[256 * 256 * 3];
static unsigned char bmp_data2[256 * 256 * 3];

static void write_bmp(unsigned char *data, int sym) {
  FILE *fp;
  char namebuf[16];

  /* open output file */
  snprintf(namebuf, sizeof(namebuf), "hello%c.bmp", sym);
  fp = fopen(namebuf, "wb");
  if (fp == NULL) {
    perror(namebuf);
    exit(EXIT_FAILURE);
  }

  /* write bitmap header and data */
  fwrite(bmp_header, sizeof(bmp_header), 1, fp);
  fwrite(data, 256 * 256 * 3, 1, fp);
  fclose(fp);
}

static void filter3x3(unsigned char *dst, const unsigned char *src,
    const float *kernel) {
  int x;
  int y;
  int kernel_x;
  int kernel_y;
  int adj_x;
  int adj_y;
  int acc_r;
  int acc_g;
  int acc_b;

  float mul;

  for (y = 0; y < 256; y++) {
    for (x = 0; x < 256; x++) {
      acc_r = acc_g = acc_b = 0;
      for (kernel_y = -1; kernel_y <= 1; kernel_y++) {
        for (kernel_x = -1; kernel_x <= 1; kernel_x++) {
          adj_y = CLAMP(y + kernel_y, 0, 255);
          adj_x = CLAMP(x + kernel_x, 0, 255);

          mul = kernel[(kernel_y + 1) * 3 + kernel_x + 1];
          acc_b += (float)src[(255 - adj_y) * 256 * 3 + adj_x * 3] * mul;
          acc_g += (float)src[(255 - adj_y) * 256 * 3 + adj_x * 3 + 1] * mul;
          acc_r += (float)src[(255 - adj_y) * 256 * 3 + adj_x * 3 + 2] * mul;
        }
      }

      dst[(255 - y) * 256 * 3 + x * 3]      = CLAMP(acc_b, 0, 255);
      dst[(255 - y) * 256 * 3 + x * 3 + 1]  = CLAMP(acc_g, 0, 255);
      dst[(255 - y) * 256 * 3 + x * 3 + 2]  = CLAMP(acc_r, 0, 255);
    }
  }
}

int main() {
  FILE *fp;
  int x;
  int y;

  /* generate the original image, backwards because BMP (though neither of
   * the calculations really care) */
  for (y = 0; y < 256; y++) {
    for (x = 0; x < 256; x++) {
      bmp_data[(255 - y) * 256 * 3 + x * 3]     = (-x*x + 2*y + y) & 0xff;
      bmp_data[(255 - y) * 256 * 3 + x * 3 + 1] = x^y;
      bmp_data[(255 - y) * 256 * 3 + x * 3 + 2] = x^y;
    }
  }
  write_bmp(bmp_data, '0');

  /* apply kernels */
  filter3x3(bmp_data2, bmp_data, gaussian_blur);
  write_bmp(bmp_data2, '1');
  filter3x3(bmp_data, bmp_data2, edge);
  write_bmp(bmp_data, '2');
  filter3x3(bmp_data2, bmp_data, sharpen);
  write_bmp(bmp_data2, '3');
  filter3x3(bmp_data, bmp_data2, box_blur);
  write_bmp(bmp_data, '4');


  return EXIT_SUCCESS;
}
