/* t.h
 **
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#ifndef _T_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <getopt.h>
#include <sys/time.h>

#define FILENAMESZ  40
#define CLKNUM      10

#define BUFSIZE	12
#define IFDNUM	40
#define IFDSIZE	12

#define OUTTHDSIZE  8
#define OUTIHDSIZE  2
#define OUTIFDNUM   9
#define OUTNOFSIZE  4
#define OUTIFDSIZE  (OUTIFDNUM * IFDSIZE + OUTIHDSIZE + OUTNOFSIZE)
#define OUTBPSSIZE  6

#define TYPE_BYTE	1
#define TYPE_ASCII	2
#define TYPE_SHORT	3
#define TYPE_LONG	4

#define TAGWID	0x100
#define TAGLEN	0x101
#define TAGBPS	0x102		
#define TAGCMP	0x103
#define TAGBIZ	0x106
#define TAGSOF	0x111
#define TAGSPP	0x115
#define TAGSBC	0x117
#define TAGCFG	0x11C

typedef unsigned char   Uint8;
typedef unsigned short  Uint16;
typedef unsigned int    Uint32;

typedef Uint8		uint8_t;

typedef struct {
    struct timeval start;
    struct timeval end;
    struct timezone tz;
}   s_clock;

typedef struct {
    char *file;
    int  ifile;
    char *src;
    int isrc;
    char *dst;
    int idst;
}    s_tifbuf;

typedef struct {
    /* file info */
    int ibn;    //isBigEndian;
    int id;     //42.D or 2A.H
    int ifd;    //ifd block offset
    int nif;    //number of ifd items
    int bof;    //bitspersample offset
    int nof;    //next ifd offset
    int iof;    //1 strip image offset
    int end;    //end of file
    /* baseline tags */
    int wid;    //imgwidth = 0;    
    int len;    //imglenth = 0;
    int bps[3]; //bitspersample[3] = {0};
    int cmp;    //compression = 1;
    int biz;    //blackiszero = 1;
    int sof;    //stripoffset = 0;
    int spp;    //sampleperpixel = 0;
    int sbc;    //stripbytecnt = 0;
    int cfg;    //planarcfg = 0;
    /* misc var*/
    int nst;    //strip number
    int fsize;  //file size
    int isize;  //image size, ==wid*len*spp, ==sbc*nst 
    int rotate; //rotate angle: 0, 90, 180, 270
    int vhchanged;
    int op;     //image operation
    /* buffers */
    char fname[FILENAMESZ];    //input file name
    char bufthd[OUTTHDSIZE];   //tif head
    char bufihd[OUTIHDSIZE];   //ifd head
    char bufnof[OUTNOFSIZE];   //ifd next index offset, always 0
    char bufbps[OUTBPSSIZE];   //bits per sample array
    char bufifd[OUTIFDNUM][IFDSIZE];
    s_tifbuf buf;
    s_clock clk[CLKNUM];
}    s_tifinfo;

#if DEBUG_LOG
    #define Log(...) \
        do { \
            printf("%s %-10s [%d]:",__FILE__, __func__, __LINE__); \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } while(0)
    #define Loge(...) \
        do { \
            printf("%s %-10s [%d]:",__FILE__, __func__, __LINE__); \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } while(0)
#else
    #define Log(...)
    #define Loge(...)\
        do { \
            printf("%s %-10s [%d]:",__FILE__, __func__, __LINE__); \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } while(0)
#endif

extern char *optarg;
extern int optind, opterr, optopt;

#define _GNU_SOURCE
#include <getopt.h>

int getopt_long(int argc, char * const argv[],
           const char *optstring,
           const struct option *longopts, int *longindex);

void init(s_tifinfo *i);
void uninit(s_tifinfo *i);
int argproc(s_tifinfo *i, int argc, char *argv[]);
int loadtif(s_tifinfo *i);
int readtif(s_tifinfo *i);
int process(s_tifinfo *i);
int savetif(s_tifinfo *i);
int tiffinfo(s_tifinfo *i);
int imginfo(s_tifinfo *i);
int prtinfo(s_tifinfo *i);

Uint16 str2int16(Uint8* buf, int endian);
Uint32 str2int32(Uint8* buf, int endian);
int valadj(int typ, Uint8 *buf, int endian);
void val2str(int val, int typ, Uint8 *buf, int endian);
int tagbufpos(s_tifinfo *i, int tag);
int s2i(Uint8 *s, int cnt);

void rotate(s_tifinfo *i);
void rotate90(Uint8 *src, Uint8 *dst, int wid, int len);
void rotate180(Uint8 *src, Uint8 *dst, int wid, int len);
void rotate270(Uint8 *src, Uint8 *dst, int wid, int len);
void swap(Uint8 *a, Uint8 *b);
void hvswap(char *src, char *dst, int wid, int len);
void hswap(char *s, int wid, int len);
void vswap(char *s, int wid, int len);

void transpose(Uint8 *a, int n);
void trans90(Uint8 *a, int n);
void trans180(Uint8 *a, int n);
void trans270(Uint8 *a, int n);

#endif

