/*
 * Readable compatibility reconstruction of the Atari Jaguar AvP inflate path.
 *
 * This is NOT claimed to be literal Rebellion/Club Drive source text.  The
 * wrapper/glue follows the machine-code reconstruction; the DEFLATE decoder is
 * an independently written RFC 1951 implementation used here so the readable-C
 * repository contains complete semantics rather than an unresolved stock-core
 * placeholder.  The byte-exact preservation repository remains the compiler/
 * object-code oracle for the historical build.
 */
#include <stddef.h>

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

extern uch *inptr;
extern uch *outptr;
extern ulg outsize;
extern void *malloc(ulg);
extern void memzero(void *, ulg);
extern void loadgpu(void *);
extern unsigned char gpunzip[];

uch *slide;
unsigned wp;
ulg bb;
unsigned bk;
unsigned lbits;
unsigned dbits;
unsigned hufts;

static uch *initial_out;

static void avp_flush(unsigned n)
{
    uch *s=slide;
    while(n--)*outptr++=*s++;
}

static ulg m2il(const uch *p)
{
    return ((ulg)p[0])|((ulg)p[1]<<8)|((ulg)p[2]<<16)|((ulg)p[3]<<24);
}

int init_in(void)
{
    loadgpu(gpunzip);
    slide=(uch *)malloc(0x8000UL);
    return slide==0;
}

#define MAXBITS 15
#define MAXLCODES 286
#define MAXDCODES 30
#define MAXCODES (MAXLCODES+MAXDCODES)

typedef struct BitStream {
    const uch *base;
    ulg bitpos;
} BitStream;

typedef struct Huffman {
    ush count[MAXBITS+1];
    ush symbol[MAXCODES];
} Huffman;

static unsigned get_bits(BitStream *s,unsigned n)
{
    unsigned v=0;
    for(unsigned i=0;i<n;i++,s->bitpos++)
        v|=(unsigned)((s->base[s->bitpos>>3]>>(s->bitpos&7u))&1u)<<i;
    return v;
}

/* Build the canonical symbol ordering used by the compact bit-at-a-time
 * decoder.  Return nonzero for an oversubscribed set. */
static int huff_build(Huffman *h,const uch *lens,unsigned n)
{
    ush offs[MAXBITS+1];
    int left=1;
    for(unsigned i=0;i<=MAXBITS;i++)h->count[i]=0;
    for(unsigned i=0;i<n;i++){
        if(lens[i]>MAXBITS)return 1;
        h->count[lens[i]]++;
    }
    if(h->count[0]==n)return 0;
    for(unsigned len=1;len<=MAXBITS;len++){
        left<<=1;left-=h->count[len];if(left<0)return 1;
    }
    offs[1]=0;
    for(unsigned len=1;len<MAXBITS;len++)offs[len+1]=(ush)(offs[len]+h->count[len]);
    for(unsigned sym=0;sym<n;sym++)if(lens[sym])h->symbol[offs[lens[sym]]++]=(ush)sym;
    return 0;
}

static int huff_decode(BitStream *s,const Huffman *h)
{
    unsigned code=0,first=0,index=0;
    for(unsigned len=1;len<=MAXBITS;len++){
        unsigned count;
        code|=get_bits(s,1);count=h->count[len];
        if(code<first+count)return h->symbol[index+(code-first)];
        index+=count;first=(first+count)<<1;code<<=1;
    }
    return -1;
}

static void put_byte(uch c)
{
    slide[wp++]=c;
    if(wp==0x8000u){avp_flush(0x8000u);wp=0;}
}

