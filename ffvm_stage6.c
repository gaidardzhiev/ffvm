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
#define CODEROW 128
#define DATAROW 2
#define STRIDE 255
#define IPX 32
#define SPX 33
#define FLX 35
#define OUTX 40
#define INBASE 0
#define OUTBASE 7650
#define INLEN 15809
#define OUTLEN 15808

typedef struct {
	uint16_t v[N];
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
	{"load", 32, 1}, {"store", 33, 1}, {"loadx", 34, 0}, {"storex", 35, 0},
	{"loadr", 48, 1}, {"storer", 49, 1},
};

static fr fm;

static int wpgm(const char *p, const fr *f) {
	FILE *fp = fopen(p, "wb");
	if (!fp) {
		perror(p);
		return -1;
	}
	fprintf(fp, "P5\n%d %d\n65535\n", W, H);
	for (int i = 0; i < N; i++) {
		uint8_t be[2] = { (uint8_t)(f->v[i] >> 8), (uint8_t)(f->v[i] & 0xff) };
		if (fwrite(be, 1, 2, fp) != 2) {
			perror("fwrite");
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);
	return 0;
}

static int rpgm(const char *p, fr *f) {
	FILE *fp = fopen(p, "rb");
	if (!fp) {
		perror(p);
		return -1;
	}
	char mg[3];
	if (fscanf(fp, "%2s", mg) != 1 || strcmp(mg, "P5") != 0) {
		fprintf(stderr, "%s: not p5\n", p);
		fclose(fp);
		return -1;
	}
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
		uint8_t be[2];
		if (fread(be, 1, 2, fp) != 2) {
			fprintf(stderr, "%s: truncated\n", p);
			fclose(fp);
			return -1;
		}
		f->v[i] = ((uint16_t)be[0] << 8) | be[1];
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
		"0*(st(1,lum(32,0))+st(2,lum(mod(ld(1),255),128+trunc((ld(1))/255)))+st(3,lum(mod(ld(1)+1,255),128+trunc((ld(1)+1)/255)"
		"))+st(4,lum(33,0))+st(5,lum(35,0))+st(6,lum(ld(4)-1,1))+st(7,lum(ld(4)-2,1))+st(8,if(max(max(max(eq(ld(2),1),eq(ld(2),"
		"16)),max(eq(ld(2),17),eq(ld(2),18))),max(max(eq(ld(2),48),eq(ld(2),49)),max(eq(ld(2),32),eq(ld(2),33)))),2,1)))+if(gte"
		"(Y,128)*lte(Y,191),lum(X,Y),if(eq(Y,0),if(eq(ld(2),49)*eq(X,ld(3)),ld(6),if(eq(X,32),if(eq(ld(2),16),ld(3),if(eq(ld(2)"
		",17),if(eq(ld(6),0),ld(3),ld(1)+2),if(eq(ld(2),18),if(eq(ld(6),0),ld(1)+2,ld(3)),if(eq(ld(2),0),ld(1),mod(ld(1)+ld(8),"
		"65536))))),if(eq(X,33),if(max(max(eq(ld(2),1),eq(ld(2),3)),max(eq(ld(2),48),eq(ld(2),32))),ld(4)+1,if(eq(ld(2),35),ld("
		"4)-2,if(max(max(max(max(eq(ld(2),5),eq(ld(2),6)),max(eq(ld(2),7),eq(ld(2),8))),max(max(eq(ld(2),20),eq(ld(2),21)),eq(l"
		"d(2),22))),max(max(eq(ld(2),2),eq(ld(2),49)),max(max(eq(ld(2),17),eq(ld(2),18)),max(eq(ld(2),19),eq(ld(2),33))))),ld(4"
		")-1,ld(4)))),if(eq(X,35),if(eq(ld(2),0),bitor(ld(5),1),if(eq(ld(2),8)*eq(ld(6),0),bitor(ld(5),2),ld(5))),if(eq(X,40),i"
		"f(max(eq(ld(2),19),eq(ld(2),56)),ld(6),lum(40,0)),lum(X,Y)))))),if(eq(Y,1),if(eq(ld(2),32)*eq(X,ld(4)),lum(mod(ld(3),2"
		"55),2+trunc((ld(3))/255)),if(eq(ld(2),48)*eq(X,ld(4)),lum(ld(3),0),if(eq(ld(2),1)*eq(X,ld(4)),ld(3),if(eq(ld(2),3)*eq("
		"X,ld(4)),ld(6),if(eq(ld(2),34)*eq(X,ld(4)-1),lum(mod(ld(6),255),2+trunc((ld(6))/255)),if(eq(ld(2),4)*eq(X,ld(4)-1),ld("
		"7),if(eq(ld(2),4)*eq(X,ld(4)-2),ld(6),if(eq(ld(2),5)*eq(X,ld(4)-2),mod(ld(7)+ld(6),65536),if(eq(ld(2),6)*eq(X,ld(4)-2)"
		",mod(ld(7)-ld(6)+65536,65536),if(eq(ld(2),7)*eq(X,ld(4)-2),mod(ld(7)*ld(6),65536),if(eq(ld(2),8)*eq(X,ld(4)-2),if(eq(l"
		"d(6),0),0,trunc(ld(7)/ld(6))),if(eq(ld(2),20)*eq(X,ld(4)-2),eq(ld(7),ld(6)),if(eq(ld(2),21)*eq(X,ld(4)-2),lt(ld(7),ld("
		"6)),if(eq(ld(2),22)*eq(X,ld(4)-2),gt(ld(7),ld(6)),lum(X,1))))))))))))))),if(gte(Y,2)*lte(Y,63),if(eq(ld(2),33)*lt(X,25"
		"5)*eq(((Y-2)*255+X),ld(3)),ld(6),if(eq(ld(2),35)*lt(X,255)*eq(((Y-2)*255+X),ld(6)),ld(7),lum(X,Y))),lum(X,Y)))))";
	char er[1<<13];
	esc(er, sizeof(er), re);
	snprintf(fc, cap, "geq=lum='%s'", er);
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

static int assemble(char *prog, uint16_t *code) {
	char *toks[4096];
	int nt = 0;
	char *t = strtok(prog, ",");
	while (t && nt < 4096) {
		toks[nt++] = t;
		t = strtok(NULL, ",");
	}
	char sym[512][32];
	int sad[512];
	int ns = 0;
	int pc = 0;
	for (int i = 0; i < nt; i++) {
		char nm[32], ov[32];
		int ho = split(toks[i], nm, ov);
		if (ho && ov[0] == 0) {
			if (ns < 512) {
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
			return -1;
		}
		pc += 1 + TAB[idx].imm;
	}
	pc = 0;
	for (int i = 0; i < nt; i++) {
		char nm[32], ov[32];
		int ho = split(toks[i], nm, ov);
		if (ho && ov[0] == 0) continue;
		int idx = find(nm);
		code[pc++] = (uint16_t)TAB[idx].op;
		if (TAB[idx].imm) {
			int v;
			if (TAB[idx].op >= 16 && TAB[idx].op <= 18 && !isnum(ov)) v = resolve(sym, sad, ns, ov);
			else v = atoi(ov);
			code[pc++] = (uint16_t)v;
		}
	}
	return pc;
}

static void put_code(fr *f, int a, uint16_t v) {
	f->v[(CODEROW + a/STRIDE)*W + (a%STRIDE)] = v;
}

static void put_data(fr *f, int a, uint16_t v) {
	f->v[(DATAROW + a/STRIDE)*W + (a%STRIDE)] = v;
}

static uint16_t get_data(const fr *f, int a) {
	return f->v[(DATAROW + a/STRIDE)*W + (a%STRIDE)];
}

static void usage(const char *nm) {
	fprintf(stderr, "usage: %s <program> [infile outfile]\n", nm);
	fprintf(stderr, "example: %s push:300,push:500,add,emit,halt\n", nm);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}
	char prog[1<<16];
	strncpy(prog, argv[1], sizeof(prog)-1);
	prog[sizeof(prog)-1] = 0;
	uint16_t code[16384];
	memset(code, 0, sizeof(code));
	int clen = assemble(prog, code);
	if (clen < 0) return 1;
	memset(&fm, 0, sizeof(fm));
	for (int a = 0; a < clen; a++) put_code(&fm, a, code[a]);
	int tape = (argc >= 4);
	if (tape) {
		FILE *in = fopen(argv[2], "rb");
		if (!in) {
			perror(argv[2]);
			return 1;
		}
		int c, k = 0;
		while ((c = fgetc(in)) != EOF && k < OUTBASE) put_data(&fm, INBASE + k++, (uint16_t)c);
		fclose(in);
		put_data(&fm, INLEN, (uint16_t)k);
	}
	char fc[1<<14];
	build_fc(fc, sizeof(fc));
	char pa[64], pb[64];
	snprintf(pa, sizeof(pa), "/tmp/ffvm6_%d_a.pgm", (int)getpid());
	snprintf(pb, sizeof(pb), "/tmp/ffvm6_%d_b.pgm", (int)getpid());
	if (wpgm(pa, &fm) < 0) return 1;
	const char *cur = pa;
	const char *nxt = pb;
	fr res;
	int steps = 0;
	int out = 0;
	int limit = tape ? 2000000 : 4000;
	while (steps < limit) {
		char cmd[1<<15];
		snprintf(cmd, sizeof(cmd),
			"ffmpeg -y -hide_banner -loglevel error"
			" -f image2 -i %s"
			" -vf \"%s\""
			" -pix_fmt gray16be"
			" -frames:v 1 -f image2 %s",
			cur, fc, nxt);
		if (system(cmd) != 0) {
			fprintf(stderr, "ffmpeg failed\n");
			remove(pa);
			remove(pb);
			return 1;
		}
		if (rpgm(nxt, &res) < 0) {
			remove(pa);
			remove(pb);
			return 1;
		}
		steps++;
		out = res.v[0*W + OUTX];
		if (res.v[0*W + FLX] & 1) break;
		const char *tmp = cur;
		cur = nxt;
		nxt = tmp;
	}
	remove(pa);
	remove(pb);
	if (tape) {
		FILE *of = fopen(argv[3], "wb");
		if (!of) {
			perror(argv[3]);
			return 1;
		}
		int olen = get_data(&res, OUTLEN);
		for (int k = 0; k < olen; k++) fputc(get_data(&res, OUTBASE + k) & 0xff, of);
		fclose(of);
		printf("assembled %d bytes in %d steps\n", olen, steps);
	} else {
		printf("result: %d\n", out);
	}
	return 0;
}
