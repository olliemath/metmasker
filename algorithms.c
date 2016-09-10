#include "algorithms.h"


static int met_to_gray(png_byte *result, png_byte *pixel)
{
  /* Shonky hash - convert met colors to rain in grayscale */
  if (pixel[3] < 255) {
    *result = 0;
    return MASKER_SUCCESS;
  }
  switch (pixel[0]) {
  case 0:
    *result = 1;
    return MASKER_SUCCESS;
  case 50:
    *result = 3;
    return MASKER_SUCCESS;
  case 127:
    *result = 6;
    return MASKER_SUCCESS;
  case 254:
    if (pixel[1] == 203) {
      *result = 12;
      return MASKER_SUCCESS;
    } else if (pixel[1] == 152) {
      *result = 24;
      return MASKER_SUCCESS;
    } else if (pixel[2] == 0) {
      *result = 48;
      return MASKER_SUCCESS;
    } else if (pixel[2] == 254) {
      *result = 96;
      return MASKER_SUCCESS;
    } else {
      result = NULL;
      return MASKER_FAILURE;
    }
  case 229:
    *result = 192;
    return MASKER_SUCCESS;
  default:
    result = NULL;
    return MASKER_FAILURE;
  }
}


static int met_to_float(float *result, png_byte *pixel)
{
  /* Shonky hash - convert met colors to rain amounts */
  if (pixel[3] < 255) {
    *result = 0.0;
    return MASKER_SUCCESS;
  }
  switch (pixel[0]) {
  case 0:
    *result = 0.25;
    return MASKER_SUCCESS;
  case 50:
    *result = 0.75;
    return MASKER_SUCCESS;
  case 127:
    *result = 1.5;
    return MASKER_SUCCESS;
  case 254:
    if (pixel[1] == 203) {
      *result = 3.0;
      return MASKER_SUCCESS;
    } else if (pixel[1] == 152) {
      *result = 6.0;
      return MASKER_SUCCESS;
    } else if (pixel[2] == 0) {
      *result = 12.0;
      return MASKER_SUCCESS;
    } else if (pixel[2] == 254) {
      *result = 24.0;
      return MASKER_SUCCESS;
    } else {
      *result = 0.0;
      return MASKER_FAILURE;
    }
  case 229:
    *result = 48.0;
    return MASKER_SUCCESS;
  default:
    *result = 0.0;
    return MASKER_FAILURE;
  }
}


static int gray_to_channel(png_byte pixel) {
  /* Convert grayscale rain value to neural net channel */
  switch (pixel) {
    case 1: return 0;
    case 3: return 1;
    case 6: return 2;
    case 12: return 3;
    case 24: return 4;
    case 48: return 5;
    case 96: return 6;
    case 192: return 7;
    default: return 0;
  }
}



int mask_total_met_image(
  float* res, masker_mask_t mask, const char* file_name)
{
  masker_image_t image;
  int error_bit = read_png_file(&image, file_name);
  if (error_bit != MASKER_SUCCESS) return error_bit;

  if (image.bytes_per_pixel != 4) {
    free_image_memory(&image);
    return MASKER_MET_COLOR_ERROR;
  }

  *res = 0.0;
  error_bit = 0;
  for (int y=mask.y_min; y<mask.y_max; y++) {
    png_byte *mask_row = mask.image[y];
    png_byte *image_row = image.image[y];
    for (int x=mask.x_min; x<mask.x_max; x++) {
      if (mask_row[x * mask.bytes_per_pixel] == 0) continue;
      float value;
      png_byte* pixel = &(image_row[x * image.bytes_per_pixel]);
      error_bit |= met_to_float(&value, pixel);
      *res += value;
    }
  }

  free_image_memory(&image);
  if (error_bit != MASKER_SUCCESS) return MASKER_MET_COLOR_ERROR;
  return MASKER_SUCCESS;
}


int mask_total_gray_image(
  float* res, masker_mask_t mask, const char *file_name)
{
  masker_image_t image;
  int error_bit = read_png_file(&image, file_name);
  if (error_bit != MASKER_SUCCESS) return error_bit;

  if (image.bytes_per_pixel != 1) {
    free_image_memory(&image);
    return MASKER_COLOR_TYPE_ERROR;
  }

  error_bit = 0;
  int total = 0;
  for (int y=mask.y_min; y<mask.y_max; y++) {
    png_byte *mask_row = mask.image[y];
    png_byte *image_row = image.image[y];
    for (int x=mask.x_min; x<mask.x_max; x++) {
      if (mask_row[x * mask.bytes_per_pixel] == 0) continue;
      total += image_row[x];
    }
  }
  *res = 0.25 * (float)total;   // Grayscale pixels are rain scaled up by 4

  free_image_memory(&image);
  return MASKER_SUCCESS;
}


