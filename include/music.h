#ifndef AVP_MUSIC_H
#define AVP_MUSIC_H
#include "avp_types.h"
#include <stddef.h>

/* Readable 68000-side AVPSOUND control state.  The DSP mixer/synth itself is
 * intentionally outside this module; the retail CPU owned voice reservation,
 * handle allocation, loop release and effect-priority decisions. */
typedef struct AvpSoundMeta {
    s16 priority;     /* low-word priority compared by DoEffect */
    u8  synth_type;   /* retail voice type; type 4 has a different control slot */
    u8  looping;      /* descriptor requests a looping voice */
} AvpSoundMeta;

typedef struct AvpTransferFile {
    const void *data;
    size_t size;
} AvpTransferFile;

void avp_music_bind_sfx_meta(const AvpSoundMeta *meta,unsigned count);
void avp_music_bind_transfer_arena(void *arena,size_t size);
const void *TransnFi(const AvpTransferFile *file);

void KillSynt(void);
void InitSynt(void);
void StopMusi(void);
void InitTnF(void);
void StartMus(unsigned id);
void ProtectV(unsigned count);
void FXFadeDo(s32 step);
void FXFadeUp(s32 step);
void WaitFXFa(void);
void KillAlar(void);
void StartAla(unsigned id);
void KillAmbi(void);
void Ambient(void);
u32 PlacedEf(unsigned id,s32 x,s32 y);
u32 DoEffect(unsigned id);
u32 TFX(unsigned id);
void KillAll(void);
void KillSfx(u32 handle);
void UnLoopAl(void);
void StopLoop(u32 handle);
void ModSfx(u32 handle,s32 value);
void avp_pause_audio_save(void);
void avp_pause_audio_restore(void);

extern s16 fx_fade_level,fx_fade_target;
extern u32 current_music,alarm_id,ambient_id;
#endif
