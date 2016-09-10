#include <stdio.h>
#include <stdlib.h>
#include "../algorithms.h"
#include "../loader.h"

void test_split_gray(const char *mask_file, const char *im_file) {
  float *data_ptr = malloc(8 * WIDTH * HEIGHT * sizeof(float));
  if (data_ptr == NULL) return;

  masker_mask_t mask;
  int err_code = read_mask_file(&mask, mask_file);
  if (err_code) {
    free(data_ptr);
    printf("Loading mask %s failed with code %i\n", mask_file, err_code);
    return;
  }

  err_code = mask_split_gray_image(data_ptr, mask, im_file);
  if (err_code) {
    printf("Splitter failed on %s with code %i\n", im_file, err_code);
    free_mask_memory(&mask);
    free(data_ptr);
    return;
  }

  printf("Splitting %s with %s successful.\n", im_file, mask_file);
  free(data_ptr);
  free_mask_memory(&mask);
}


int main() {
  test_split_gray("error0.png", "gray.png");
  test_split_gray("error4.png", "gray.png");
  test_split_gray("white.png", "image.png");
  test_split_gray("white.png", "error1.png");
  test_split_gray("white.png", "gray.png");
  test_split_gray("mask.png", "gray.png");
}
