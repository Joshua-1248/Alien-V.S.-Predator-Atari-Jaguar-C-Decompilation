/* Readable C reconstruction of FILES/files.o.
 * The Jaguar version stores a cursor into FILES.DAT and advances through
 * 4-byte codec tags + big-endian sizes with four-byte record alignment. */
#include "files.h"
#include "avp_runtime.h"
#include <stdint.h>
#include <string.h>
static const u8 *base,*end,*fpos; static void *rgb_lut,*qc_pos,*lzw_buf; static u8 jpeg_quality=80; static u8 file_gpu_busy;
static AvpDecodeFn jpeg_cb,gzip_cb;static void*decode_user;
static u32 be32(const u8*p){return ((u32)p[0]<<24)|((u32)p[1]<<16)|((u32)p[2]<<8)|p[3];}
void avp_files_bind(const void*d,size_t n){base=fpos=d;end=base?base+n:NULL;}
void avp_files_set_decode_callbacks(AvpDecodeFn j,AvpDecodeFn g,void*u){jpeg_cb=j;gzip_cb=g;decode_user=u;}
void SetFPos(const void*p){fpos=p;} const void*GetFPos(void){return fpos;}
static int record_at(const u8*p,AvpFileRecord*r,const u8**next){if(!p||!end||p+8>end)return 0;memcpy(r->tag,p,4);r->tag[4]=0;r->stored_size=be32(p+4);r->header=p;r->payload=p+8;if(r->payload+r->stored_size>end)return 0;uintptr_t q=(uintptr_t)(r->payload+r->stored_size);q=(q+3u)&~(uintptr_t)3u;*next=(const u8*)q;return *next<=end;}
const void*aFilePos(const void*archive,unsigned index){const u8*p=archive;AvpFileRecord r;const u8*n;for(unsigned i=0;i<index;i++)if(!record_at(p,&r,&n))return NULL;else p=n;return p;}
const void*xFilePos(unsigned index){return aFilePos(fpos,index);}
const void*GetFile(unsigned index){const u8*p=xFilePos(index);if(!p)return NULL;AvpFileRecord r;const u8*n;if(!record_at(p,&r,&n))return NULL;fpos=n;return p;}
const void*xGetFile(unsigned index){return GetFile(index);} const void*yGetFile(unsigned index){return GetFile(index);}
static AvpFileCodec codec(const AvpFileRecord*r){if(!memcmp(r->tag,"NONE",4))return AVP_FC_NONE;if(!memcmp(r->tag,"GZIP",4))return AVP_FC_GZIP;if(r->tag[0]=='J'&&r->tag[1]=='P')return AVP_FC_JPEG;return AVP_FC_UNKNOWN;}
int exp_none(const AvpFileRecord*r,void*dst,size_t cap,size_t*w){size_t n=r->stored_size;if(n>cap)return 0;memcpy(dst,r->payload,n);if(w)*w=n;return 1;}
int init_jpe(const AvpFileRecord*r){(void)r;file_gpu_busy=1;return 1;} int get_qcta(unsigned i){(void)i;return 1;}
int exp_jpeg(const AvpFileRecord*r,void*d,size_t n,size_t*w){int ok=jpeg_cb?jpeg_cb(decode_user,r,d,n,w):0;file_gpu_busy=0;return ok;}
int init_gzi(const AvpFileRecord*r){(void)r;return 1;} int exp_gzip(const AvpFileRecord*r,void*d,size_t n,size_t*w){return gzip_cb?gzip_cb(decode_user,r,d,n,w):0;}
int yTransFi(const void*record,void*dst,size_t cap,size_t*w){AvpFileRecord r;const u8*n;if(!record_at(record,&r,&n))return 0;switch(codec(&r)){case AVP_FC_NONE:return exp_none(&r,dst,cap,w);case AVP_FC_JPEG:init_jpe(&r);return exp_jpeg(&r,dst,cap,w);case AVP_FC_GZIP:init_gzi(&r);return exp_gzip(&r,dst,cap,w);default:return 0;}}
int TransFil(const void*record,void*dst,size_t cap,size_t*w){return yTransFi(record,dst,cap,w);} int xTransFi(unsigned i,void*d,size_t n,size_t*w){const void*p=xGetFile(i);return p?yTransFi(p,d,n,w):0;}
void InitFile(void){jpeg_quality=80;file_gpu_busy=0;rgb_lut=qc_pos=lzw_buf=NULL;}
void SetRGBLo(void*p){rgb_lut=p;} void SetJPEGT(u8 q){if(q!=jpeg_quality){jpeg_quality=q;file_gpu_busy=0;}} void ResetFGP(void){file_gpu_busy=0;} void SetQCPos(void*p){qc_pos=p;file_gpu_busy=0;} void SetLZWBu(void*p){lzw_buf=p;file_gpu_busy=0;}
