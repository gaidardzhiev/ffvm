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
#define RY 192
#define STY 1
#define IPX 32
#define SPX 33
#define FLX 35
#define OUTX 40

typedef struct {
	uint8_t r[N], g[N], b[N];
} fr;

typedef struct {
	const char *nm;
	int op;
	int imm;
} insn;

static const insn TAB[] = {
	{"halt", 0, 0}, {"push", 1, 1}, {"pop", 2, 0}, {"dup", 3, 0}, {"swap", 4, 0},
	{"add", 5, 0}, {"sub", 6, 0}, {"mul", 7, 0}, {"div", 8, 0},
	{"jmp", 16, 1}, {"jz", 17, 1}, {"jnz", 18, 1}, {"emit", 19, 0},
	{"eq", 20, 0}, {"lt", 21, 0}, {"gt", 22, 0},
	{"loadr", 48, 1}, {"storer", 49, 1},
};

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

static void esc(char *d, size_t cap, const char *s) {
	size_t j = 0;
	for (size_t i = 0; s[i] && j+2 < cap; i++) {
		if (s[i] == ',') {
			d[j++] = '\\';
			d[j++] = ',';
		} else d[j++] = s[i];
	}
	d[j] = 0;
}

static void build_fc(char *fc, size_t cap) {
	static const char *re =
		"0*(st(1,r(32,0))+st(2,r(ld(1),192))+st(3,r(ld(1)+1,192))+st(4,r(33,0))+st(5,r(35,0))+st(6,r(ld(4)-1,1))+st(7,r(ld(4)-2,1"
		"))+st(8,if(max(max(max(eq(ld(2),1),eq(ld(2),16)),max(eq(ld(2),17),eq(ld(2),18))),max(eq(ld(2),48),eq(ld(2),49))),2,1)))+"
		"if(gte(Y,192),r(X,Y),if(eq(Y,0),if(eq(ld(2),49)*eq(X,ld(3)),ld(6),if(eq(X,32),if(eq(ld(2),16),ld(3),if(eq(ld(2),17),if(e"
		"q(ld(6),0),ld(3),ld(1)+2),if(eq(ld(2),18),if(eq(ld(6),0),ld(1)+2,ld(3)),if(eq(ld(2),0),ld(1),mod(ld(1)+ld(8),256))))),if"
		"(eq(X,33),if(max(max(eq(ld(2),1),eq(ld(2),3)),eq(ld(2),48)),ld(4)+1,if(max(max(max(eq(ld(2),5),eq(ld(2),6)),max(eq(ld(2)"
		",7),eq(ld(2),8))),max(max(eq(ld(2),20),eq(ld(2),21)),eq(ld(2),22))),ld(4)-1,if(max(max(eq(ld(2),2),eq(ld(2),49)),max(eq("
		"ld(2),17),max(eq(ld(2),18),eq(ld(2),19)))),ld(4)-1,ld(4)))),if(eq(X,35),if(eq(ld(2),0),bitor(ld(5),1),if(eq(ld(2),8)*eq("
		"ld(6),0),bitor(ld(5),2),ld(5))),if(eq(X,40),if(eq(ld(2),19),ld(6),r(40,0)),r(X,Y)))))),if(eq(Y,1),if(eq(ld(2),48)*eq(X,l"
		"d(4)),r(ld(3),0),if(eq(ld(2),1)*eq(X,ld(4)),ld(3),if(eq(ld(2),3)*eq(X,ld(4)),ld(6),if(eq(ld(2),4)*eq(X,ld(4)-1),ld(7),if"
		"(eq(ld(2),4)*eq(X,ld(4)-2),ld(6),if(eq(ld(2),5)*eq(X,ld(4)-2),mod(ld(7)+ld(6),256),if(eq(ld(2),6)*eq(X,ld(4)-2),mod(ld(7"
		")-ld(6)+256,256),if(eq(ld(2),7)*eq(X,ld(4)-2),mod(ld(7)*ld(6),256),if(eq(ld(2),8)*eq(X,ld(4)-2),if(eq(ld(6),0),0,trunc(l"
		"d(7)/ld(6))),if(eq(ld(2),20)*eq(X,ld(4)-2),eq(ld(7),ld(6)),if(eq(ld(2),21)*eq(X,ld(4)-2),lt(ld(7),ld(6)),if(eq(ld(2),22)"
		"*eq(X,ld(4)-2),gt(ld(7),ld(6)),r(X,1))))))))))))),r(X,Y))))";
	char er[1<<13];
	esc(er, sizeof(er), re);
	snprintf(fc, cap,
		"[0:v]geq=r='%s':g='g(X\\,Y)':b='b(X\\,Y)':interpolation=nearest[o]",
		er);
}

