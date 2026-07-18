/*
 * Copyright (C) 2026 Ivan Gaydardzhiev
 * Licensed under the GPL-3.0-only
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define W 256
#define H 256
#define N (W*H)

typedef struct {
	uint8_t r[N], g[N], b[N];
} fr;

static fr in, out;

static int px(int x, int y) {
	return y*W+x;
}

static int wppm(const char *p, const fr *f) {
	FILE *fp = fopen(p, "wb");
	if (!fp) {
		perror(p);
		return -1;
	}
	fprintf(fp, "P6\n%d %d\n255\n", W, H);
	for (int i = 0; i < N; i++) {
		uint8_t rgb[3] = { f->r[i], f->g[i], f->b[i] };
		if (fwrite(rgb, 1, 3, fp) != 3) {
			perror("fwrite");
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);
	return 0;
}

static int rppm(const char *p, fr *f) {
	FILE *fp = fopen(p, "rb");
	if (!fp) {
		perror(p);
		return -1;
	}
	char mg[3];
	if (fscanf(fp, "%2s", mg) != 1 || strcmp(mg, "P6") != 0) {
		fprintf(stderr, "%s: not p6\n", p);
		fclose(fp);
		return -1;
	}
	int c;
	while ((c = fgetc(fp)) == '\n' || c == '\r' || c == ' ') {}
	if (c == '#') {
		while ((c = fgetc(fp)) != '\n' && c != EOF) {}
	} else ungetc(c, fp);
	int w, h, mv;
	if (fscanf(fp, "%d %d %d", &w, &h, &mv) != 3) {
		fprintf(stderr, "%s: bad header\n", p);
		fclose(fp);
		return -1;
	}
	if (w != W || h != H) {
		fprintf(stderr, "%s: bad size\n", p);
		fclose(fp);
		return -1;
	}
	fgetc(fp);
	for (int i = 0; i < N; i++) {
		uint8_t rgb[3];
		if (fread(rgb, 1, 3, fp) != 3) {
			fprintf(stderr, "%s: truncated\n", p);
			fclose(fp);
			return -1;
		}
		f->r[i] = rgb[0];
		f->g[i] = rgb[1];
		f->b[i] = rgb[2];
	}
	fclose(fp);
	return 0;
}

static void mkcmd(char *cmd, size_t sz, const char *ip, const char *op) {
	snprintf(cmd, sz,
		 "ffmpeg -hide_banner -loglevel error"
		 " -f image2 -i %s"
		 " -vf \"geq="
		 "r='if(eq(Y\\,1)\\,min(2*r(X\\,0)\\,255)\\,r(X\\,Y))':"
		 "g='g(X\\,Y)':"
		 "b='b(X\\,Y)':"
		 "interpolation=nearest\""
		 " -frames:v 1 -f image2 %s",
		 ip, op);
}

int main(void) {
	const char *ip = "/tmp/s1in.ppm";
	const char *op = "/tmp/s1out.ppm";
	memset(&in, 0, sizeof(in));
	for (int x = 0; x < W; x++)
		in.r[px(x,0)] = (x < 255) ? (uint8_t)(x+1) : 255;
	if (wppm(ip, &in) < 0) return 1;
	char cmd[2048];
	mkcmd(cmd, sizeof(cmd), ip, op);
	printf("%s\n", cmd);
	if (system(cmd) != 0) {
		fprintf(stderr, "ffmpeg failed\n");
		return 1;
	}
	if (rppm(op, &out) < 0) return 1;
	int err = 0, chk = 0;
	for (int x = 0; x < W; x++, chk++) {
		uint8_t e = in.r[px(x,0)], g = out.r[px(x,0)];
		if (g != e) {
			fprintf(stderr, "row0 x=%d e=%d g=%d\n", x, e, g);
			err++;
		}
	}
	for (int x = 0; x < W; x++, chk++) {
		uint8_t s = in.r[px(x,0)];
		uint8_t e = (s <= 127) ? (uint8_t)(s*2) : 255;
		uint8_t g = out.r[px(x,1)];
		if (g != e) {
			fprintf(stderr, "row1 x=%d e=%d g=%d\n", x, e, g);
			err++;
		}
	}
	for (int y = 2; y < H && err <= 10; y++)
		for (int x = 0; x < W; x++, chk++) {
			uint8_t g = out.r[px(x,y)];
			if (g) {
				fprintf(stderr, "row%d x=%d g=%d\n", y, x, g);
				err++;
			}
		}
	printf("%d pixels checked, %d errors\n", chk, err);
	if (!err) {
		printf("stage 1 complete\n");
		return 0;
	}
	return 1;
}
