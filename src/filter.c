#include "pngparser.h"
#include <math.h>
#include <stdio.h>
#include <float.h>

/* This filter iterates over the image and calculates the average value of the
 * color channels for every pixel This value is then written to all the channels
 * to get the grayscale representation of the image
 */
void filter_grayscale(struct image *img, void *weight_arr) {
  struct pixel(*image_data)[img->size_x] =
      (struct pixel(*)[img->size_x])img->px;
  double *weights = (double *)weight_arr;

  for (unsigned short i = 0; i < img->size_y; i++) {
    for (unsigned short j = 0; j < img->size_x; j++) {
      double luminosity = 0;

      luminosity += weights[0] * image_data[i][j].red;
      luminosity += weights[1] * image_data[i][j].green;
      luminosity += weights[2] * image_data[i][j].blue;

      image_data[i][j].red = (uint8_t)luminosity;
      image_data[i][j].green = (uint8_t)luminosity;
      image_data[i][j].blue = (uint8_t)luminosity;
    }
  }
}

/* This filter blurs an image. The larger the radius, the more noticeable the
 * blur.
 *
 * For every pixel we define a square of side 2*radius+1 centered around it.
 * The new value of the pixel is the average value of all pixels in the square.
 *
 * Pixels of the square which fall outside the image do not count towards the
 * average. They are ignored (e.g. 5x5 box will turn into a 3x3 box in the
 * corner).
 *
 */
void filter_blur(struct image *img, void *r) {
  struct pixel(*image_data)[img->size_x] =
      (struct pixel(*)[img->size_x])img->px;
int radius = *((int *)r);

  struct pixel(*new_data)[img->size_x] =
      malloc(sizeof(struct pixel) * img->size_x * img->size_y);

  if (!new_data) {
    return;
  }

  /* We iterate over all pixels */
  for (long i = 0; i < img->size_y; i++) {
    for (long j = 0; j < img->size_x; j++) {

      unsigned red = 0, green = 0, blue = 0, alpha = 0;

      long y_start = i - radius;
      long y_stop = i + radius;

      long x_start = j - radius;
      long x_stop = j + radius;

      y_start = y_start < 0 ? 0 : y_start;
      y_stop = y_stop >= img->size_y ? img->size_y - 1 : y_stop;

      x_start = x_start < 0 ? 0 : x_start;
      x_stop = x_stop >= img->size_x ? img->size_x - 1 : x_stop;

      /* We iterate over all pixels in the square */
      for (long y = y_start; y <= y_stop; y++) {
        for (long x = x_start; x <= x_stop; x++) {

          struct pixel current = image_data[y][x];

          red += current.red;
          blue += current.blue;
          green += current.green;
          alpha += current.alpha;
        }
      }

      int num_pixels = (y_stop - y_start + 1) * (x_stop - x_start + 1);
      /* Calculate the average */
      red /= num_pixels;
      green /= num_pixels;
      blue /= num_pixels;
      alpha /= num_pixels;

      /* Assign new values */
      new_data[i][j].red = red;
      new_data[i][j].green = green;
      new_data[i][j].blue = blue;
      new_data[i][j].alpha = alpha;
    }
  }

  memcpy(img->px, new_data, sizeof(struct pixel) * img->size_x * img->size_y);
  free(new_data);
  return;
}

/* This filter just negates every color in the image */
void filter_negative(struct image *img, void *noarg) {
  struct pixel(*image_data)[img->size_x] =
      (struct pixel(*)[img->size_x])img->px;

  /* Iterate over all the pixels */
  for (long i = 0; i < img->size_y; i++) {
    for (long j = 0; j < img->size_x; j++) {

      struct pixel current = image_data[i][j];
      struct pixel neg;

      neg.red = 255 - current.red;
      neg.green = 255 - current.green;
      neg.blue = 255 - current.blue;
      neg.alpha = current.alpha;

      image_data[i][j] = neg;
    }
  }
}

