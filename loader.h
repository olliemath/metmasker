#ifndef MASKER_LOADER_H
#  define MASKER_LOADER_H

#  define MASKER_SUCCESS 0
#  define MASKER_FAILURE 1
#  define MASKER_IO_ERROR 2
#  define MASKER_MEMORY_ERROR 3
#  define MASKER_INIT_IO_ERROR 4
#  define MASKER_COLOR_TYPE_ERROR 5
#  define MASKER_READ_ERROR 6
#  define MASKER_WRITE_ERROR 7
#  define MASKER_IMAGE_SIZE_DEPTH_ERROR 8
#  define MASKER_MET_COLOR_ERROR 9
#  define MASKER_NOT_PNG_ERROR 10

#  include <stdlib.h>
#  include <png.h>

#  define WIDTH 500
#  define HEIGHT 500
#  define DEPTH 8


/* Image plus metadata */
typedef struct masker_image {
  png_bytep *image;
  int bytes_per_pixel;
  int color_type;
  int is_freed;   // prevent double frees
} masker_image_t;


/* Image plus more metadata */
typedef struct masker_mask {
  png_bytep *image;
  int bytes_per_pixel;
  int color_type;
  int is_freed;
  int x_min, x_max;
  int y_min, y_max;
} masker_mask_t;

/* Functions for IO operations */
int read_png_file(masker_image_t *result, const char *file_name);
int read_mask_file(masker_mask_t *result, const char *file_name);
void free_image_memory(masker_image_t *image);
void free_mask_memory(masker_mask_t *image);
int write_png_file(masker_image_t image, const char *file_name);


#endif	// MASKER_LOADER_H
