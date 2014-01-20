/* 1.c 
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

//#define DEBUG_TAGINFO     1
//#define DEBUG_LOG   1
/* output tiff block order: 0: head-img-ifd-bps; 1: head-ifd-bps-img */
#define OUTORDER    0    
#define PERFORMANCE_PROFILING   1

#include "t.h"

int main(int argc, char *argv[])
{
    int ret = 0;
    s_tifinfo inf;

    init(&inf);
    
    ret = argproc(&inf, argc, argv);
    if (ret < 0) goto finish;

    ret = loadtif(&inf);
    if (ret < 0) goto finish;

    ret = readtif(&inf);
    if (ret < 0) goto finish;

    ret = process(&inf);
    if (ret < 0) goto finish;

    ret = savetif(&inf);
    if (ret < 0) goto finish;

    
finish:
    Log("Finishing...");
    uninit(&inf);
   
    return 0;
}

void init(s_tifinfo *i){    
    char defaultname[] = "res/lena_color_512.tif";
    //char defaultname[] = "res/jupiter.tif";

    memset(i, 0, sizeof(s_tifinfo));
    Log("%d bytes cleared", sizeof(s_tifinfo));
    
    strncpy(i->fname, defaultname, sizeof(defaultname));
    i->ibn = -1;
    i->cmp = 1;
    i->biz = 1;
}

void uninit(s_tifinfo *i) {
    if (i->buf.file != NULL) {
        Log("file: %x", i->buf.file);
        free(i->buf.file);
        i->buf.file = NULL;
    }
    if (i->buf.src != NULL) {
        Log("src: %x", i->buf.src);
        free(i->buf.src);
        i->buf.src = NULL;
    }
    if (i->buf.dst != NULL) {
        Log("dst: %x", i->buf.dst);
        free(i->buf.dst);
        i->buf.dst = NULL;
    }
}

int argproc(s_tifinfo *i, int argc, char *argv[]) {
    int ret = 0;
    int c;
    int digit_optind = 0;
    
    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
            {"add", 1, 0, 0},
            {"append", 0, 0, 0},
            {"delete", 1, 0, 0},
            {"verbose", 0, 0, 0},
            {"create", 1, 0, 'c'},
            {"file", 1, 0, 0},
            {0, 0, 0, 0}
        };
    
        c = getopt_long (argc, argv, "abc:d:012i:r:",
                 long_options, &option_index);
        if (c == -1)
            break;
    
        switch (c) {
        case 0:
            printf ("option %s", long_options[option_index].name);
            if (optarg)
                printf (" with arg %s", optarg);
            printf ("\n");
            break;
    
        case '0':
        case '1':
        case '2':
            if (digit_optind != 0 && digit_optind != this_option_optind)
                printf ("digits occur in two different argv-elements.\n");
            digit_optind = this_option_optind;
            printf ("option %c\n", c);
            break;
    
        case 'a':
            printf ("option a\n");
            break;
    
        case 'b':
            printf ("option b\n");
            break;
    
        case 'c':
            printf ("option c with value '%s'\n", optarg);
            break;
    
        case 'd':
            printf ("option d with value '%s'\n", optarg);
            break;
    
        case '?':
            break;

        case 'i':
            printf ("option %c with value '%s' len %d\n", c, optarg, strlen(optarg));
            if (strlen(optarg) <  FILENAMESZ) {
                memset(i->fname, 0, FILENAMESZ);
                strncpy(i->fname, optarg, strlen(optarg));
            } else {
                Loge("file name length over limit: %d", FILENAMESZ);
            }
            break;

        case 'r':
            printf ("option %c with value '%s' len %d\n", c, optarg, strlen(optarg));
            char tmpbuf[4];
            strncpy(tmpbuf, optarg, 4);
            i->rotate = s2i(tmpbuf, 4);
            i->rotate %= 360;
            if(i->rotate % 90) {
                Loge("rotate: %d, round to 0", i->rotate); 
                i->rotate = 0;
            }
            break;
    
        default:
            printf ("?? getopt returned character code 0%o ??\n", c);
        }
    }
    
    if (optind < argc) {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        printf ("\n");
    }

    return ret;
}

