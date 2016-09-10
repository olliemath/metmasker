#ifndef MASKER_ALGORITHMS_H
#define MASKER_ALGORITHMS_H
#include "loader.h"
#include <stdlib.h>
#include <png.h>


/* ===== MASKING FUNCTIONS ===== */
int mask_total_met_image(
  float *res, masker_mask_t mask, const char* file_name);

int mask_total_gray_image(
  float *res, masker_mask_t mask, const char *file_name);

int mask_gray_image(
  float *data_ptr, masker_mask_t mask, const char *file_name);

int mask_split_gray_image(
  float *data_ptr, masker_mask_t mask, const char *file_name);

/* ===== MISCELANEOUS OTHER FUNCTIONS ===== */
int met_image_to_gray(masker_image_t *res, const char *file_name);

int load_gray_to_array(float *data_ptr, const char *file_name);

#endif
