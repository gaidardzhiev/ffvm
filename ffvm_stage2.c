/*
 * Copyright (C) 2026 Ivan Gaydardzhiev
 * Licensed under the GPL-3.0-only
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define W 256
#define H 256
#define N (W*H)
#define SPX 32
#define SPY 0
#define STY 1

typedef struct {
	uint8_t r[N], g[N], b[N];
} fr;

static fr fm;

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

typedef struct {
	char  fc[1<<20];
	int   fc_len;
	int   sp;
	int   stage;
	char  cur[32];
} as;

static void as_init(as *a) {
	memset(a, 0, sizeof(*a));
	snprintf(a->cur, sizeof(a->cur), "0:v");
}

static void as_append(as *a, int tx, int ty, const char *vexpr) {
	char out[32];
	snprintf(out, sizeof(out), "s%d", a->stage++);
	char buf[4096];
	int n = snprintf(buf, sizeof(buf),
			 "[%s]geq="
			 "r='if(eq(X\\,%d)\\,if(eq(Y\\,%d)\\,%s\\,r(X\\,Y))\\,r(X\\,Y))':"
			 "g='g(X\\,Y)':b='b(X\\,Y)':interpolation=nearest[%s]",
			 a->cur, tx, ty, vexpr, out);
	if (a->fc_len > 0)
		a->fc[a->fc_len++] = ';';
	memcpy(a->fc + a->fc_len, buf, n);
	a->fc_len += n;
	a->fc[a->fc_len] = 0;
	snprintf(a->cur, sizeof(a->cur), "%s", out);
}

static void as_push(as *a, int val) {
	char vexpr[64];
	snprintf(vexpr, sizeof(vexpr), "%d", val);
	as_append(a, a->sp, STY, vexpr);
	as_append(a, SPX, SPY, "r(X\\,Y)+1");
	a->sp++;
}

static void as_pop(as *a) {
	a->sp--;
	as_append(a, SPX, SPY, "r(X\\,Y)-1");
}

static void as_dup(as *a) {
	char vexpr[64];
	snprintf(vexpr, sizeof(vexpr), "r(%d\\,%d)", a->sp-1, STY);
	as_append(a, a->sp, STY, vexpr);
	as_append(a, SPX, SPY, "r(X\\,Y)+1");
	a->sp++;
}

static void as_binop(as *a, const char *op) {
	char vexpr[128];
	snprintf(vexpr, sizeof(vexpr),
		 "r(%d\\,%d)%sr(%d\\,%d)",
		 a->sp-2, STY, op, a->sp-1, STY);
	as_append(a, a->sp-2, STY, vexpr);
	a->sp--;
	as_append(a, SPX, SPY, "r(X\\,Y)-1");
}

static void as_add(as *a) {
	as_binop(a, "+");
}

static void as_sub(as *a) {
	as_binop(a, "-");
}

static void as_mul(as *a) {
	as_binop(a, "*");
}

static void as_swap(as *a) {
	char vexpr[64];
	snprintf(vexpr, sizeof(vexpr), "r(%d\\,%d)", a->sp-1, STY);
	as_append(a, a->sp, STY, vexpr);
	snprintf(vexpr, sizeof(vexpr), "r(%d\\,%d)", a->sp-2, STY);
	as_append(a, a->sp-1, STY, vexpr);
	snprintf(vexpr, sizeof(vexpr), "r(%d\\,%d)", a->sp, STY);
	as_append(a, a->sp-2, STY, vexpr);
}

static void as_div(as *a) {
	char vexpr[128];
	snprintf(vexpr, sizeof(vexpr),
		 "trunc(r(%d\\,%d)/r(%d\\,%d))",
		 a->sp-2, STY, a->sp-1, STY);
	as_append(a, a->sp-2, STY, vexpr);
	a->sp--;
	as_append(a, SPX, SPY, "r(X\\,Y)-1");
}

static void as_emit(as *a) {
	char vexpr[64];
	snprintf(vexpr, sizeof(vexpr), "r(%d\\,%d)", a->sp-1, STY);
	as_append(a, 0, H-1, vexpr);
	as_pop(a);
}

static void usage(const char *nm) {
	fprintf(stderr, "usage: %s <program>\n", nm);
	fprintf(stderr, "example: push:3,push:4,add,emit\n");
}

int main(int argc, char **argv) {
	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}
	as a;
	as_init(&a);
	char prog[4096];
	strncpy(prog, argv[1], sizeof(prog)-1);
	char *tok = strtok(prog, ",");
	while (tok) {
		if (strncmp(tok, "push:", 5) == 0) as_push(&a, atoi(tok+5));
		else if (strcmp(tok, "add")  == 0) as_add(&a);
		else if (strcmp(tok, "sub")  == 0) as_sub(&a);
		else if (strcmp(tok, "mul")  == 0) as_mul(&a);
		else if (strcmp(tok, "div")  == 0) as_div(&a);
		else if (strcmp(tok, "dup")  == 0) as_dup(&a);
		else if (strcmp(tok, "swap") == 0) as_swap(&a);
		else if (strcmp(tok, "pop")  == 0) as_pop(&a);
		else if (strcmp(tok, "emit") == 0) as_emit(&a);
		else {
			fprintf(stderr, "unknown: %s\n", tok);
			return 1;
		}
		tok = strtok(NULL, ",");
	}
	memset(&fm, 0, sizeof(fm));
	char frm[64], out[64];
	snprintf(frm, sizeof(frm), "/tmp/ffvm2_%d_frame.ppm", (int)getpid());
	snprintf(out, sizeof(out), "/tmp/ffvm2_%d_out.ppm", (int)getpid());
	if (wppm(frm, &fm) < 0) return 1;
	char cmd[1<<21];
	snprintf(cmd, sizeof(cmd),
		 "ffmpeg -y -hide_banner -loglevel error"
		 " -f image2 -i %s"
		 " -filter_complex \"%s\""
		 " -map \"[%s]\""
		 " -frames:v 1 -f image2 %s",
		 frm, a.fc, a.cur, out);
	if (system(cmd) != 0) {
		fprintf(stderr, "ffmpeg failed\n");
		return 1;
	}
	fr res;
	if (rppm(out, &res) < 0) {
		remove(frm);
		remove(out);
		return 1;
	}
	remove(frm);
	remove(out);
	printf("result: %d\n", res.r[(H-1)*W + 0]);
	return 0;
}