int mask_gray_image(
  float *data_ptr, masker_mask_t mask, const char *file_name)
{
  masker_image_t res;
  int error_bit = read_png_file(&res, file_name);
  if (error_bit != MASKER_SUCCESS) return error_bit;

  if (res.bytes_per_pixel != 1) {
    free_image_memory(&res);
    return MASKER_COLOR_TYPE_ERROR;
  }

  for (int y=0; y<HEIGHT; y++) {
    png_byte *mask_row = mask.image[y];
    png_byte *image_row = res.image[y];
    for (int x=0; x<WIDTH; x++) {
      if (mask_row[x * mask.bytes_per_pixel] == 0)
        data_ptr[y * HEIGHT + x] = 0.0;
      else
        data_ptr[y * HEIGHT + x] = 0.25 * (float)image_row[x];
    }
  }
  free_image_memory(&res);
  return MASKER_SUCCESS;
}


int mask_split_gray_image(
  float *data_ptr, masker_mask_t mask, const char *file_name)
{
  masker_image_t image;
  int error_bit = read_png_file(&image, file_name);
  if (error_bit != MASKER_SUCCESS) return error_bit;

  if (image.bytes_per_pixel != 1) {
    free_image_memory(&image);
    return MASKER_COLOR_TYPE_ERROR;
  }

  /* Zero the object first. */
  for (int z=0; z<8*HEIGHT*WIDTH; z++) {
    data_ptr[z] = 0.0;
  }

  /* Split the data among channels for neural net */
  for (int y=0; y<HEIGHT; y++) {
    png_byte *mask_row = mask.image[y];
    png_byte *image_row = image.image[y];
    for (int x=0; x<WIDTH; x++) {
      if (mask_row[x * mask.bytes_per_pixel] == 0) continue;
      if (image_row[x] == 0) continue;
      int channel = gray_to_channel(image_row[x]);
      data_ptr[(channel * WIDTH + y) * HEIGHT + x] = 1.0;
    }
  }

  free_image_memory(&image);
  return MASKER_SUCCESS;
}


int met_image_to_gray(
  masker_image_t *res, const char *file_name)
{
  masker_image_t met_image;
  int error_bit = read_png_file(&met_image, file_name);
  if (error_bit != MASKER_SUCCESS) return error_bit;

  if (met_image.bytes_per_pixel != 4) {
    free_image_memory(&met_image);
    return MASKER_MET_COLOR_ERROR;
  }

  res->image = malloc(HEIGHT * sizeof(png_bytep));
  if (res->image == NULL) {
    free_image_memory(&met_image);
    return MASKER_MEMORY_ERROR;
  }
  res->bytes_per_pixel = 1;
  res->color_type = 0;
  res->is_freed = 0;

  for (int y=0; y<HEIGHT; y++) {
    res->image[y] = malloc(WIDTH * sizeof(png_byte));
    if (res->image[y] == NULL) {
      for (int w=0; w<y; w++) {free(res->image[w]);}
      free(res->image);
      free_image_memory(&met_image);
      return MASKER_MEMORY_ERROR;
    }
    png_byte *in_row = met_image.image[y];
    png_byte *out_row = res->image[y];
    for (int x=0; x<WIDTH; x++) {
      error_bit |= met_to_gray(
        &(out_row[x]), &(in_row[x * met_image.bytes_per_pixel]));
    }
  }
  if (error_bit != MASKER_SUCCESS) {
    free_image_memory(res);
    free_image_memory(&met_image);
    return MASKER_MET_COLOR_ERROR;
  }

  free_image_memory(&met_image);
  return MASKER_SUCCESS;
}

int load_gray_to_array(float *data_ptr, const char *file_name) {
  masker_image_t image;
  int error_bit = read_png_file(&image, file_name);
  if (error_bit != MASKER_SUCCESS) return error_bit;

  if (image.bytes_per_pixel != 1) {
    free_image_memory(&image);
    return MASKER_MET_COLOR_ERROR;
  }

  for (int y=0; y<HEIGHT; y++) {
    png_byte *row = image.image[y];
    for (int x=0; x<WIDTH; x++) {
      data_ptr[y * HEIGHT + x] = 0.25 * (float)row[x];
    }
  }

  free_image_memory(&image);
  return MASKER_SUCCESS;
}
