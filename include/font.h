#ifndef AVP_FONT_H
#define AVP_FONT_H
#include "avp_types.h"
#include <stddef.h>

typedef struct AvpFontMetric { u16 source_x,width,brush; } AvpFontMetric;

/* The retail font consists of three 512-pixel-wide, 2bpp brushes.  The C
 * scanner only reads them; resource ownership stays outside the public code. */
void avp_font_bind_brushes(const u8 *font_a,const u8 *font_b,const u8 *font_c,size_t bytes_each);
const AvpFontMetric *avp_font_metrics(void);
void init_font(void);
void pr_string(const char *encoded);
void cursor(void);
void hide_cursor(void);
void blitchar(unsigned ch);
void horiz_line(s16 width);
void vert_line(s16 height);
void box_draw(const s16 box[4]);

extern s16 x_print_pos,y_print_pos,x_line_pos,y_line_pos;
extern s16 line_width,line_height;
extern AvpFontMetric chrlist[256];
#endif