static int inflate_codes(BitStream *s,const Huffman *lit,const Huffman *dist)
{
    static const ush lbase[29]={3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
    static const uch lext[29]={0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
    static const ush dbase[30]={1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
    static const uch dext[30]={0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
    for(;;){
        int sym=huff_decode(s,lit);
        if(sym<0)return 2;
        if(sym<256){put_byte((uch)sym);continue;}
        if(sym==256)return 0;
        if(sym<257||sym>285)return 2;
        {
            unsigned li=(unsigned)sym-257u;
            unsigned len=lbase[li]+get_bits(s,lext[li]);
            int ds=huff_decode(s,dist);unsigned distance;
            if(ds<0||ds>=30)return 3;
            distance=dbase[ds]+get_bits(s,dext[ds]);
            if(!distance||distance>0x8000u)return 3;
            while(len--){unsigned src=(wp-distance)&0x7fffu;put_byte(slide[src]);}
        }
    }
}

static int fixed_block(BitStream *s)
{
    uch ll[288],dd[32];Huffman lit,dist;
    for(unsigned i=0;i<=143;i++)ll[i]=8;
    for(unsigned i=144;i<=255;i++)ll[i]=9;
    for(unsigned i=256;i<=279;i++)ll[i]=7;
    for(unsigned i=280;i<288;i++)ll[i]=8;
    for(unsigned i=0;i<32;i++)dd[i]=5;
    if(huff_build(&lit,ll,288)||huff_build(&dist,dd,32))return 2;
    return inflate_codes(s,&lit,&dist);
}

static int dynamic_block(BitStream *s)
{
    static const uch order[19]={16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
    unsigned nlit=get_bits(s,5)+257u,ndist=get_bits(s,5)+1u,ncode=get_bits(s,4)+4u;
    uch clen[19]={0},lens[MAXCODES];Huffman ch,lit,dist;unsigned n=nlit+ndist,pos=0;
    if(nlit>MAXLCODES||ndist>MAXDCODES)return 2;
    for(unsigned i=0;i<ncode;i++)clen[order[i]]=(uch)get_bits(s,3);
    if(huff_build(&ch,clen,19))return 2;
    while(pos<n){
        int sym=huff_decode(s,&ch);if(sym<0)return 2;
        if(sym<16)lens[pos++]=(uch)sym;
        else if(sym==16){unsigned rep;if(!pos)return 2;rep=get_bits(s,2)+3u;if(pos+rep>n)return 2;while(rep--)lens[pos]=lens[pos-1],pos++;}
        else if(sym==17){unsigned rep=get_bits(s,3)+3u;if(pos+rep>n)return 2;while(rep--)lens[pos++]=0;}
        else if(sym==18){unsigned rep=get_bits(s,7)+11u;if(pos+rep>n)return 2;while(rep--)lens[pos++]=0;}
        else return 2;
    }
    if(!lens[256])return 2;
    if(huff_build(&lit,lens,nlit)||huff_build(&dist,lens+nlit,ndist))return 2;
    return inflate_codes(s,&lit,&dist);
}

static int stored_block(BitStream *s)
{
    unsigned len,nlen;
    s->bitpos=(s->bitpos+7u)&~7u;
    len=get_bits(s,16);nlen=get_bits(s,16);
    if((len^0xffffu)!=(nlen&0xffffu))return 1;
    while(len--)put_byte((uch)get_bits(s,8));
    return 0;
}

static int inflate_deflate(BitStream *s)
{
    int last=0;
    do{
        unsigned type;int r;last=(int)get_bits(s,1);type=get_bits(s,2);
        if(type==0)r=stored_block(s);
        else if(type==1)r=fixed_block(s);
        else if(type==2)r=dynamic_block(s);
        else return 2;
        if(r)return r;
    }while(!last);
    s->bitpos=(s->bitpos+7u)&~7u;
    return 0;
}

int inflate_avp_wrapper_candidate(void)
{
    BitStream bits;int r;const uch *trailer;
    initial_out=outptr;
    inptr+=10;                       /* historical wrapper skips gzip header */
    wp=0;bk=0;bb=0;lbits=9;dbits=6;hufts=0;
    memzero(slide,0x8000UL);
    bits.base=inptr;bits.bitpos=0;
    r=inflate_deflate(&bits);if(r)return r;
    trailer=bits.base+(bits.bitpos>>3);inptr=(uch *)trailer;
    avp_flush(wp);wp=0;
    outsize=(ulg)(outptr-initial_out);
    /* Retail code contains tolerance for the one-byte historical pointer
     * ambiguity, hence the +4 / +3 checks retained from the machine code. */
    if(m2il(trailer+4)!=outsize && m2il(trailer+3)!=outsize)return 6;
    return 0;
}
