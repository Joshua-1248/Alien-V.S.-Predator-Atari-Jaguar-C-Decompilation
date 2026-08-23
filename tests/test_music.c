#include "music.h"
#include "avp_runtime.h"
#include <assert.h>
#include <string.h>

static unsigned played,stopped,modified,killed,music_start,music_stop,ambient_start,ambient_kill;
static u32 last_handle;static s32 last_mod;
static void play(void*u,unsigned id){(void)u;(void)id;++played;}
static void stop(void*u,u32 h){(void)u;last_handle=h;++stopped;}
static void mod(void*u,u32 h,s32 v){(void)u;last_handle=h;last_mod=v;++modified;}
static void kill(void*u){(void)u;++killed;}
static void sm(void*u,unsigned id){(void)u;(void)id;++music_start;}
static void xm(void*u){(void)u;++music_stop;}
static void amb(void*u){(void)u;++ambient_start;}
static void kamb(void*u){(void)u;++ambient_kill;}

int main(void)
{
    AvpRuntimeOps o;AvpSoundMeta meta[16];u8 arena[64];const char payload[]="abcdef";AvpTransferFile f={payload,sizeof(payload)};
    memset(&o,0,sizeof(o));o.play_sfx=play;o.stop_sfx=stop;o.mod_sfx=mod;o.kill_sounds=kill;o.start_music=sm;o.stop_music=xm;o.ambient=amb;o.kill_ambient=kamb;avp_runtime_bind(&o);
    memset(meta,0,sizeof(meta));for(unsigned i=0;i<16;i++){meta[i].priority=(s16)i;meta[i].synth_type=1;}meta[3].looping=1;avp_music_bind_sfx_meta(meta,16);
    InitSynt();assert(killed==1u&&music_stop==1u);
    ProtectV(2);
    u32 h1=DoEffect(3),h2=DoEffect(4);assert(h1!=0u&&h1!=(u32)-1&&h2!=h1&&played==2u);
    ModSfx(h1,123);assert(modified==1u&&last_handle==h1&&last_mod==123);
    StopLoop(h1);KillSfx(h2);assert(stopped==1u&&last_handle==h2);
    StartAla(5);assert(alarm_id!=0u);KillAlar();assert(alarm_id==0u);
    Ambient();assert(ambient_id!=0u&&ambient_start==1u);KillAmbi();assert(ambient_kill==1u&&ambient_id==0u);
    StartMus(7);assert(current_music==7u&&music_start==1u);StopMusi();assert(current_music==0u);
    FXFadeDo(0x1000);WaitFXFa();assert(fx_fade_level==0);FXFadeUp(0x1000);WaitFXFa();assert(fx_fade_level==0x7fff);
    avp_music_bind_transfer_arena(arena,sizeof(arena));const void *a=TransnFi(&f),*b=TransnFi(&f);assert(a&&a==b&&memcmp(a,payload,sizeof(payload))==0);
    avp_runtime_reset_ops();return 0;
}
