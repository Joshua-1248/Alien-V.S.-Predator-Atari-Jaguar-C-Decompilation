#ifndef AVP_FILES_H
#define AVP_FILES_H
#include "avp_types.h"
#include <stddef.h>

typedef enum AvpFileCodec { AVP_FC_NONE=0, AVP_FC_JPEG=1, AVP_FC_GZIP=2, AVP_FC_UNKNOWN=3 } AvpFileCodec;
typedef struct AvpFileRecord { char tag[5]; const u8 *payload; u32 stored_size; const u8 *header; } AvpFileRecord;
typedef int (*AvpDecodeFn)(void *user,const AvpFileRecord *record,void *dst,size_t dst_size,size_t *written);
void avp_files_bind(const void *data,size_t size);
void avp_files_set_decode_callbacks(AvpDecodeFn jpeg,AvpDecodeFn gzip,void *user);
void SetFPos(const void *pos); const void *GetFPos(void);
const void *GetFile(unsigned index); const void *xGetFile(unsigned index); const void *yGetFile(unsigned index);
const void *aFilePos(const void *archive,unsigned index); const void *xFilePos(unsigned index);
int TransFil(const void *record,void *dst,size_t dst_size,size_t *written);
int xTransFi(unsigned index,void *dst,size_t dst_size,size_t *written);
int yTransFi(const void *record,void *dst,size_t dst_size,size_t *written);
void InitFile(void); void SetRGBLo(void *p); void SetJPEGT(u8 quality); void ResetFGP(void); void SetQCPos(void *p); void SetLZWBu(void *p);
int exp_none(const AvpFileRecord *r,void *dst,size_t dst_size,size_t *written);
int init_jpe(const AvpFileRecord *r); int get_qcta(unsigned index); int exp_jpeg(const AvpFileRecord *r,void *dst,size_t dst_size,size_t *written);
int init_gzi(const AvpFileRecord *r); int exp_gzip(const AvpFileRecord *r,void *dst,size_t dst_size,size_t *written);
#endif
