#ifndef AVP_FONT_H
#define AVP_FONT_H
#include "avp_types.h"
void init_font(void);void pr_string(const char *s);void cursor(void);void blitchar(unsigned ch);void horiz_line(s16 x0,s16 x1,s16 y);void vert_line(s16 x,s16 y0,s16 y1);void box_draw(s16 x0,s16 y0,s16 x1,s16 y1);
extern s16 x_print_pos,y_print_pos,x_line_pos,y_line_pos;
#endif
