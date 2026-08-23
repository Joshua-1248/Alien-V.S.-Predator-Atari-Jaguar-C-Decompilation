#ifndef AVP_MUSIC_H
#define AVP_MUSIC_H
#include "avp_types.h"
void KillSynt(void);void InitSynt(void);void StopMusi(void);void InitTnF(void);void TransnFi(void);void StartMus(unsigned id);void ProtectV(void);
void FXFadeDo(void);void FXFadeUp(void);void WaitFXFa(void);void KillAlar(void);void StartAla(unsigned id);void KillAmbi(void);void Ambient(void);u32 PlacedEf(unsigned id,s32 x,s32 y);u32 DoEffect(unsigned id);u32 TFX(unsigned id);void KillAll(void);void KillSfx(u32 handle);void UnLoopAl(void);void StopLoop(u32 handle);void ModSfx(u32 handle,s32 left,s32 right);
extern s16 fx_fade_level,fx_fade_target;extern u32 current_music,alarm_id,ambient_id;
#endif
