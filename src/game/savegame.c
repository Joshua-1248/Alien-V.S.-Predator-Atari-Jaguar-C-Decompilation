/* Source-guided C translation of MAZESCRN.S::MakeSave / SaveGame.
 *
 * The retail save record is exactly five big-endian longwords (20 bytes).
 * EEPROM storage remains in AVPCART/eeprom.c; this module owns the ordinary
 * 68000 game-state packing that the pause menu uses before Write_EE.
 */
#include "savegame.h"
#include "eeprom.h"
#include "maze.h"
#include "player.h"
#include "collectables.h"
#include "weapons.h"
#include "levels.h"
#include "hud.h"
#include "avp_runtime.h"

#include <stdint.h>

static void put_be16(u8 *p,u16 v){p[0]=(u8)(v>>8);p[1]=(u8)v;}
static void put_be32(u8 *p,u32 v){p[0]=(u8)(v>>24);p[1]=(u8)(v>>16);p[2]=(u8)(v>>8);p[3]=(u8)v;}
static u16 ror16(u16 v,unsigned n){n&=15u;return n?(u16)((v>>n)|(u16)(v<<(16u-n))):v;}

/* encode_cocoon from MAZESCRN.S:
 *   x(6):y(6):level(4):frame(5):unused(11)
 */
static u32 encode_cocoon(const AvpCocoonState *c)
{
    u32 v=0;
    u16 f;
    if(!c)return 0;
    v|=(c->x&0x003f0000u)<<10;
    v|=(c->y&0x003f0000u)<<4;
    v|=((u32)(u16)c->level&0x0fu)<<16;
    f=ror16((u16)c->frame&0x001fu,5);
    v|=f;
    return v;
}

void MakeSave(s32 slot)
{
    u8 *out;
    u32 d0,d1;

    /* CMP.L #2 / BLS is unsigned. Negative/other values select SaveCont. */
    if((u32)slot<=2u)out=cartcopy+(unsigned)slot*AVP_SAVE_SIZE;
    else out=SaveCont;

    /* LONG 0: x_pos(6.6):y_pos(6.6):centre_angle(8). */
    d0=(x_pos&0x003ffc00u)<<10;
    d0|=(y_pos&0x003ffc00u)>>2;
    d1=(centre_angle>>24)&0xffu; /* ROL.L #8 then AND #$ff selects original high byte */
    d0|=d1;
    put_be32(out,d0);out+=4;

    /* LONG 1: signed score, stored as raw 32-bit bits. */
    put_be32(out,(u32)score);out+=4;

    if(player_type==PT_HUMAN){
        /* LONGS 2 & 3: ammo1..ammo4 as four big-endian words. */
        for(unsigned i=0;i<4u;i++){put_be16(out,ammo_info[i].cur);out+=2;}
    }else if(player_type==PT_ALIEN){
        /* Three 21-bit cocoon records are packed across LONGS 2 & 3 exactly
         * as the SWAP/ROR/OR sequence in MAZESCRN.S. */
        u32 e0=encode_cocoon(&cocoon_data[0]);
        u32 e1=encode_cocoon(&cocoon_data[1]);
        u32 e2=encode_cocoon(&cocoon_data[2]);
        u32 l2=e0|(e1>>21);
        u32 l3=((e1&0x001ff800u)<<11)|(e2>>10);
        put_be32(out,l2);out+=4;
        put_be32(out,l3);out+=4;
    }else{
        /* Predator LONG 2: medpak ammo word + zero word; LONG 3 zero. */
        put_be16(out,ammo_info[5].cur);out+=2;
        put_be16(out,0);out+=2;
        put_be32(out,0);out+=4;
    }

    /* LONG 4: energy(16):access(4):motion tracker(1):weapons(5):
     *         level(4):player type(2). */
    d0=(u32)(u16)player_energy<<16;
    d0|=((u32)acs_level&0x0fu)<<12;
    d0|=(u32)((u16)show_mt&(1u<<11));
    d0|=((u32)cur_weps&0x3eu)<<5;
    d0|=((u32)(u16)cur_level&0x0fu)<<2;
    d0|=((u32)(u16)player_type>>2)&3u;
    put_be32(out,d0);
}

void SaveGame(s32 slot)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    /* Retail plays the shotgun save-confirm effect, then writes EEPROM and
     * flashes the slot graphics. Audio/pixel flashing are presentation seams. */
    if(o->play_sfx)o->play_sfx(o->user,1u);
    MakeSave(slot);
    Write_EE();
    if(o->frontend_event)o->frontend_event(o->user,0x53415645u,slot,20); /* SAVE */
}