int loadtif(s_tifinfo *i){
    int ret = 0;
    FILE *fp = NULL;
    if ((fp = fopen(i->fname, "rb")) == NULL){
        fprintf (stderr, "Input file '%s' does not exist !!\n", i->fname);
        ret = -1;
        goto end;
    } else {
        fseek(fp, 0L, SEEK_END);
        i->fsize = ftell(fp);
        rewind(fp);
        Log("Open inputfile '%s' [%ld bytes] succeed.", i->fname, i->fsize);
    }
    
    if ((i->buf.file = calloc(sizeof(char),i->fsize)) == NULL) {ret = -1; goto end;}
    ret = fread(i->buf.file, sizeof(char), i->fsize, fp);
    Log("read: %x, filesize: %x", ret, i->fsize);
    if (ret != i->fsize) {
        Loge("ferror %x feof %x", ferror(fp), feof(fp));
        ret = -1;
    }
end:
    fclose(fp);
    return ret;
}

int readtif(s_tifinfo *i) {
    int tmp = 0;
    if (!i || !(i->buf.file)) {
        Loge("Invalid buffer");
        return -1;
    }
    char *buf = i->buf.file;
    i->buf.ifile = 0;
    if (*buf == 'I' && *(buf+1) == 'I') {//0x4949
        Log("little endian");
        i->ibn = 0;
    } else if (*buf == 'M' && *(buf+1) == 'M') {//0x4D4D
        Log("big endian");
        i->ibn = 1;
    }
    if (i->ibn < 0) {
        Loge("Invalid endian");
        return -1;
    }
    memcpy(i->bufthd, buf, 4);
    
    i->buf.ifile += 2;
    buf = i->buf.file + i->buf.ifile;
    i->id = str2int16(buf, i->ibn); 
    if (i->id != 42) {
        Loge("Invalid id");
        return -1;
    }

    i->buf.ifile += 2;
    buf = i->buf.file + i->buf.ifile;
    i->ifd = str2int32(buf, i->ibn); 

    i->buf.ifile = i->ifd;
    buf = i->buf.file + i->buf.ifile;
    i->nif = str2int16(buf, i->ibn);
    if (i->nif > IFDNUM) {
        Loge("Invalid ifd item number");
        return -1;
    }

    i->buf.ifile += 2;
    buf = i->buf.file + i->buf.ifile;
    for (tmp = 0; tmp < i->nif; tmp++) {
        tiffinfo(i);
        i->buf.ifile += IFDSIZE;
    }
    
    buf = i->buf.file + i->buf.ifile;
    i->nof = str2int32(buf, i->ibn);
    if (i->nof != 0) {
        Loge("Invalid next ifd offset");
        return -1;
    }
   
    tmp = i->wid * i->len * i->spp;
    i->isize = i->nst * i->sbc;
    if (i->isize < tmp) {
        if (i->cmp == 32773) {
            Loge("Warning: Packbits compression");
        } else {
            Loge("Error: image size not match: %x %x", i->isize, tmp);
            return -1;
        }
    }

    tmp = prtinfo(i);
    if (tmp != 0) {
        Loge("prtinfo error");
        return -1;
    }

    if ((i->buf.src = calloc(sizeof(char),i->isize)) == NULL) {return -1;}    
    if ((i->buf.dst = calloc(sizeof(char),i->isize)) == NULL) {return -1;}

    tmp = imginfo(i);
    if (tmp != 0) {
        Loge("read image data error");
        return -1;
    }

    return 0;
}

