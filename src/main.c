#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#define IMAGE_IMPLEMENTATION
#include "image.h"

float G_x_m[9] = {
	-1, 0, 1,
	-2, 0, 2,
	-1, 0, 1,
};

float G_y_m[9] = {
	-1,-2,-1,
	0, 0, 0,
	1, 2, 1,
};

float mat_conv_mult(float *a, float *b, size_t r, size_t c) {
	float res = 0;
	for (size_t y = 0; y < r; y++) {
		for (size_t x = 0; x < c; x++) {
			res += a[y * c + x] * b[y * c + x];
		}
	}
	return res;
}

void sbl_from_image(Image orig, float *res) {
	float A[9];

	for (size_t y = 0; y < orig.height; y++) {
		for (size_t x = 0; x < orig.width; x++) {
			for (int i = -1; i < 2; i++) {
				for (int j = -1; j < 2; j++) {
					int nx = j + x;
					int ny = i + y;
					if (nx < 0 && ny < 0)
						A[(i+1)*3+(j+1)] = COLOR_GRAYSCALE(IMAGE_AT(orig, nx+1, ny+1));
					else if (nx < 0)
						A[(i+1)*3+(j+1)] = COLOR_GRAYSCALE(IMAGE_AT(orig, nx+1, ny));
					else if (ny < 0)
						A[(i+1)*3+(j+1)] = COLOR_GRAYSCALE(IMAGE_AT(orig, nx, ny+1));
					else if (ny > orig.height && nx > orig.width)
						A[(i+1)*3+(j+1)] = COLOR_GRAYSCALE(IMAGE_AT(orig, nx-1, ny-1));
					else if (nx > orig.width)
						A[(i+1)*3+(j+1)] = COLOR_GRAYSCALE(IMAGE_AT(orig, nx-1, ny));
					else if (ny > orig.height)
						A[(i+1)*3+(j+1)] = COLOR_GRAYSCALE(IMAGE_AT(orig, nx, ny-1));
					else
						A[(i+1)*3+(j+1)] = COLOR_GRAYSCALE(IMAGE_AT(orig, nx, ny));
					A[(i+1)*3+(j+1)] /= 255.0;
				}
			}

			float G_x = mat_conv_mult(G_x_m, A, 3, 3);
			float G_y = mat_conv_mult(G_y_m, A, 3, 3);	

			float G = sqrtf(G_x * G_x + G_y * G_y);
			res[y * orig.width + x] = G;
		}
	}
}

void sbl_find_minimal_column(float *sbl, size_t r, size_t c, size_t *res) {
	for (size_t i = 1; i < r; i++) {
		for (size_t j = 0; j < c; j++) {
			float min = 999999.0;
			if (j > 0) min = fminf(sbl[(i-1) * c + j - 1], min);
			if (j < c - 1) min = fminf(sbl[(i-1) * c + j + 1], min);
			min = fminf(sbl[(i-1) * c + j], min);
			sbl[i * c + j] += min;
		}
	}

	size_t smallest_index = 0;
	float smallest_energy = 999999.0;
	for (size_t i = 0; i < c; i++) {
		if (sbl[(r-1) * c + i] < smallest_energy) {
			smallest_energy = sbl[(r-1) * c + i];
			smallest_index = i;
		}
	}

	int cur_ind = smallest_index;
	res[r-1] = (size_t) cur_ind;
	for (size_t i = r-1; i >= 1; i--) {
		float min = 999999.0;
		int change_cur_ind = cur_ind;

		if (cur_ind > 0) {
			if (sbl[(i-1) * c + cur_ind - 1] < min) {
				min = sbl[(i-1) * c + cur_ind - 1];
				change_cur_ind = cur_ind - 1;
			}
		} 
		if (cur_ind < c - 1) {
			if (sbl[(i-1) * c + cur_ind + 1] < min) {
				min = sbl[(i-1) * c + cur_ind + 1];
				change_cur_ind = cur_ind + 1;
			}
		}
		if (sbl[(i-1) * c + cur_ind] < min) {
			min = sbl[(i-1) * c + cur_ind];
			change_cur_ind = cur_ind;
		}

		cur_ind = change_cur_ind;
		res[i-1] = (size_t) cur_ind;
	}
}

