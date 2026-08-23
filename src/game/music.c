/* Ordinary-68000 AVPSOUND/music.o control layer. DSP FullSynth execution is a
 * separate processor domain and is intentionally behind AvpRuntimeOps. */
#include "music.h"
#include "avp_runtime.h"
s16 fx_fade_level=0x100,fx_fade_target=0x100;u32 current_music,alarm_id,ambient_id;static u32 next_handle=1;
void KillSynt(void){KillAll();current_music=0;} void InitSynt(void){fx_fade_level=fx_fade_target=0x100;current_music=alarm_id=ambient_id=0;}
void StopMusi(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->stop_music)o->stop_music(o->user);current_music=0;} void InitTnF(void){} void TransnFi(void){} void StartMus(unsigned id){current_music=id;const AvpRuntimeOps*o=avp_runtime_ops();if(o->start_music)o->start_music(o->user,id);} void ProtectV(void){}
void FXFadeDo(void){if(fx_fade_level<fx_fade_target)++fx_fade_level;else if(fx_fade_level>fx_fade_target)--fx_fade_level;} void FXFadeUp(void){fx_fade_target=0x100;} void WaitFXFa(void){while(fx_fade_level!=fx_fade_target)FXFadeDo();}
void KillAlar(void){alarm_id=0;} void StartAla(unsigned id){alarm_id=id;const AvpRuntimeOps*o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,id);} void KillAmbi(void){ambient_id=0;const AvpRuntimeOps*o=avp_runtime_ops();if(o->kill_ambient)o->kill_ambient(o->user);} void Ambient(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->ambient)o->ambient(o->user);}
u32 DoEffect(unsigned id){u32 h=next_handle++;const AvpRuntimeOps*o=avp_runtime_ops();if(o->play_sfx)o->play_sfx(o->user,id);return h;}u32 TFX(unsigned id){return DoEffect(id);}u32 PlacedEf(unsigned id,s32 x,s32 y){u32 h=next_handle++;const AvpRuntimeOps*o=avp_runtime_ops();if(o->play_sfx_params)o->play_sfx_params(o->user,id,x,y);else if(o->play_sfx)o->play_sfx(o->user,id);return h;}
void KillAll(void){const AvpRuntimeOps*o=avp_runtime_ops();if(o->kill_sounds)o->kill_sounds(o->user);alarm_id=ambient_id=0;} void KillSfx(u32 h){(void)h;} void UnLoopAl(void){alarm_id=0;} void StopLoop(u32 h){(void)h;} void ModSfx(u32 h,s32 l,s32 r){(void)h;const AvpRuntimeOps*o=avp_runtime_ops();if(o->play_sfx_params)o->play_sfx_params(o->user,0,l,r);}