int process(s_tifinfo *i){
    int ret = 0;
    int ii, jj;
    char *sc = NULL;
    char *dc = NULL;
    int n = i->spp;
    int compsize = i->wid * i->len;
    if ((sc = calloc(sizeof(char),compsize)) == NULL) {ret = -1; goto err1;} 
    if ((dc = calloc(sizeof(char),compsize)) == NULL) {ret = -1; goto err2;}    

    /* peformance profiling start */
#if PERFORMANCE_PROFILING
    gettimeofday(&(i->clk[0].start), &(i->clk[0].tz));
#endif
    
    for (ii = 0; ii < n; ii++) {
        for (jj = 0; jj < compsize; jj++) {
            sc[jj] = i->buf.src[jj*n + ii];        
        }                

        switch (i->rotate) {
            case 90:
                rotate90(sc, dc, i->wid, i->len); 
                i->vhchanged = 1;
                break;
            case 180:
                rotate180(sc, dc, i->wid, i->len);
                i->vhchanged = 0;
                break;
            case 270:
                rotate270(sc, dc, i->wid, i->len);
                i->vhchanged = 1;
                break;
            default:
                memcpy(dc, sc, compsize);
                i->vhchanged = 0;
                break;
        }

        for (jj = 0; jj < compsize; jj++) {            
            i->buf.dst[jj*n + ii] = dc[jj];        
        }    
    } 

    /* peformance profiling end */
#if PERFORMANCE_PROFILING
    gettimeofday(&(i->clk[0].end), &(i->clk[0].tz));
    {
        long start = i->clk[0].start.tv_sec * 1000000 + i->clk[0].start.tv_usec;
        long end   = i->clk[0].end.tv_sec * 1000000   + i->clk[0].end.tv_usec;;
        long cpu_time_used = end - start;
        Loge("%dx%d, %s pattern process: cputime = [%d] us = [%.3f] ms", \
            i->wid, i->len, (i->spp == 1)?"Grey":"Color", cpu_time_used, (double)cpu_time_used/1000.0);
    }
#endif

    if (i->vhchanged == 1) {          
        int itw = tagbufpos(i, TAGWID);
        int itl = tagbufpos(i, TAGLEN);
        int cnt = 0;
        for (cnt = 0; cnt < 4; cnt++)
            swap(i->bufifd[itl] + 8 + cnt, i->bufifd[itw] + 8 + cnt);
    }
    
    free(sc);
err2:
    free(dc);
err1:

    return ret;
}