static int find(const char *nm) {
	for (int i = 0; i < (int)(sizeof(TAB)/sizeof(TAB[0])); i++)
		if (strcmp(TAB[i].nm, nm) == 0) return i;
	return -1;
}

static int split(const char *tok, char *nm, char *ov) {
	const char *c = strchr(tok, ':');
	if (!c) {
		strncpy(nm, tok, 31);
		nm[31] = 0;
		ov[0] = 0;
		return 0;
	}
	int n = c-tok;
	if (n > 31) n = 31;
	memcpy(nm, tok, n);
	nm[n] = 0;
	strncpy(ov, c+1, 31);
	ov[31] = 0;
	return 1;
}

static int isnum(const char *s) {
	if (!s[0]) return 0;
	for (int i = 0; s[i]; i++)
		if (s[i] < '0' || s[i] > '9') return 0;
	return 1;
}

static int resolve(char sym[][32], int *sad, int ns, const char *nm) {
	for (int i = 0; i < ns; i++)
		if (strcmp(sym[i], nm) == 0) return sad[i];
	fprintf(stderr, "undefined label: %s\n", nm);
	return 0;
}

static void usage(const char *nm) {
	fprintf(stderr, "usage: %s <program>\n", nm);
	fprintf(stderr, "example: push:3,push:4,add,emit,halt\n");
}

int main(int argc, char **argv) {
	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}
	char prog[4096];
	strncpy(prog, argv[1], sizeof(prog)-1);
	prog[sizeof(prog)-1] = 0;
	char *toks[512];
	int nt = 0;
	char *t = strtok(prog, ",");
	while (t && nt < 512) {
		toks[nt++] = t;
		t = strtok(NULL, ",");
	}
	char sym[128][32];
	int sad[128];
	int ns = 0;
	int pc = 0;
	for (int i = 0; i < nt; i++) {
		char nm[32], ov[32];
		int ho = split(toks[i], nm, ov);
		if (ho && ov[0] == 0) {
			if (ns < 128) {
				strncpy(sym[ns], nm, 31);
				sym[ns][31] = 0;
				sad[ns] = pc;
				ns++;
			}
			continue;
		}
		int idx = find(nm);
		if (idx < 0) {
			fprintf(stderr, "unknown: %s\n", toks[i]);
			return 1;
		}
		pc += 1 + TAB[idx].imm;
	}
	uint8_t rom[256];
	memset(rom, 0, sizeof(rom));
	pc = 0;
	for (int i = 0; i < nt; i++) {
		char nm[32], ov[32];
		int ho = split(toks[i], nm, ov);
		if (ho && ov[0] == 0) continue;
		int idx = find(nm);
		rom[pc++] = (uint8_t)TAB[idx].op;
		if (TAB[idx].imm) {
			int v;
			if (TAB[idx].op >= 16 && TAB[idx].op <= 18 && !isnum(ov)) v = resolve(sym, sad, ns, ov);
			else v = atoi(ov);
			rom[pc++] = (uint8_t)v;
		}
	}
	memset(&fm, 0, sizeof(fm));
	for (int i = 0; i < 256; i++) fm.r[RY*W + i] = rom[i];
	char fc[1<<14];
	build_fc(fc, sizeof(fc));
	char pa[64], pb[64];
	snprintf(pa, sizeof(pa), "/tmp/ffvm3_%d_a.ppm", (int)getpid());
	snprintf(pb, sizeof(pb), "/tmp/ffvm3_%d_b.ppm", (int)getpid());
	if (wppm(pa, &fm) < 0) return 1;
	const char *cur = pa;
	const char *nxt = pb;
	fr res;
	int steps = 0;
	int out = 0;
	while (steps < 512) {
		char cmd[1<<15];
		snprintf(cmd, sizeof(cmd),
			"ffmpeg -y -hide_banner -loglevel error"
			" -f image2 -i %s"
			" -filter_complex \"%s\""
			" -map \"[o]\""
			" -frames:v 1 -f image2 %s",
			cur, fc, nxt);
		if (system(cmd) != 0) {
			fprintf(stderr, "ffmpeg failed\n");
			remove(pa);
			remove(pb);
			return 1;
		}
		if (rppm(nxt, &res) < 0) {
			remove(pa);
			remove(pb);
			return 1;
		}
		steps++;
		out = res.r[0*W + OUTX];
		if (res.r[0*W + FLX] & 1) break;
		const char *tmp = cur;
		cur = nxt;
		nxt = tmp;
	}
	remove(pa);
	remove(pb);
	printf("result: %d\n", out);
	return 0;
}
