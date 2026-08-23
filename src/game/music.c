/* Readable reconstruction of the ordinary-68000 AVPSOUND/music.o control
 * layer.  The retail DSP/FullSynth program is a separate processor domain,
 * but voice reservation, effect handles, loop release, priority stealing and
 * transfer-file caching are CPU responsibilities and are kept here. */
#include "music.h"
#include "avp_runtime.h"
#include <stdint.h>
#include <string.h>

#define AVP_SOUND_VOICES 8u
#define AVP_TNF_CACHE 9u
#define AVP_DEFAULT_PROTECTED 2u
#define AVP_ALARM_VOICE 2u
#define AVP_AMBIENT_VOICE 0u
#define AVP_SOUND_TYPE_STOPPED (-4)

typedef struct AvpVoiceState {
    s32 type;                 /* retail voice long +0; -4 means stop/free */
    s16 priority;             /* low word of retail priority/control long */
    u8 looping;
    u8 reserved;
    u32 handle;               /* VIdent[] entry */
    unsigned sound_id;
    s32 mod_value;
} AvpVoiceState;

typedef struct AvpTnfCacheEntry {
    const AvpTransferFile *file;
    const void *copy;
} AvpTnfCacheEntry;

s16 fx_fade_level=0x7fff,fx_fade_target=0x7fff;
u32 current_music,alarm_id,ambient_id;

static AvpVoiceState voice[AVP_SOUND_VOICES];
static u32 ident;
static unsigned protected_voices=AVP_DEFAULT_PROTECTED;
static const AvpSoundMeta *sound_meta;
static unsigned sound_meta_count;
static AvpTnfCacheEntry tnf_cache[AVP_TNF_CACHE];
static u8 *tnf_arena;
static size_t tnf_arena_size,tnf_used;
static s32 fade_step;

static const AvpSoundMeta default_meta={0,1,0};
static const AvpSoundMeta *meta_for(unsigned id)
{
    return sound_meta&&id<sound_meta_count?&sound_meta[id]:&default_meta;
}
static void backend_stop(u32 handle)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(handle&&o->stop_sfx)o->stop_sfx(o->user,handle);
}
static void backend_mod(u32 handle,s32 value)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(handle&&o->mod_sfx)o->mod_sfx(o->user,handle,value);
}
static void request_stop(unsigned i,int clear_handle)
{
    if(i>=AVP_SOUND_VOICES)return;
    backend_stop(voice[i].handle);
    voice[i].type=AVP_SOUND_TYPE_STOPPED;
    voice[i].looping=0;
    if(clear_handle)voice[i].handle=0;
}
static void unloop_voice(unsigned i,int clear_handle)
{
    if(i>=AVP_SOUND_VOICES)return;
    if(clear_handle)voice[i].handle=0;
    voice[i].looping=0;
}
static u32 begin_voice(unsigned i,unsigned id,s16 priority,s32 mod_value)
{
    const AvpSoundMeta *m=meta_for(id);
    const AvpRuntimeOps *o=avp_runtime_ops();
    u32 h;
    if(i>=AVP_SOUND_VOICES)return (u32)-1;
    h=++ident;
    if(!h)h=++ident; /* retail Ident naturally wraps; keep zero as no-handle */
    voice[i].type=(s32)m->synth_type;
    voice[i].priority=priority;
    voice[i].looping=m->looping?1u:0u;
    voice[i].handle=h;
    voice[i].sound_id=id;
    voice[i].mod_value=mod_value;
    if(o->play_sfx)o->play_sfx(o->user,id);
    return h;
}

void avp_music_bind_sfx_meta(const AvpSoundMeta *meta,unsigned count)
{
    sound_meta=meta;sound_meta_count=count;
}
void avp_music_bind_transfer_arena(void *arena,size_t size)
{
    tnf_arena=(u8*)arena;tnf_arena_size=size;InitTnF();
}

void KillSynt(void)
{
    /* Retail KillSynt stops the DSP/DAC engines.  Host-side effect state is
     * killed explicitly so a backend sees the same terminal condition. */
    KillAll();StopMusi();
}
void InitSynt(void)
{
    unsigned i;
    KillSynt();
    fx_fade_level=fx_fade_target=0x7fff;
    fade_step=0;
    current_music=alarm_id=ambient_id=0;
    ident=0;
    for(i=0;i<AVP_SOUND_VOICES;i++){
        memset(&voice[i],0,sizeof(voice[i]));
        voice[i].type=AVP_SOUND_TYPE_STOPPED;
    }
    ProtectV(AVP_DEFAULT_PROTECTED);
    InitTnF();
}
void StopMusi(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->stop_music)o->stop_music(o->user);
    current_music=0;
}