int savetif(s_tifinfo *i) {
    int ret = 0;
    char *serialFileName = "out.tiff";
    char *buf = NULL;
    FILE *fpout = NULL;
    if ((fpout = fopen(serialFileName, "wb+")) == NULL){
        fprintf (stderr, "Open file '%s' fail !!\n", serialFileName);
        ret = -1;
        goto end;
    } else {
        Log("Open file '%s' succeed.", serialFileName);
    }

#if OUTORDER
    //head-ifd-bps-img    
    int idx, cnt;
    i->ifd = OUTTHDSIZE;
    val2str(i->ifd, TYPE_LONG, i->bufthd + 4, i->ibn);

    //tiff head, OUTIHDSIZE = 8 bytes
    ret = fwrite(i->bufthd, sizeof(char), OUTTHDSIZE, fpout);
    
    //ifd head,  OUTIHDSIZE = 2 bytes
    i->nif = OUTIFDNUM;
    val2str(i->nif, TYPE_SHORT, i->bufihd, i->ibn);
    ret = fwrite(i->bufihd, sizeof(char), OUTIHDSIZE, fpout);
    
    //ifd block, OUTIFDNUM * IFDSIZE = 9 * 12 bytes; recalculate bps offset and strip offset    
    i->bof = i->ifd + OUTIFDSIZE;

    idx = tagbufpos(i, TAGBPS);
    cnt = str2int32(i->bufifd[idx] + 4, i->ibn);
    if (cnt == 1) {
        val2str(TYPE_LONG, TYPE_SHORT, i->bufifd[idx] + 2, i->ibn); //here is different when order changed
        val2str(i->bps[0], TYPE_LONG,  i->bufifd[idx] + 8, i->ibn);
        i->iof = i->bof;
    }
    else if (cnt == 3) {
        val2str(TYPE_LONG, TYPE_SHORT, i->bufifd[idx] + 2, i->ibn); //here is different when order changed
        val2str(i->bof,    TYPE_LONG,  i->bufifd[idx] + 8, i->ibn);
        i->iof = i->bof + OUTBPSSIZE;
    }
    
    idx = tagbufpos(i, TAGSOF);
    val2str(1,      TYPE_LONG,                             i->bufifd[idx] + 4, i->ibn);
    val2str(i->iof, str2int16(i->bufifd[idx] + 2, i->ibn), i->bufifd[idx] + 8, i->ibn);
    
    idx = tagbufpos(i, TAGSBC);
    val2str(1,        TYPE_LONG,                             i->bufifd[idx] + 4, i->ibn);
    val2str(i->isize, str2int16(i->bufifd[idx] + 2, i->ibn), i->bufifd[idx] + 8, i->ibn);
        
    ret = fwrite(i->bufifd, sizeof(char), OUTIFDNUM * IFDSIZE, fpout);

    //ifd next index offset, OUTNOFSIZE bytes
    ret = fwrite(i->bufnof, sizeof(char), OUTNOFSIZE, fpout);

    //bps block, 0 or 6 bytes
    ret = fwrite(i->bufbps, sizeof(char), i->iof - i->bof, fpout); 

    //img block, i->isize bytes
    ret = fwrite(i->buf.dst, sizeof(char), i->isize, fpout);
#else
    //head-img-ifd-bps
    i->iof = OUTTHDSIZE;
    i->ifd = i->iof + i->isize;

    int idx, cnt;
    val2str(i->ifd, TYPE_LONG, i->bufthd + 4, i->ibn);

    //tiff head, OUTIHDSIZE = 8 bytes
    ret = fwrite(i->bufthd, sizeof(char), OUTTHDSIZE, fpout);

    //img block, i->isize bytes
    ret = fwrite(i->buf.dst, sizeof(char), i->isize, fpout);
   
    //ifd head,  OUTIHDSIZE = 2 bytes
    i->nif = OUTIFDNUM;
    val2str(i->nif, TYPE_SHORT, i->bufihd, i->ibn);
    ret = fwrite(i->bufihd, sizeof(char), OUTIHDSIZE, fpout);
    
    //ifd block, OUTIFDNUM * IFDSIZE = 9 * 12 bytes; recalculate bps offset and strip offset    
    i->bof = i->ifd + OUTIFDSIZE;

    idx = tagbufpos(i, TAGBPS);
    cnt = str2int32(i->bufifd[idx] + 4, i->ibn);
    if (cnt == 1) {
        val2str(TYPE_SHORT, TYPE_SHORT, i->bufifd[idx] + 2, i->ibn); //here is different when order changed
        val2str(i->bps[0],  TYPE_LONG,  i->bufifd[idx] + 8, i->ibn);
        i->end = i->bof;
    }
    else if (cnt == 3) {
        val2str(TYPE_SHORT, TYPE_SHORT, i->bufifd[idx] + 2, i->ibn); //here is different when order changed
        val2str(i->bof,     TYPE_LONG,  i->bufifd[idx] + 8, i->ibn);
        i->end = i->bof + OUTBPSSIZE;
    }
    
    idx = tagbufpos(i, TAGSOF);
    val2str(1,      TYPE_LONG,                             i->bufifd[idx] + 4, i->ibn);
    val2str(i->iof, str2int16(i->bufifd[idx] + 2, i->ibn), i->bufifd[idx] + 8, i->ibn);    

    idx = tagbufpos(i, TAGSBC);
    val2str(1,        TYPE_LONG,                             i->bufifd[idx] + 4, i->ibn);
    val2str(i->isize, str2int16(i->bufifd[idx] + 2, i->ibn), i->bufifd[idx] + 8, i->ibn);       

    ret = fwrite(i->bufifd, sizeof(char), OUTIFDNUM * IFDSIZE, fpout);
    
    //ifd next index offset, OUTNOFSIZE bytes
    ret = fwrite(i->bufnof, sizeof(char), OUTNOFSIZE, fpout);

    //bps block, 0 or 6 bytes
    ret = fwrite(i->bufbps, sizeof(char), i->end - i->bof, fpout); 

#endif
    
end:
    fclose(fpout);
    return ret;
}

