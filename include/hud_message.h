#ifndef AVP_HUD_MESSAGE_H
#define AVP_HUD_MESSAGE_H
#include "avp_types.h"

#define AVP_HUDMSG_LINES 3
#define AVP_HUDMSG_STOP 0u
#define AVP_HUDMSG_NEW  0xffu

typedef struct AvpHudMessageStep {
    u32 info;                    /* low byte is command/countdown, upper bytes are colour/control */
    const char *line[AVP_HUDMSG_LINES];
} AvpHudMessageStep;

typedef void (*AvpHudMessageDrawLineFn)(unsigned line, const char *text, unsigned x_pixels);

void InitHUDMsg(void);
void UpdtHUDMsg(void);
void ShowHUDMsg(void);
void avp_hudmsg_queue(const AvpHudMessageStep *steps);
void avp_hudmsg_set_draw_callback(AvpHudMessageDrawLineFn fn);
const AvpHudMessageStep *avp_hudmsg_current(void);

extern const AvpHudMessageStep avp_msg_dead[];
extern const AvpHudMessageStep avp_msg_pressure[];
extern const AvpHudMessageStep avp_msg_notsecure[];
extern const AvpHudMessageStep avp_msg_airlocked[];
extern const AvpHudMessageStep avp_msg_jammed[];
extern const AvpHudMessageStep avp_msg_access_denied[];
extern AvpHudMessageStep avp_msg_countdown[];
extern char avp_countdown_text[];

extern u32 msg_info;
extern u8 msg_status;

#endif
