#include "loader.h"


/* Translate abstract PNG color codes to bytes per pixel */
static int translate_color_type(int* result, png_byte color_type) {
  switch(color_type) {
    case 0:
      *result = 1;    // Grayscale
      return MASKER_SUCCESS;
    case 2:
      *result = 3;    // RGB
      return MASKER_SUCCESS;
    case 4:
      *result = 2;    // Greyscale + Alpha
      return MASKER_SUCCESS;
    case 6:
      *result = 4;   // RGBA
      return MASKER_SUCCESS;
    default:
      return MASKER_FAILURE;
  }
}


/* Read file into memory and return pointer to image */
int read_png_file(masker_image_t *result, const char *file_name)
{
  FILE *fp = fopen(file_name, "rb");
  if (fp == NULL) {
    return MASKER_IO_ERROR;
  }

  // Check file is png
  unsigned char sig[8];
  if (fread(sig, 1, 8, fp) < 8) {
	fclose(fp);
	return MASKER_NOT_PNG_ERROR;
  }
  if (!png_check_sig(sig, 8)) {
    fclose(fp);
    return MASKER_NOT_PNG_ERROR;
  }


  // Initialise png structs
  png_structp png_ptr;
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) {
    fclose(fp);
    return MASKER_MEMORY_ERROR;
  }

  // Initialise info structs
  png_infop info_ptr;
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    fclose(fp);
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return MASKER_MEMORY_ERROR;
  }

  // Initialise IO, read png info bytes
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return MASKER_INIT_IO_ERROR;
  }
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  // Check image properties
  int width = png_get_image_width(png_ptr, info_ptr);
  int height = png_get_image_height(png_ptr, info_ptr);
  int depth = png_get_bit_depth(png_ptr, info_ptr);
  if ((width != WIDTH) || (height != HEIGHT) || (depth != DEPTH)) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return MASKER_IMAGE_SIZE_DEPTH_ERROR;
  }

  // Allocate memory and read image
  int color_type = png_get_color_type(png_ptr, info_ptr);
  int pixel_size;
  if (translate_color_type(&pixel_size, color_type) != MASKER_SUCCESS) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return MASKER_COLOR_TYPE_ERROR;
  }
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return MASKER_READ_ERROR;
  }
  png_bytep *image = malloc(sizeof(png_bytep) * HEIGHT);
  if (image == NULL) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return MASKER_MEMORY_ERROR;
  }
  for (int y=0; y<HEIGHT; y++) {
    image[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));
    if (image[y] == NULL) {
      for (int z=0; z<y; z++) {
        free(image[y]);
      }
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      fclose(fp);
      return MASKER_MEMORY_ERROR;
    }
  }
  png_read_image(png_ptr, image);

  // Clean up and return image
  fclose(fp);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  result->image = image;
  result->bytes_per_pixel = pixel_size;
  result->color_type = color_type;
  result->is_freed = 0;
  return MASKER_SUCCESS;
}


void free_image_memory(masker_image_t *image)
{
  if (image->is_freed != 0) return;
  for (int y=0; y<HEIGHT; y++) {
    free(image->image[y]);
  }
  free(image->image);
  image->is_freed = 1;
}

void free_mask_memory(masker_mask_t *image)
{
  if (image->is_freed != 0) return;
  for (int y=0; y<HEIGHT; y++) {
    free(image->image[y]);
  }
  free(image->image);
  image->is_freed = 1;
}

/* Write image to file - possibly free memory */
int write_png_file(masker_image_t image, const char *file_name)
{
  FILE *fp = fopen(file_name, "wb");
  if (fp == NULL) {
    return MASKER_IO_ERROR;
  }

  // Initialise PNG structs
  png_structp png_ptr;
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) {
    fclose(fp);
    return MASKER_MEMORY_ERROR;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    png_destroy_write_struct(&png_ptr, NULL);
    fclose(fp);
    return MASKER_MEMORY_ERROR;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return MASKER_INIT_IO_ERROR;
  }
  png_init_io(png_ptr, fp);

  // Write the image.
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return MASKER_WRITE_ERROR;
  }
  png_set_IHDR(png_ptr, info_ptr, WIDTH, HEIGHT, DEPTH, image.color_type,
    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png_ptr, info_ptr);
  png_write_image(png_ptr, image.image);
  png_write_end(png_ptr, NULL);

  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(fp);
  return MASKER_SUCCESS;
}


/* Read png file to mask struct */
int read_mask_file(masker_mask_t* result, const char *file_name)
{
  int x_min = WIDTH - 1;
  int y_min = HEIGHT - 1;
  int x_max = 0;
  int y_max = 0;

  masker_image_t image;
  int error_bit = read_png_file(&image, file_name);
  if (error_bit != MASKER_SUCCESS)
    return error_bit;

  for (int y=0; y<HEIGHT; y++) {
    png_byte *row = image.image[y];
    for (int x=0; x<WIDTH; x++) {
      png_byte *pixel =  &row[x * image.bytes_per_pixel];
      if (pixel[0] > 0) {
        if (x < x_min) x_min = x;
        if (x > x_max) x_max = x;
        if (y < y_min) y_min = y;
        if (y > y_max) y_max = y;
      }
    }
  }

  result->image = image.image,
  result->bytes_per_pixel = image.bytes_per_pixel,
  result->color_type = image.color_type,
  result->is_freed = 0;
  result->x_min = x_min;
  result->x_max = x_max;
  result->y_min = y_min;
  result->y_max = y_max;
  return MASKER_SUCCESS;
}