void filter_transparency(struct image *img, void *transparency) {
  struct pixel(*image_data)[img->size_x] =
      (struct pixel(*)[img->size_x])img->px;
  uint8_t local_alpha = *((uint8_t *)transparency);
  /* Iterate over all pixels */
  for (long i = 0; i < img->size_y; i++) {
    for (long j = 0; j < img->size_x; j++) {

      image_data[i][j].alpha = local_alpha;
    }
  }
}

/* This filter applies a sepia tone to the input image. The depth argument
 * controls the magnitude of the sepia effect. The formula for each pixel is:
 * average = (r + g + b) / 3
 * red = average + 2*depth, capped at 255
 * green = average + depth, capped at 255
 * blue = average
 * Alpha is unaffected. */
void filter_sepia(struct image *img, void *depth_arg) {
  struct pixel(*image_data)[img->size_x] =
      (struct pixel(*)[img->size_x])img->px;
  uint8_t depth = *(uint8_t *)depth_arg;

  /* Iterate over all pixels */
  for (long i = 0; i < img->size_y; i++) {
    for (long j = 0; j < img->size_x; j++) {
      
      struct pixel current = image_data[i][j];
      int red = 0, green = 0, blue = 0;
      // getting the RGB of image into new values
      red += current.red;
      green += current.green;
      blue += current.blue;
      // getting the average
      int average = (red + blue + green) / 3;
      // sepia values for making image sepia as given on the ppt guide
      int sepiaRed = average + 2*depth;
      int sepiaGreen = average + depth;
      int sepiaBlue = average;
      // setting the cap of 255 for sepia red and sepia green
      sepiaRed = sepiaRed > 255 ? 255 : sepiaRed;
      sepiaGreen = sepiaGreen > 255 ? 255 : sepiaGreen;
      // applying the changes
      image_data[i][j].red = sepiaRed;
      image_data[i][j].green = sepiaGreen;  
      image_data[i][j].blue = sepiaBlue;
    }
  }
}

/* This filter replaces all pixels whose average rgb value is over the
 * threshold with white, otherwise black. Alpha is unaffected.
 */
void filter_bw(struct image *img, void *threshold_arg) {
  struct pixel(*image_data)[img->size_x] =
      (struct pixel(*)[img->size_x])img->px;
  uint8_t threshold = *(uint8_t *)threshold_arg;

  /* Iterate over all pixels */
  for (long i = 0; i < img->size_y; i++) {
    for (long j = 0; j < img->size_x; j++) {
      int red = 0, green = 0, blue = 0, average_bw = 0;
      struct pixel current = image_data[i][j];  
      // we get the current RGB of image into seprate values
      red += current.red;
      green += current.green;
      blue += current.blue;
      // caluculate the average by dividing the sum of RGB
      average_bw = (red + green + blue)/3;
      // if the average is above the threshold then WHITE
      if (average_bw > threshold){
        image_data[i][j].red = 255;
        image_data[i][j].green = 255;  
        image_data[i][j].blue = 255;
      }else{
        // if less than threshold BLACK
        image_data[i][j].red = 0;
        image_data[i][j].green = 0;  
        image_data[i][j].blue = 0;
      }

    }
  }
}

/* This filter is used to detect edges by computing the gradient for each
 * pixel and comparing it to the threshold argument. When the gradient exceeds
 * the threshold, the pixel is replaced by black, otherwise white.
 * Alpha is unaffected.
 *
 * For each pixel and channel, the x-gradient and y-gradient are calculated
 * by using the following convolution matrices:
 *     Gx            Gy
 *  -1  0  +1     +1 +2 +1
 *  -2  0  +2      0  0  0
 *  -1  0  +1     -1 -2 -1
 * The convolution matrix are multiplied with the neighbouring pixels'
 * channel values. At edges, the indices are bounded.
 * Suppose the red channel values for the pixel and its neighbours are as
 * follows: 11 22 33 44 55 66 77 88 99 the x-gradient for red is: (-1*11 + 1*33
 * + -2*44 + 2*66 + -1*77  + 1*99).
 *
 * The net gradient for each channel = sqrt(g_x^2 + g_y^2)
 * For the pixel, the net gradient = sqrt(g_red^2 + g_green^2 + g_blue_2)
 */
