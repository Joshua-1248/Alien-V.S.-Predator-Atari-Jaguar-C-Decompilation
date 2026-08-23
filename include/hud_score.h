#ifndef AVP_HUD_SCORE_H
#define AVP_HUD_SCORE_H
#include "avp_types.h"

typedef void (*AvpHudNumberFn)(s32 value,unsigned digits,unsigned x,unsigned y,unsigned char_w,unsigned char_h);
void avp_hud_score_set_draw(AvpHudNumberFn fn);
void InitScores(void);
void ALScore(void);
void PREDScore(void);
void HUMScores(void);
extern s32 old_score;
extern u8 old_acs;
#endif
