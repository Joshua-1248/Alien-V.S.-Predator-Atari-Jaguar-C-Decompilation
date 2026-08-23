/* AMP/FONT.S high-level text/drawing semantics. Pixel blits are host/Jaguar backend work. */
#include "font.h"
#include "avp_runtime.h"
s16 x_print_pos,y_print_pos,x_line_pos,y_line_pos;
void init_font(void){x_print_pos=y_print_pos=x_line_pos=y_line_pos=0;}void blitchar(unsigned ch){char t[2]={(char)ch,0};const AvpRuntimeOps*o=avp_runtime_ops();if(o->draw_text)o->draw_text(o->user,(unsigned)y_print_pos,t,(unsigned)x_print_pos);x_print_pos+=8;}void pr_string(const char*s){if(!s)return;while(*s){if(*s=='\n'){x_print_pos=0;y_print_pos+=8;}else blitchar((unsigned char)*s);++s;}}void cursor(void){blitchar('_');}
static void line_event(unsigned e,s16 a,s16 b,s16 c,s16 d){const AvpRuntimeOps*o=avp_runtime_ops();if(o->frontend_event)o->frontend_event(o->user,e,((s32)a<<16)|(u16)b,((s32)c<<16)|(u16)d);}
void horiz_line(s16 x0,s16 x1,s16 y){line_event(0x484c,x0,y,x1,y);}void vert_line(s16 x,s16 y0,s16 y1){line_event(0x564c,x,y0,x,y1);}void box_draw(s16 x0,s16 y0,s16 x1,s16 y1){horiz_line(x0,x1,y0);horiz_line(x0,x1,y1);vert_line(x0,y0,y1);vert_line(x1,y0,y1);}
