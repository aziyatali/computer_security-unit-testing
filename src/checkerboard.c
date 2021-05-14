#include "pngparser.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  struct pixel palette[2];
  struct image *img = NULL;

  if (argc != 7) {
    goto error;
  }

  /* Name the arguments */

  const char *output_name = argv[1];
  const char *height_arg = argv[2];
  const char *width_arg = argv[3];
  const char *square_width_arg = argv[4];
  const char *hex_color_arg1 = argv[5];
  const char *hex_color_arg2 = argv[6];

  char *end_ptr;

  /* Parse the arguments */

  if (strlen(hex_color_arg1) != 6) {
    goto error;
  }

  if (strlen(hex_color_arg2) != 6) {
    goto error;
  }

  long height = strtol(height_arg, &end_ptr, 10);

  if (height < 0 || *end_ptr) {
    goto error;
  }

  long width = strtol(width_arg, &end_ptr, 10);

  if (width < 0 || *end_ptr) {
    goto error;
  }

  long color1 = strtol(hex_color_arg1, &end_ptr, 16);

  if (*end_ptr) {
    goto error;
  }

  long color2 = strtol(hex_color_arg2, &end_ptr, 16);

  if (*end_ptr) {
    goto error;
  }

  long square_width = strtol(square_width_arg, &end_ptr, 10);

  if (square_width <= 0 || *end_ptr) {
    goto error;
  }

  /* We assign colors to the palette */
  palette[0].red = (color1 & 0xff0000) >> 16;
  palette[0].green = (color1 & 0x00ff00) >> 8;
  palette[0].blue = (color1 & 0x0000ff);
  palette[0].alpha = 0xff;

  palette[1].red = (color2 & 0xff0000) >> 16;
  palette[1].green = (color2 & 0x00ff00) >> 8;
  palette[1].blue = (color2 & 0x0000ff);
  palette[1].alpha = 0xff;

  /* Memory allocation and error handling */
  img = malloc(sizeof(struct image));
  if (!img) {
    goto error_mem;
  }

  img->px = malloc(sizeof(struct pixel) * height * width);
  if (!img->px) {
    goto error_img;
  }

  img->size_x = width;
  img->size_y = height;

  {
    struct pixel(*image_data)[width] = (struct pixel(*)[width])img->px;

    for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
        int square_i = i / square_width;
        int square_j = j / square_width;

        int color = (square_i + square_j) % 2;

        image_data[i][j].red = palette[color].red;
        image_data[i][j].green = palette[color].green;
        image_data[i][j].blue = palette[color].blue;
        image_data[i][j].alpha = 0xff;
      }
    }
  }

  store_png(output_name, img, palette, 2);

  free(img->px);
  free(img);

  return 0;

/* Error handling code */
error:
  printf(
      "Usage: %s output_name height width square_width hex_color1 hex_color2\n",
      argv[0]);
  return 1;
error_px:
  free(img->px);
error_img:
  free(img);
error_mem:
  printf("Couldn't allocate memory\n");
  return 1;
}