void filter_edge_detect(struct image *img, void *threshold_arg) {
  struct pixel(*image_data)[img->size_x] =
      (struct pixel(*)[img->size_x])img->px;
  uint8_t threshold = *(uint8_t *)threshold_arg;
  double weights_x[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
  double weights_y[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};
  // creating new image to save the new image
  struct pixel image2[img->size_y][img->size_x];
  // FOR DETECTING CORNER CASES but it works for other parts too
  #define BOUND(x, min, max)                                                     \
    ((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x));

  double g_red = 0.0, g_blue = 0.0, g_green = 0.0, g = 0.0;
  // casting to double 
  double thr = (double)threshold;
  // image2 = image_data;
  /* Iterate over all pixels */
  for (long i = 0; i < img->size_y; i++) {
    for (long j = 0; j < img->size_x; j++) {
      // copying the image_data into new image
      image2[i][j] = image_data[i][j];    
      double x_red = 0.0, y_red = 0.0, x_blue = 0.0, y_blue = 0.0, x_green = 0.0, y_green = 0.0;
      for (long k = -1; k < 2; k++){
        for (long l = -1; l < 2; l++) {
          long y_dash = BOUND(i + k, 0, img->size_y - 1);
          long x_dash = BOUND(j + l, 0, img->size_x - 1);
          // calclulating gradient for red color of (x, y) axes
          x_red += (weights_x[k+1][l+1]) * image_data[y_dash][x_dash].red;
          y_red += weights_y[k+1][l+1] * image_data[y_dash][x_dash].red;
          
          // calclulating gradient for blue color of (x, y) axes
          x_blue += (weights_x[k+1][l+1]) * image_data[y_dash][x_dash].blue;
          y_blue += weights_y[k+1][l+1] * image_data[y_dash][x_dash].blue;
          
          // calclulating gradient for green color of (x, y) axes
          x_green += weights_x[k+1][l+1] * image_data[y_dash][x_dash].green;
          y_green += weights_y[k+1][l+1] * image_data[y_dash][x_dash].green;
        }
      }
      // gradients for RGB
      g_red = sqrt(x_red*x_red +y_red*y_red);
      g_blue = sqrt(x_blue * x_blue + y_blue * y_blue);
      g_green = sqrt( x_green * x_green + y_green * y_green);
      // net gradient
      g = sqrt(g_red * g_red + g_green * g_green + g_blue * g_blue);
      
      // if above threshold then BLACK
      if (g > thr){
        image2[i][j].red = 0;
        image2[i][j].green = 0;  
        image2[i][j].blue = 0;
      }else{
        // if below threshold then WHITE
        image2[i][j].red = 255;
        image2[i][j].green = 255;  
        image2[i][j].blue = 255;
        }
  }
}
  // copying the changes image2 into the orignal image_data
  for (long i = 0; i < img->size_y; i++) {
  for (long j = 0; j < img->size_x; j++) {
    image_data[i][j] = image2[i][j];     
    }
  }
}

/* This filter performs keying, replacing the color specified by the argument
 * by a transparent pixel */
void filter_keying(struct image *img, void *key_color) {
  struct pixel(*image_data)[img->size_x] =
      (struct pixel(*)[img->size_x])img->px;
  struct pixel key = *(struct pixel *)key_color;

  /* Iterate over all pixels */
  for (long i = 0; i < img->size_y; i++) {
    for (long j = 0; j < img->size_x; j++) {
      // 0 is the most transparent while 255 is clear image(not transparent)
      // also of key value of red image equal to key red then make it transparent
      if (key.red == image_data[i][j].red){
        image_data[i][j].alpha = 0;
      }
      // if the key green equal to the image green then make it transparent
      else if (key.green == image_data[i][j].green){
        image_data[i][j].alpha = 0;
      }
      // if they blue key equal to the image blue then make it transparent
      else if (key.blue == image_data[i][j].blue){
        image_data[i][j].alpha = 0;
      }
    }
  }
}

/* The filter structure comprises the filter function, its arguments and the
 * image we want to process */
struct filter {
  void (*filter)(struct image *img, void *arg);
  void *arg;
  struct image *img;
};

void execute_filter(struct filter *fil) { fil->filter(fil->img, fil->arg); }

int __attribute__((weak)) main(int argc, char *argv[]) {
  struct filter fil;
  char arg[256];
  char input[255];
  char output[255];
  char command[256];
  int radius;
  struct pixel px;
  uint8_t alpha, depth, threshold;
  uint32_t key;
  struct image *img = NULL;
  double weights[] = {0.2125, 0.7154, 0.0721};

  /* Some filters take no arguments, while others have 1 */
  if (argc != 4 && argc != 5) {
    goto error_usage;
  }

  fil.filter = NULL;
  fil.arg = NULL;

  /* Copy arguments for easier reference */
  strncpy(input, argv[1], 255);
  input[254] = '\0';

  strncpy(output, argv[2], 255);
  output[254] = '\0';

  strncpy(command, argv[3], 256);
  command[255] = '\0';

  /* If the filter takes an argument, copy it */
  if (argv[4]) {
    strncpy(arg, argv[4], 256);
    arg[255] = '\0';
  }

  /* Error when loading a png image */
  if (load_png(input, &img)) {
    exit(1);
  }

  /* Set up the filter structure */
  fil.img = img;

  /* Decode the filter */
  if (!strcmp(command, "grayscale")) {
    fil.filter = filter_grayscale;
    fil.arg = weights;
  } else if (!strcmp(command, "negative")) {
    fil.arg = NULL;
    fil.filter = filter_negative;
  } else if (!strcmp(command, "blur")) {
    /* Bad filter radius will just be interpretted as 0 - no change to the image
     */
    radius = atoi(arg);
    fil.filter = filter_blur;
    fil.arg = &radius;
  } else if (!strcmp(command, "alpha")) {

    char *end_ptr;
    alpha = strtol(arg, &end_ptr, 16);

    if (*end_ptr) {
      goto error_usage;
    }

    fil.filter = filter_transparency;
    fil.arg = &alpha;
  }
  else if (!strcmp(command, "sepia")) {
    char *end_ptr;
    depth = strtol(arg, &end_ptr, 16);

    if (*end_ptr) {
      goto error_usage;
    }

    fil.filter = filter_sepia;
    fil.arg = &depth;
  } else if (!strcmp(command, "bw")) {
    char *end_ptr;
    threshold = strtol(arg, &end_ptr, 16);

    if (*end_ptr) {
      goto error_usage;
    }

    fil.filter = filter_bw;
    fil.arg = &threshold;
  } else if (!strcmp(command, "edge")) {
    char *end_ptr;
    threshold = strtol(arg, &end_ptr, 16);

    if (*end_ptr) {
      goto error_usage;
    }

    fil.filter = filter_edge_detect;
    fil.arg = &threshold;
  } else if (!strcmp(command, "keying")) {
    char *end_ptr;
    key = strtol(arg, &end_ptr, 16);

    if (*end_ptr) {
      goto error_usage;
    }

    px.red = (key & 0xff0000) >> 16;
    px.green = (key & 0x00ff00) >> 8;
    px.blue = (key & 0x0000ff);
    px.alpha = 255;
    fil.filter = filter_keying;
    fil.arg = &px;
  }

  /* Invalid filter check */
  if (fil.filter) {
    execute_filter(&fil);
  } else {
    goto error_filter;
  }

  store_png(output, img, NULL, 0);
  free(img->px);
  free(img);
  return 0;

error_filter:
  free(img->px);
  free(img);
error_usage:
  printf("Usage: %s input_image output_image filter_name [filter_arg]\n",
         argv[0]);
  printf("Filters:\n");
  printf("grayscale\n");
  printf("negative\n");
  printf("blur radius_arg\n");
  printf("alpha hex_alpha\n");
  printf("sepia hex_depth\n");
  printf("bw hex_threshold\n");
  printf("edge hex_threshold\n");
  printf("keying hex_color\n");
  return 1;
}