int tiffinfo(s_tifinfo *i){
    char *buf = i->buf.file + i->buf.ifile;
    int endian = i->ibn;
    int tag = str2int16(buf, endian);
    int typ = str2int16(buf+2, endian);
    int cnt = str2int32(buf+4, endian);
    int val = valadj(typ, buf+8, endian);
#if DEBUG_TAGINFO
    Loge("  TAG:        %8d[0x%8x]", tag, tag); 
    Loge("  Field type: %8d[0x%8x]", typ, typ); 
    Loge("  Count:      %8d[0x%8x]", cnt, cnt); 
    Loge("  Value:      %8d[0x%8x]", val, val); 
#endif

    switch (tag) {
        case TAGWID:    i->wid = val; break; 
        case TAGLEN:    i->len = val; break; 
        case TAGBPS: 
            val = str2int32(buf+8, endian);
            if (cnt == 1)
                i->bps[0] = val; 
            else if (cnt == 3) {
                i->bof = val;
                i->bps[0] = str2int16(i->buf.file+val, endian); 
                i->bps[1] = str2int16(i->buf.file+val+2, endian); 
                i->bps[2] = str2int16(i->buf.file+val+4, endian); 
                memcpy(i->bufbps, i->buf.file+val, 6);
            }
            break; 
        case TAGCMP:    i->cmp = val; break;
        case TAGBIZ:    i->biz = val; break;
        case TAGSOF:
            if (cnt == 1)
                i->sof = val;
            else
                i->sof = str2int32(i->buf.file + val, endian); 
            break;
        case TAGSPP:    i->spp = val; break; 
        case TAGSBC:
            i->nst = cnt; 
            if (i->nst == 1)
                i->sbc = val;
            else
                i->sbc = str2int32(i->buf.file + val, endian); 
            break; 
        case TAGCFG:    i->cfg = valadj(typ, buf+8, endian); break; 
    }
    
    switch (tag) {
        case TAGWID:    memcpy(i->bufifd[0], buf, IFDSIZE); break; 
        case TAGLEN:    memcpy(i->bufifd[1], buf, IFDSIZE); break; 
        case TAGBPS:    memcpy(i->bufifd[2], buf, IFDSIZE); break;
        case TAGCMP:    memcpy(i->bufifd[3], buf, IFDSIZE); break;
        case TAGBIZ:    memcpy(i->bufifd[4], buf, IFDSIZE); break;
        case TAGSOF:    memcpy(i->bufifd[5], buf, IFDSIZE); break;
        case TAGSPP:    memcpy(i->bufifd[6], buf, IFDSIZE); break;
        case TAGSBC:    memcpy(i->bufifd[7], buf, IFDSIZE); break;
        case TAGCFG:    memcpy(i->bufifd[8], buf, IFDSIZE); break;
    }
    return 0;
}

int imginfo(s_tifinfo *i) {
    if (i->cmp == 1) {
        memcpy(i->buf.src, i->buf.file + i->sof, i->isize);
    } else if (i->cmp == 32773) {
        Loge("Packetbits compression!");
        return -1;
    }
    return 0;
}

int prtinfo(s_tifinfo *i) {  
    Log("  imgwidth:        %8d", i->wid); 
    Log("  imglenth:        %8d", i->len); 
    Log("  bps offset:      %8d", i->bof);
    Log("  bitspersample: [%2d %2d %2d]", i->bps[0], i->bps[1], i->bps[2]); 
    Log("  compression:     %8d", i->cmp); 
    Log("  blackiszero:     %8d", i->biz); 
    Log("  stripoffset:     %8d", i->sof); 
    Log("  sampleperpixel:  %8d", i->spp); 
    Log("  numberofstrip:   %8d", i->nst); 
    Log("  stripbytecnt:    %8d", i->sbc); 
    Log("  planarcfg:       %8d", i->cfg); 
    Log("  image size:      %8d", i->isize); 
    return 0;
}

Uint16 str2int16(Uint8* buf, int endian){
    if (endian)
        return (((Uint16)buf[0] << 8) + buf[1]); 
    else
        return (((Uint16)buf[1] << 8) + buf[0]); 
}

Uint32 str2int32(Uint8* buf, int endian){
    if (endian)
        return (((((((Uint32)buf[0] << 8) + (Uint32)buf[1]) << 8) + (Uint32)buf[2]) << 8) + (Uint32)buf[3]);
    else
        return (((((((Uint32)buf[3] << 8) + (Uint32)buf[2]) << 8) + (Uint32)buf[1]) << 8) + (Uint32)buf[0]);
}