void sbl_find_minimal_row(float *sbl, size_t r, size_t c, size_t *res) {
	for (size_t j = 1; j < c; j++) {
		for (size_t i = 0; i < r; i++) {
			float min = 999999.0;
			if (i > 0) min = fminf(sbl[(i-1) * c + j - 1], min);
			if (i < r - 1) min = fminf(sbl[(i+1) * c + j - 1], min);
			min = fminf(sbl[i * c + j - 1], min);
			sbl[i * c + j] += min;
		}
	}

	size_t smallest_index = 0;
	float smallest_energy = 999999.0;
	for (size_t i = 0; i < c; i++) {
		if (sbl[i * c + (c-1)] < smallest_energy) {
			smallest_energy = sbl[i * c + (c-1)];
			smallest_index = i;
		}
	}

	int cur_ind = smallest_index;
	res[c-1] = (size_t) cur_ind;
	for (size_t i = c-1; i >= 1; i--) {
		float min = 999999.0;
		int change_cur_ind = cur_ind;

		if (cur_ind > 0) {
			if (sbl[(cur_ind-1) * c + i - 1] < min) {
				min = sbl[(cur_ind-1) * c + i - 1];
				change_cur_ind = cur_ind - 1;
			}
		} 
		if (cur_ind < r - 1) {
			if (sbl[(cur_ind+1) * c + i - 1] < min) {
				min = sbl[(cur_ind+1) * c + i - 1];	
				change_cur_ind = cur_ind + 1;
			}
		}
		if (sbl[cur_ind * c + i - 1] < min) {
			min = sbl[cur_ind * c + i - 1];
			change_cur_ind = cur_ind;
		}

		cur_ind = change_cur_ind;
		res[i-1] = (size_t) cur_ind;
	}
}

void image_remove_column(Image pimg, Image *nimg, size_t *res) {
	nimg->width--;
	for (size_t i = 0; i < pimg.height; i++) {
		size_t j = 0;
		for (size_t k = 0; k < pimg.width; k++) {
			if (k != res[i]) {
				IMAGE_AT((*nimg), j, i) = IMAGE_AT(pimg, k, i);
				j++;
			}
		}
	}
}

void image_remove_row(Image pimg, Image *nimg, size_t *res) {
	nimg->height--;
	for (size_t j = 0; j < pimg.width; j++) {
		size_t i = 0;
		for (size_t k = 0; k < pimg.height; k++) {
			if (k != res[j]) {
				IMAGE_AT((*nimg), j, i) = IMAGE_AT(pimg, j, k);
				i++;
			}
		}
	}
}

int main(int argc, char **argv) {
	char *input_file;
	char *output_file;
	size_t iters;

	if (argc < 4) {
		printf("Usage: [INPUT_IMAGE] [OUTPUT_IMAGE] [ITERS]\n");
		printf("INPUT_IMAGE  - path to the input .bmp image file\n");
		printf("OUTPUT_IMAGE - path to save the edited .bmp image\n");
		printf("ITERS        - how many times to apply the effect\n");
		exit(0);	
	}

	input_file = argv[1];
	output_file = argv[2];
	iters = atoi(argv[3]);

	if (iters == 0) {
		printf("ITERS must be greater than 0\n");
		exit(0);
	}

	Image pimg = image_from_bmp(input_file);
	Image nimg = image_alloc(pimg.width, pimg.height);
	Image bimg;

	size_t *res = malloc((int)fmax(pimg.height, pimg.width) * sizeof(size_t));
	float *sbl = malloc(pimg.width * pimg.height * sizeof(float));

	for (size_t iter = 0; iter < iters; iter++) {
		sbl_from_image(pimg, sbl);
		sbl_find_minimal_column(sbl, pimg.height, pimg.width, res);
		image_remove_column(pimg, &nimg, res);

		bimg = pimg;
		pimg = nimg;
		nimg = pimg;

		sbl_from_image(pimg, sbl);
		sbl_find_minimal_row(sbl, pimg.height, pimg.width, res);
		image_remove_row(pimg, &nimg, res);

		bimg = pimg;
		pimg = nimg;
		nimg = pimg;
	}

	image_to_bmp(pimg, output_file);
	free(sbl);
	free(res);
	return 0;
}