void InitTnF(void)
{
    memset(tnf_cache,0,sizeof(tnf_cache));
    tnf_used=0;
}
const void *TransnFi(const AvpTransferFile *file)
{
    unsigned i,empty=AVP_TNF_CACHE;
    size_t aligned,need;
    if(!file)return NULL;
    if(!tnf_arena)return file->data; /* retail fix_loc==0 path */
    for(i=0;i<AVP_TNF_CACHE;i++){
        if(tnf_cache[i].file==file)return tnf_cache[i].copy;
        if(!tnf_cache[i].file&&empty==AVP_TNF_CACHE)empty=i;
    }
    if(empty==AVP_TNF_CACHE)return NULL;
    aligned=(tnf_used+7u)&~(size_t)7u;
    need=(file->size+7u)&~(size_t)7u;
    if(aligned>tnf_arena_size||need>tnf_arena_size-aligned)return NULL;
    if(file->size)memcpy(tnf_arena+aligned,file->data,file->size);
    tnf_cache[empty].file=file;
    tnf_cache[empty].copy=tnf_arena+aligned;
    tnf_used=aligned+need;
    return tnf_cache[empty].copy;
}
void StartMus(unsigned id)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    InitTnF();
    StopMusi();
    current_music=id;
    if(o->start_music)o->start_music(o->user,id);
}
void ProtectV(unsigned count)
{
    /* Retail ProtectVoices stores free_ide=VIdent+count and
     * free_voi=voice_table+count*0x50. */
    protected_voices=count>AVP_SOUND_VOICES?AVP_SOUND_VOICES:count;
}

/* FXFadeDown/FXFadeUp install a temporary vblank callback in retail.  The
 * hosted representation keeps the same signed step/target state and advances
 * it when WaitFXFade pumps vblanks. */
static void fade_tick(void)
{
    s32 v=fx_fade_level;
    if(!fade_step)return;
    if(fx_fade_target==0){
        v-=fade_step;
        if(v<=0){v=0;fade_step=0;}
    }else{
        v+=fade_step;
        if(v>=0x7fff){v=0x7fff;fade_step=0;}
    }
    fx_fade_level=(s16)v;
}
void FXFadeDo(s32 step)
{
    WaitFXFa();
    fade_step=step;
    fx_fade_target=0;
}
void FXFadeUp(s32 step)
{
    WaitFXFa();
    fade_step=step;
    fx_fade_target=0x7fff;
}
void WaitFXFa(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    while(fade_step){if(o->wait_vblank)o->wait_vblank(o->user);fade_tick();}
}

void KillAlar(void)
{
    if(!alarm_id)return;
    alarm_id=0;
    unloop_voice(AVP_ALARM_VOICE,0);
    ProtectV(AVP_DEFAULT_PROTECTED);
}
void StartAla(unsigned id)
{
    /* Retail forces the alarm into voice 2, first releasing any loop already
     * there, then raises the protected range from 2 to 3 voices. */
    if(voice[AVP_ALARM_VOICE].type>=0)request_stop(AVP_ALARM_VOICE,1);
    ProtectV(3);
    alarm_id=begin_voice(AVP_ALARM_VOICE,id,-1,-1);
}
void KillAmbi(void)
{
    unloop_voice(AVP_AMBIENT_VOICE,0);
    ambient_id=0;
    {const AvpRuntimeOps *o=avp_runtime_ops();if(o->kill_ambient)o->kill_ambient(o->user);}
}
void Ambient(void)
{
    /* The exact level->ambient descriptor table is authored retail data.  The
     * CPU semantics are to release voice 0 before asking the resource/backend
     * layer to start the level's selected ambient sound in that reserved slot. */
    if(voice[AVP_AMBIENT_VOICE].type>=0)request_stop(AVP_AMBIENT_VOICE,1);
    ambient_id=++ident;if(!ambient_id)ambient_id=++ident;
    voice[AVP_AMBIENT_VOICE].type=1;
    voice[AVP_AMBIENT_VOICE].priority=-1;
    voice[AVP_AMBIENT_VOICE].looping=1;
    voice[AVP_AMBIENT_VOICE].handle=ambient_id;
    {const AvpRuntimeOps *o=avp_runtime_ops();if(o->ambient)o->ambient(o->user);}
}

