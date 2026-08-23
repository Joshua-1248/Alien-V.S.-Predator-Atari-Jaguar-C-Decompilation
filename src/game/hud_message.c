/* Readable C reconstruction of MAZE/HUD_MSG.S.
 *
 * The Jaguar source stores each step as a 32-bit info word followed by three
 * 32-bit text pointers.  This C form makes that record explicit while
 * preserving the original one-frame MSG_NEW display handshake and countdown
 * state machine.  Blitter/font drawing is a backend callback here.
 */
#include "hud_message.h"
#include <stddef.h>

#define HUD_CHAR_WIDTH 13u
#define MSG_WIDTH      192u

u32 msg_info;
u8 msg_status;

static const AvpHudMessageStep *new_message;
static const AvpHudMessageStep *msg_ptr;
static AvpHudMessageDrawLineFn draw_line;

static unsigned text_pixel_width(const char *s)
{
    unsigned n=0;
    if (!s) return 0;
    while (s[n]) ++n;
    return n*HUD_CHAR_WIDTH;
}

static unsigned centred_x(const char *s)
{
    unsigned w=text_pixel_width(s);
    return (w>=MSG_WIDTH) ? 0u : (MSG_WIDTH-w)/2u;
}

void avp_hudmsg_set_draw_callback(AvpHudMessageDrawLineFn fn) { draw_line=fn; }
const AvpHudMessageStep *avp_hudmsg_current(void) { return msg_ptr; }

void InitHUDMsg(void)
{
    new_message=NULL;
    msg_ptr=NULL;
    msg_info=0;
    msg_status=AVP_HUDMSG_STOP;
}

void avp_hudmsg_queue(const AvpHudMessageStep *steps)
{
    /* Historical new_message is only consumed by UpdtHUDMsg, immediately
     * before the frame in which ShowHUDMsg observes MSG_NEW. */
    new_message=steps;
}

void UpdtHUDMsg(void)
{
    if (new_message) {
        msg_ptr=new_message;
        new_message=NULL;
    } else {
        if (msg_status==AVP_HUDMSG_STOP) return;
        --msg_status;
        if (msg_status!=AVP_HUDMSG_STOP) return;
        if (!msg_ptr) return;
        ++msg_ptr;
    }

    if (!msg_ptr) {
        msg_info=0;
        msg_status=AVP_HUDMSG_STOP;
        return;
    }
    msg_info=msg_ptr->info;
    msg_status=(u8)msg_info;
}

void ShowHUDMsg(void)
{
    unsigned i;
    if (msg_status!=AVP_HUDMSG_NEW || !msg_ptr) return;
    if (draw_line) {
        for (i=0;i<AVP_HUDMSG_LINES;++i) {
            const char *s=msg_ptr->line[i];
            if (s) draw_line(i,s,centred_x(s));
        }
    }
    /* In assembly msg_ptr is advanced past three line pointers here, then
     * msg_status=1 so UpdtHUDMsg advances to the next command next frame.
     * Our explicit-record representation advances on that next update. */
    msg_status=1;
}

#define INFO_NEW(rgb) ((u32)(rgb) | AVP_HUDMSG_NEW)
#define INFO_WAIT(rgb,n) ((u32)(rgb) | (u8)(n))
#define END_STEP { AVP_HUDMSG_STOP,{NULL,NULL,NULL} }

/* These sequences are direct structural transcriptions of the corresponding
 * HUD_MSG.S command streams.  Null-line timing records preserve the original
 * coloured/blank flash cadence even though host drawing is delegated. */
const AvpHudMessageStep avp_msg_dead[]={
    {INFO_NEW(0x00ff0000u),{NULL,"GAME OVER",NULL}},
    {INFO_WAIT(0x00ff0000u,254),{NULL,NULL,NULL}}, END_STEP
};
const AvpHudMessageStep avp_msg_pressure[]={
    {INFO_NEW(0xffff0000u),{"AIRLOCK","PRESSURISING",NULL}},
    {INFO_WAIT(0xffff0000u,4),{NULL,NULL,NULL}},
    {5,{NULL,NULL,NULL}},{INFO_WAIT(0xffff0000u,5),{NULL,NULL,NULL}},
    {5,{NULL,NULL,NULL}},{INFO_WAIT(0xffff0000u,5),{NULL,NULL,NULL}}, END_STEP
};
const AvpHudMessageStep avp_msg_notsecure[]={
    {INFO_NEW(0xffffff00u),{"CLOSE OTHER","AIRLOCK DOOR","FIRST"}},
    {INFO_WAIT(0xffffff00u,4),{NULL,NULL,NULL}},
    {5,{NULL,NULL,NULL}},{INFO_WAIT(0xffffff00u,5),{NULL,NULL,NULL}},
    {5,{NULL,NULL,NULL}},{INFO_WAIT(0xffffff00u,5),{NULL,NULL,NULL}}, END_STEP
};
const AvpHudMessageStep avp_msg_airlocked[]={
    {INFO_NEW(0xffff0000u),{"AIRLOCK","NOT IN USE",NULL}},
    {INFO_WAIT(0xffff0000u,4),{NULL,NULL,NULL}},
    {5,{NULL,NULL,NULL}},{INFO_WAIT(0xffff0000u,5),{NULL,NULL,NULL}},
    {5,{NULL,NULL,NULL}},{INFO_WAIT(0xffff0000u,5),{NULL,NULL,NULL}}, END_STEP
};
const AvpHudMessageStep avp_msg_jammed[]={
    {INFO_NEW(0x00ff0000u),{NULL,"DOOR JAMMED",NULL}},
    {INFO_WAIT(0x00ff0000u,4),{NULL,NULL,NULL}},
    {5,{NULL,NULL,NULL}},{INFO_WAIT(0x00ff0000u,5),{NULL,NULL,NULL}},
    {5,{NULL,NULL,NULL}},{INFO_WAIT(0x00ff0000u,5),{NULL,NULL,NULL}}, END_STEP
};
const AvpHudMessageStep avp_msg_access_denied[]={
    {INFO_NEW(0xff000000u),{"SECURITY #","ACCESS","DENIED"}},
    {INFO_WAIT(0xff000000u,4),{NULL,NULL,NULL}},
    {INFO_WAIT(0xffffff00u,5),{NULL,NULL,NULL}},{INFO_WAIT(0xff000000u,5),{NULL,NULL,NULL}},
    {INFO_WAIT(0xffffff00u,5),{NULL,NULL,NULL}},{INFO_WAIT(0xff000000u,5),{NULL,NULL,NULL}},
    {INFO_WAIT(0xffffff00u,5),{NULL,NULL,NULL}},{INFO_WAIT(0xff000000u,5),{NULL,NULL,NULL}}, END_STEP
};

char avp_countdown_text[]="TIME 1:00";
AvpHudMessageStep avp_msg_countdown[]={
    {INFO_NEW(0xffffff00u),{"EVACUATE","BASE",avp_countdown_text}},
    {INFO_WAIT(0xffffff00u,4),{NULL,NULL,NULL}},
    {INFO_WAIT(0x0000ff00u,0x7f),{NULL,NULL,NULL}}, END_STEP
};
