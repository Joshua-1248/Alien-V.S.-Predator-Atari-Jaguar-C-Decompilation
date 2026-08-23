#include "hud_message.h"
#include <assert.h>
#include <string.h>
static unsigned calls,x;
static char got[32];
static void draw(unsigned line,const char *s,unsigned xp){(void)line; ++calls; x=xp; strcpy(got,s);}
int main(void){
 InitHUDMsg(); avp_hudmsg_set_draw_callback(draw); avp_hudmsg_queue(avp_msg_dead);
 UpdtHUDMsg(); assert(msg_status==AVP_HUDMSG_NEW); ShowHUDMsg();
 assert(msg_status==1); assert(calls==1); assert(!strcmp(got,"GAME OVER")); assert(x==(192-9*13)/2);
 UpdtHUDMsg(); assert(msg_status==254); return 0;
}
