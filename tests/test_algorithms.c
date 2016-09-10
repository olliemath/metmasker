#include "../algorithms.h"
#include <stdio.h>


void test_met_to_gray(const char *in_file) {
  masker_image_t result;
  int err_code = met_image_to_gray(&result, in_file);
  if (err_code) {
    printf("Got error code %i for %s\n", err_code, in_file);
    return;
  }

  printf("Masking went successfully: %s\n", in_file);
  free_image_memory(&result);
  return;
}

void test_met_total_image(const char *mask_file, const char *in_file) {
  masker_mask_t mask;
  int err_code = read_mask_file(&mask, mask_file);
  if (err_code) {
    printf("Got code %i from mask file %s \n", err_code, mask_file);
    return;
  }

  float res;
  err_code = mask_total_met_image(&res, mask, in_file);
  if (err_code) {
    printf("Got code %i summing %s\n", err_code, in_file);
    free_mask_memory(&mask);
    return;
  }

  printf("successfully masked %s with %s\n", in_file, mask_file);
  free_mask_memory(&mask);
  return;
}

void test_gray_total_image(const char *mask_file, const char *in_file) {
  masker_mask_t mask;
  int err_code = read_mask_file(&mask, mask_file);
  if (err_code) {
    printf("Got code %i from mask file %s \n", err_code, mask_file);
    return;
  }

  float res;
  err_code = mask_total_gray_image(&res, mask, in_file);
  if (err_code) {
    printf("Got code %i summing %s\n", err_code, in_file);
    free_mask_memory(&mask);
    return;
  }

  printf("successfully masked %s with %s\n", in_file, mask_file);
  free_mask_memory(&mask);
  return;
}


int main() {
  // Test met to gray
  test_met_to_gray("error0.png");
  test_met_to_gray("error1.png");
  test_met_to_gray("error2.png");
  test_met_to_gray("error3.png");
  test_met_to_gray("error4.png");
  test_met_to_gray("mask.png");
  test_met_to_gray("image.png");

  // Test total met
  test_met_total_image("error4.png", "image.png");
  test_met_total_image("mask.png", "error0.png");
  test_met_total_image("mask.png", "error1.png");
  test_met_total_image("mask.png", "error4.png");
  test_met_total_image("mask.png", "mask.png");
  test_met_total_image("mask.png", "image.png");
  test_met_total_image("white.png", "image.png");
  test_met_total_image("image.png", "image.png");

  // Test total gray
  test_gray_total_image("error4.png", "gray.png");
  test_gray_total_image("mask.png", "error0.png");
  test_gray_total_image("mask.png", "error1.png");
  test_gray_total_image("mask.png", "error4.png");
  test_gray_total_image("mask.png", "mask.png");
  test_gray_total_image("mask.png", "image.png");
  test_gray_total_image("mask.png", "gray.png");
  test_gray_total_image("white.png", "gray.png");
  test_gray_total_image("image.png", "gray.png");
}
