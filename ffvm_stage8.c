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
#define INCAP 3900
#define SRCCAP (1<<16)

typedef struct {
	uint16_t v[N];
} fr;

typedef struct {
	char c;
	int op;
	int arg;
} cop;

static const cop COPS[] = {
	{'h', 0, 0}, {'p', 1, 1}, {'o', 2, 0}, {'u', 3, 0}, {'s', 4, 0},
	{'+', 5, 0}, {'-', 6, 0}, {'*', 7, 0}, {'/', 8, 0},
	{'j', 16, 2}, {'z', 17, 2}, {'n', 18, 2}, {'e', 19, 0},
	{'=', 20, 0}, {'<', 21, 0}, {'>', 22, 0},
	{'l', 32, 1}, {'t', 33, 1}, {'x', 34, 0}, {'y', 35, 0},
	{'r', 48, 1}, {'w', 49, 1},
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

static int cfind(char c) {
	for (int i = 0; i < (int)(sizeof(COPS)/sizeof(COPS[0])); i++)
		if (COPS[i].c == c) return i;
	return -1;
}

static int ws(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int cassemble(const char *src, uint16_t *code) {
	int labad[26];
	for (int i = 0; i < 26; i++) labad[i] = -1;
	int pc = 0;
	int i = 0;
	while (src[i]) {
		char c = src[i];
		if (ws(c)) {
			i++;
			continue;
		}
		if (c == ':') {
			i++;
			if (src[i] >= 'A' && src[i] <= 'Z') {
				labad[src[i]-'A'] = pc;
				i++;
			}
			continue;
		}
		int k = cfind(c);
		if (k < 0) {
			fprintf(stderr, "bad char: %c\n", c);
			return -1;
		}
		i++;
		pc += 1;
		if (COPS[k].arg == 1) {
			while (src[i] >= '0' && src[i] <= '9') i++;
			pc += 1;
		} else if (COPS[k].arg == 2) {
			if (src[i] >= 'A' && src[i] <= 'Z') i++;
			pc += 1;
		}
	}
	pc = 0;
	i = 0;
	while (src[i]) {
		char c = src[i];
		if (ws(c)) {
			i++;
			continue;
		}
		if (c == ':') {
			i++;
			if (src[i] >= 'A' && src[i] <= 'Z') i++;
			continue;
		}
		int k = cfind(c);
		i++;
		code[pc++] = (uint16_t)COPS[k].op;
		if (COPS[k].arg == 1) {
			int v = 0;
			while (src[i] >= '0' && src[i] <= '9') {
				v = v*10 + (src[i]-'0');
				i++;
			}
			code[pc++] = (uint16_t)v;
		} else if (COPS[k].arg == 2) {
			int v = 0;
			if (src[i] >= 'A' && src[i] <= 'Z') {
				v = labad[src[i]-'A'];
				if (v < 0) {
					fprintf(stderr, "undefined label: %c\n", src[i]);
					return -1;
				}
				i++;
			}
			code[pc++] = (uint16_t)v;
		}
	}
	return pc;
}

static int rdfile(const char *p, char *buf, int cap) {
	FILE *f = fopen(p, "rb");
	if (!f) {
		perror(p);
		return -1;
	}
	int n = (int)fread(buf, 1, cap, f);
	fclose(f);
	buf[n] = 0;
	return n;
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

static int refmode(const char *sp, const char *op) {
	static char src[SRCCAP];
	int n = rdfile(sp, src, SRCCAP-1);
	if (n < 0) return 1;
	static uint16_t code[16384];
	int clen = cassemble(src, code);
	if (clen < 0) return 1;
	FILE *f = fopen(op, "wb");
	if (!f) {
		perror(op);
		return 1;
	}
	for (int i = 0; i < clen; i++) {
		fputc((code[i] >> 8) & 0xff, f);
		fputc(code[i] & 0xff, f);
	}
	fclose(f);
	printf("reference: %d bytes\n", clen*2);
	return 0;
}

static void usage(const char *nm) {
	fprintf(stderr, "usage: %s <assembler.ffvm> <source> <output>\n", nm);
	fprintf(stderr, "       %s -r <source> <output>\n", nm);
}

int main(int argc, char **argv) {
	if (argc < 4) {
		usage(argv[0]);
		return 1;
	}
	if (strcmp(argv[1], "-r") == 0) return refmode(argv[2], argv[3]);
	static char asmsrc[SRCCAP];
	if (rdfile(argv[1], asmsrc, SRCCAP-1) < 0) return 1;
	static uint16_t code[16384];
	memset(code, 0, sizeof(code));
	int clen = cassemble(asmsrc, code);
	if (clen < 0) return 1;
	memset(&fm, 0, sizeof(fm));
	for (int a = 0; a < clen; a++) put_code(&fm, a, code[a]);
	static char src[SRCCAP];
	int slen = rdfile(argv[2], src, SRCCAP-1);
	if (slen < 0) return 1;
	if (slen > INCAP) {
		fprintf(stderr, "source too long: %d > %d\n", slen, INCAP);
		return 1;
	}
	for (int k = 0; k < slen; k++) put_data(&fm, INBASE + k, (uint16_t)(unsigned char)src[k]);
	put_data(&fm, INLEN, (uint16_t)slen);
	char fc[1<<14];
	build_fc(fc, sizeof(fc));
	char pa[64], pb[64];
	snprintf(pa, sizeof(pa), "/tmp/ffvm8_%d_a.pgm", (int)getpid());
	snprintf(pb, sizeof(pb), "/tmp/ffvm8_%d_b.pgm", (int)getpid());
	if (wpgm(pa, &fm) < 0) return 1;
	const char *cur = pa;
	const char *nxt = pb;
	fr res;
	int steps = 0;
	while (steps < 2000000) {
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
		if (steps % 1000 == 0) fprintf(stderr, "step %d\n", steps);
		if (res.v[0*W + FLX] & 1) break;
		const char *tmp = cur;
		cur = nxt;
		nxt = tmp;
	}
	remove(pa);
	remove(pb);
	FILE *of = fopen(argv[3], "wb");
	if (!of) {
		perror(argv[3]);
		return 1;
	}
	int olen = get_data(&res, OUTLEN);
	for (int k = 0; k < olen; k++) fputc(get_data(&res, OUTBASE + k) & 0xff, of);
	fclose(of);
	printf("assembled %d bytes in %d steps\n", olen, steps);
	return 0;
}