u32 DoEffect(unsigned id)
{
    const AvpSoundMeta *m=meta_for(id);
    unsigned i,best=AVP_SOUND_VOICES;
    s16 p=m->priority;
    /* First pass mirrors the retail search for a -4/stopped slot. */
    for(i=protected_voices;i<AVP_SOUND_VOICES;i++){
        if(voice[i].type<0){best=i;break;}
    }
    /* If every generic voice is active, steal the first voice whose existing
     * priority is <= the new effect priority, matching DoEffect's signed word
     * comparison/order. */
    if(best==AVP_SOUND_VOICES){
        for(i=protected_voices;i<AVP_SOUND_VOICES;i++){
            if(p>voice[i].priority){best=i;break;}
        }
    }
    if(best==AVP_SOUND_VOICES)return (u32)-1;
    if(voice[best].type>=0)request_stop(best,0);
    return begin_voice(best,id,p,-1);
}
u32 TFX(unsigned id){return DoEffect(id);}
u32 PlacedEf(unsigned id,s32 x,s32 y)
{
    u32 h=DoEffect(id);
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(h!=(u32)-1&&o->play_sfx_params)o->play_sfx_params(o->user,id,x,y);
    return h;
}
void KillAll(void)
{
    unsigned i;const AvpRuntimeOps *o=avp_runtime_ops();
    for(i=0;i<AVP_SOUND_VOICES;i++)if(voice[i].type!=0)voice[i].type=AVP_SOUND_TYPE_STOPPED;
    if(o->kill_sounds)o->kill_sounds(o->user);
}
void KillSfx(u32 h)
{
    unsigned i;
    for(i=0;i<AVP_SOUND_VOICES;i++)if(voice[i].handle==h){request_stop(i,1);return;}
}
void UnLoopAl(void)
{
    unsigned i;
    /* Retail starts at voice 1, deliberately leaving ambient voice 0 alone,
     * and preserves voice 2 while an alarm handle is active. */
    for(i=1;i<AVP_SOUND_VOICES;i++){
        if(alarm_id&&i==AVP_ALARM_VOICE)continue;
        unloop_voice(i,1);
    }
}
void StopLoop(u32 h)
{
    unsigned i;
    for(i=0;i<AVP_SOUND_VOICES;i++)if(voice[i].handle==h){unloop_voice(i,1);return;}
}
/* MAZESCRN.S LoseSounds/RestoreSounds CPU bookkeeping.  Retail snapshots the
 * complete second voice plus the first long of the third voice, preserves the
 * weapon-effect identifier, silences all but the first two voices, and restores
 * that state after pause.  The DSP table itself is represented by AvpVoiceState
 * here; mixer execution remains backend-owned. */
static AvpVoiceState pause_voice1;
static s32 pause_voice2_type;
static u32 pause_weapon_handle;
static int pause_audio_valid;

void avp_pause_audio_save(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    fx_fade_level=0; /* UEBERVOLUME=0 before the table transfer */
    pause_voice1=voice[1];
    pause_voice2_type=voice[2].type;
    pause_weapon_handle=voice[1].handle; /* VIdent+4 */
    pause_audio_valid=1;
    voice[2].type=AVP_SOUND_TYPE_STOPPED;
    voice[0].type=AVP_SOUND_TYPE_STOPPED;
    voice[1].type=AVP_SOUND_TYPE_STOPPED;
    if(o->kill_sounds)o->kill_sounds(o->user);
    fx_fade_level=0x7fff; /* in-pause sounds run at full master volume */
}

void avp_pause_audio_restore(void)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(!pause_audio_valid)return;
    /* Hardware version busy-waits for the two pause voices to become free.
     * A host mixer owns completion; stopping them provides the same boundary. */
    if(voice[0].type>=0)request_stop(0,0);
    if(voice[1].type>=0)request_stop(1,0);
    fx_fade_level=0;
    Ambient();
    voice[1]=pause_voice1;
    voice[2].type=pause_voice2_type;
    voice[1].handle=pause_weapon_handle;
    fx_fade_level=0x7fff;
    pause_audio_valid=0;
    (void)o;
}

void ModSfx(u32 h,s32 value)
{
    unsigned i;
    for(i=0;i<AVP_SOUND_VOICES;i++)if(voice[i].handle==h){voice[i].mod_value=value;backend_mod(h,value);return;}
}