int valadj(int typ, Uint8 *buf, int endian) {
    int val;
    switch (typ) {
        case TYPE_BYTE:
        case TYPE_ASCII:    val = (int)buf[0]; break;
        case TYPE_SHORT:    val = str2int16(buf, endian); break;
        case TYPE_LONG:     val = str2int32(buf, endian); break;
        default:            Log("Not supported type 0x%x", typ); break;
    }
    return val;
}

void val2str(int val, int typ, Uint8 *buf, int endian) {
    switch (typ) {
        case TYPE_BYTE:
        case TYPE_ASCII:    *buf = val; break;
        case TYPE_SHORT:    
            if (endian) {
                buf[0] = val >> 8;
                buf[1] = val & 0xFF;
            } else {
                buf[1] = val >> 8;
                buf[0] = val & 0xFF;
            }
            break;
        case TYPE_LONG: 
            if (endian) {
                buf[0] = val >> 24;
                buf[1] = (val >> 16) & 0xFF;
                buf[2] = (val >> 8)  & 0xFF;
                buf[3] = val & 0xFF;
            } else {
                buf[3] = val >> 24;
                buf[2] = (val >> 16) & 0xFF;
                buf[1] = (val >> 8)  & 0xFF;
                buf[0] = val & 0xFF;
            }
            break;
        default: Log("Not supported type 0x%x", typ); break;
    }
}

int tagbufpos(s_tifinfo *i, int tag){
    int cnt, tmp;
    for (cnt = 0; cnt < i->nif; cnt++) {
        tmp = str2int16((i->bufifd)[cnt], i->ibn);
        if (tmp == tag)
            return cnt;
    }
    return -1;
}

void rotate(s_tifinfo *i) {

}

void rotate90(Uint8 *src, Uint8 *dst, int wid, int len) {
    hvswap(src, dst, wid, len);
    vswap(dst, len, wid);
}

void rotate180(Uint8 *src, Uint8 *dst, int wid, int len) {
    memcpy(dst, src, wid*len);
    hswap(dst, wid, len);
    vswap(dst, wid, len);
}

void rotate270(Uint8 *src, Uint8 *dst, int wid, int len) {
    hvswap(src, dst, wid, len);
    hswap(dst, len, wid);
}

void swap(Uint8 *a, Uint8 *b){
    Uint8 t = *a;
    *a = *b;
    *b = t;
}

void hvswap(char *src, char *dst, int wid, int len){
    int cnt;
    for (cnt = 0; cnt < wid*len; cnt++){
        dst[cnt] = src[(cnt%len)*wid + cnt/len];
    }
}

void hswap(char *s, int wid, int len) {
    int i, j;
    for(i = 0; i < len; i++)
        for(j = 0; j < wid/2; j++)
            swap(&s[i*wid+j], &s[i*wid+wid-1-j]);
}

void vswap(char *s, int wid, int len) {
    int i, j;
    for(i = 0; i < len/2; i++)
        for(j = 0; j < wid; j++)
            swap(&s[i*wid+j], &s[(len-1-i)*wid+j]);
}

int s2i(Uint8 *s, int cnt) {
    int i = 0;
    int num = 0;
    while (s[i]) {
        Log("%4d", s[i]);
        if (s[i] > '9' || s[i] < '0')
            return -1;
        else {
            num *= 10;
            num += s[i] - '0';
        }
        i++;
    }
    return num;
}

void transpose(Uint8 *s, int n){
    int i, j;
    for(i = 0; i < n; i++)
        for(j = i + 1; j < n; j++)
            swap(&s[i*n+j], &s[j*n+i]);
    for(i = 0; i < n/2; i++)
        for(j = 0; j < n; j++)
            swap(&s[i*n+j], &s[(n-1-i)*n+j]);
}
void trans90(Uint8 *s, int n){
    transpose(s, n);
    vswap(s, n, n);
}
void trans270(Uint8 *s, int n){
    transpose(s, n);
    hswap(s, n, n);

}
void trans180(Uint8 *s, int n){
    int i, j;
    for(i = 0; i < n; i++)
        for(j = 0; j < n-i; j++)
            swap(&s[i*n+j], &s[(n-i-1)*n+n-j-1]);
}


