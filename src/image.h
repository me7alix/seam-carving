#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
	uint8_t r, g, b;
} Color;

typedef struct {
	size_t width, height;
	Color *pixels;
} Image;

#define IMAGE_AT(image, x, y) \
	image.pixels[(y)*image.width+(x)]
#define COLOR_GRAYSCALE(color) \
	((color.r + color.g + color.b) / 3.0)

void image_to_bmp(Image image, char *file);
Image image_from_bmp(char *file);
Image image_alloc(size_t width, size_t height);
void image_free(Image img);

#endif

#ifdef IMAGE_IMPLEMENTATION

void image_to_bmp(Image image, char *file) {
	int32_t w = image.width, h = image.height;
	int32_t row_padded = (w * 3 + 3) & ~3;
	int32_t filesize = 54 + row_padded * image.height;

	uint8_t fh[14] = {
		'B','M',
		(unsigned char)(filesize	  ), (unsigned char)(filesize >>  8),
		(unsigned char)(filesize >> 16), (unsigned char)(filesize >> 24),
		0,0,0,0,54,0,0,0
	};
	uint8_t ih[40] = {
		40,0,0,0,
		(unsigned char)(w	   ), (unsigned char)(w >>	8),
		(unsigned char)(w >> 16), (unsigned char)(w >> 24),
		(unsigned char)(h	   ), (unsigned char)(h >>	8),
		(unsigned char)(h >> 16), (unsigned char)(h >> 24),
		1,0,24,0,0,0,0,0,0,0,0,0,19,11,0,0,19,11,0,0,0,0,0,
		0,0,0,0,0
	};

	FILE *f = fopen(file, "wb");
	fwrite(fh, 1, 14, f);
	fwrite(ih, 1, 40, f);

	uint8_t px[3];
	uint8_t pad[3] = {0};

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++){
			px[0] = image.pixels[y*w+x].b;
			px[1] = image.pixels[y*w+x].g;
			px[2] = image.pixels[y*w+x].r;
			fwrite(px, 1, 3, f);
		}
		fwrite(pad, 1, row_padded - w * 3, f);
	}

	fclose(f);
}

Image image_from_bmp(char *file) {
	FILE *f = fopen(file, "rb");
	if (!f) {
		perror("fopen");
		exit(1);
	}

	uint32_t data_offset;
	fseek(f, 10, SEEK_SET);
	if (fread(&data_offset, 4, 1, f) != 1) {
		perror("fread data_offset");
		fclose(f);
		exit(1);
	}

	int32_t width, height;
	fseek(f, 18, SEEK_SET);
	if (fread(&width, 4, 1, f) != 1 ||
			fread(&height, 4, 1, f) != 1) {
		perror("fread width/height");
		fclose(f);
		exit(1);
	}

	int flip_vertically = 0;
	if (height < 0) {
		flip_vertically = 1;
		height = -height;
	} else {
		flip_vertically = 0;
	}

	size_t w = (size_t)width;
	size_t h = (size_t)height;
	Color *pixels = (Color *)malloc(sizeof(Color) * w * h);

	int32_t row_padded = (width * 3 + 3) & ~3;

	if (fseek(f, data_offset, SEEK_SET) != 0) {
		perror("fseek to pixel data");
		free(pixels);
		fclose(f);
		exit(1);
	}

	uint8_t buffer[3];
	uint8_t padding[3];
	for (int row = 0; row < height; row++) {
		int target_row = flip_vertically
			? (height - 1 - row)
			: row;

		Color *row_ptr = pixels + (size_t)target_row * w;
		for (int col = 0; col < width; col++) {
			if (fread(buffer, 1, 3, f) != 3) {
				perror("fread pixel data");
				free(pixels);
				fclose(f);
				exit(1);
			}

			row_ptr[col].b = buffer[0];
			row_ptr[col].g = buffer[1];
			row_ptr[col].r = buffer[2];
		}

		int pad_bytes = row_padded - width * 3;
		if (pad_bytes > 0) {
			if (fread(padding, 1, pad_bytes, f) != (size_t)pad_bytes) {
				perror("fread padding");
				free(pixels);
				fclose(f);
				exit(1);
			}
		}
	}

	fclose(f);

	return (Image) {
		.width = w,
			.height = h,
			.pixels = pixels,
	};
}

Image image_alloc(size_t width, size_t height) {
	return (Image) {
		.width = width,
		.height = height,
		.pixels = (Color *)malloc(sizeof(Color) * width * height),
	};
}

void image_free(Image img) {
	free(img.pixels);
}

#endif
