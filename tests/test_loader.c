#include "../loader.h"
#include <stdio.h>


void read_png_test(const char *file_name) {
	masker_image_t image;
	int err_code = read_png_file(&image, file_name);
	if (err_code) {
		printf("Received code %i for %s \n", err_code, file_name);
		return;
	}

	printf("Successfully loaded %s \n", file_name);
	free_image_memory(&image);
	return;
}


void read_mask_test(const char *file_name) {
	masker_mask_t mask;
	int err_code = read_mask_file(&mask, file_name);
	if (err_code) {
		printf("Received code %i for %s \n", err_code, file_name);
		return;
	}

	printf("Successfully loaded %s \n", file_name);
	free_mask_memory(&mask);
	return;
}


int main() {
	printf("Conducting read_png tests.\n");
	read_png_test("error0.png");	// Does not exist
	read_png_test("error1.png");	// Empty file
	read_png_test("error2.png");	// Very small file
	read_png_test("error3.png");	// Random text file
	read_png_test("error4.png");	// Mushroom picture - 700x700
	read_png_test("mask.png");	// Actual mask

	printf("Conducting read_mask tests.\n");
	read_mask_test("error0.png");
	read_mask_test("error1.png");
	read_mask_test("error2.png");
	read_mask_test("error3.png");
	read_mask_test("error4.png");
	read_mask_test("mask.png");
}
