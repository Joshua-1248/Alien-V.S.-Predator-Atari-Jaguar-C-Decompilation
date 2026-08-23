/* CPU-side score/access update logic from MAZE/HUD.S.  The original routines
 * configure GPUBUF registers then invoke the Jaguar number-print GPU overlay;
 * this readable C form keeps change detection, values and coordinates while a
 * renderer callback replaces the GPU implementation.
 */
#include "hud_score.h"
#include "player.h"
#include "doors.h"

s32 old_score=-1;
u8 old_acs=0xffu;
static AvpHudNumberFn draw_number;
extern u8 show_coords;
extern s32 x_vel;

void avp_hud_score_set_draw(AvpHudNumberFn fn){draw_number=fn;}
void InitScores(void){old_score=-1;old_acs=0xffu;}

void ALScore(void)
{
    if(score==old_score)return;
    old_score=score;
    if(draw_number)draw_number(score,10,148,10,8,10);
}
void PREDScore(void)
{
    if(score==old_score)return;
    old_score=score;
    if(draw_number)draw_number(score,10,111,10,11,12);
}
void HUMScores(void)
{
    s32 shown=show_coords?(s32)(u16)player_energy:score;
    if(shown!=old_score){old_score=shown;if(draw_number)draw_number(shown,10,151,8,8,9);}
    if(acs_level!=old_acs){old_acs=acs_level;if(draw_number)draw_number((s32)acs_level,2,43,90,8,9);}
}
