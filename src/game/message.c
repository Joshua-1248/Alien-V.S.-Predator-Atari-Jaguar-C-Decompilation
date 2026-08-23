#include "message.h"
#include "hud_message.h"
/* Retail ClearSta clears the status-message display state; HUD message storage
 * remains owned by hud_message.c. */
void ClearSta(void){InitHUDMsg();}
