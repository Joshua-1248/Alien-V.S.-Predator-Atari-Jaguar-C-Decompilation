/* Source-shaped ordinary-68000 translation of AMP/FONT.S.
 *
 * The original computes proportional glyph metrics by scanning three 2bpp
 * font brushes, then interprets an encoded stream: BE x/y words followed by
 * text/control bytes.  Jaguar Blitter writes are an explicit rendering seam;
 * parsing, wrapping, cursor timing and input-abort behavior remain here. */
#include "font.h"
#include "computer.h"
#include "joypad.h"
#include "objects.h"
#include "avp_runtime.h"
#include <stdint.h>
#include <string.h>

#define CHR_HEIGHT 9
#define FIRST_CHAR 33u
#define TTYPE_ON   0xfeu
#define TTYPE_OFF  0xfdu
#define NEW_XY     0xffu
#define TTYPE_SPEED 2

s16 x_print_pos,y_print_pos,x_line_pos,y_line_pos;
s16 line_width,line_height;
AvpFontMetric chrlist[256];
static const u8 *brushes[3];
static size_t brush_sizes[3];
static s16 typeflag,cursorflag,cursor_on,cursor_off,s_delay;

static u16 be16p(const u8 *p){return (u16)(((u16)p[0]<<8)|p[1]);}

void avp_font_bind_brushes(const u8 *a,const u8 *b,const u8 *c,size_t n)
{
    brushes[0]=a;brushes[1]=b;brushes[2]=c;
    brush_sizes[0]=brush_sizes[1]=brush_sizes[2]=n;
}
const AvpFontMetric *avp_font_metrics(void){return chrlist;}

static int column_nonblank(const u8 *brush,size_t cap,unsigned pixel)
{
    static const u8 masks[4]={0xc0u,0x30u,0x0cu,0x03u};
    size_t byte=(size_t)(pixel>>2);
    u8 mask=masks[pixel&3u];
    for(unsigned y=0;y<CHR_HEIGHT;y++){
        size_t off=byte+(size_t)y*64u;
        if(off>=cap)return 0;
        if(brush[off]&mask)return 1;
    }
    return 0;
}

void init_font(void)
{
    unsigned out=0;
    repeat_pad=0;
    memset(chrlist,0,sizeof(chrlist));
    for(unsigned b=0;b<3u && out<256u;b++){
        const u8 *brush=brushes[b];
        unsigned pixel=0,width=0,blankflag=0;
        if(!brush)continue;
        chrlist[out].source_x=0;
        for(unsigned guard=0;guard<2048u && out<256u;guard++){
            ++width;++pixel;
            if(column_nonblank(brush,brush_sizes[b],pixel-1u)){
                blankflag=0;
                continue;
            }
            if(++blankflag==2u){
                /* FONT.S backs a3 up by one word at brush termination so the
                 * uncommitted next-glyph x offset is discarded. */
                break;
            }
            chrlist[out].width=(u16)width;
            chrlist[out].brush=(u16)b;
            ++out;
            if(out<256u)chrlist[out].source_x=(u16)pixel;
            width=0;
        }
    }
}

static void emit_glyph(unsigned ch,int clear)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(ch<FIRST_CHAR)return;
    unsigned i=ch-FIRST_CHAR;
    if(i>=256u)return;
    AvpFontMetric m=chrlist[i];
    if(o->draw_font_glyph){
        o->draw_font_glyph(o->user,m.brush,m.source_x,m.width,
                           (unsigned)(u16)x_print_pos,(unsigned)(u16)y_print_pos,clear);
    }else if(!clear && o->draw_text){
        char t[2]={(char)ch,0};
        o->draw_text(o->user,(unsigned)(u16)y_print_pos,t,(unsigned)(u16)x_print_pos);
    }
}

void blitchar(unsigned ch){emit_glyph(ch,0);}

static void curprint(int clear)
{
    unsigned i=(unsigned)'_'-FIRST_CHAR;
    AvpFontMetric m=chrlist[i];
    if((s32)x_print_pos+(s32)m.width>319){y_print_pos=(s16)(y_print_pos+15);x_print_pos=0;}
    emit_glyph('_',clear);
}
void hide_cursor(void){curprint(1);}
void cursor(void)
{
    if(!cursorflag){
        if(++cursor_on==10){cursor_on=0;cursorflag=1;}
        curprint(0);
    }else{
        if(++cursor_off==10){cursor_off=0;cursorflag=0;}
        curprint(1);
    }
}

void pr_string(const char *encoded)
{
    const u8 *p=(const u8 *)encoded;
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(!p||repeat_pad)return;
    repeat_pad=0xffu;s_delay=0;typeflag=0;cursor_on=cursor_off=0;
getcoords:
    x_print_pos=(s16)be16p(p);p+=2;
    y_print_pos=(s16)be16p(p);p+=2;
    for(;;){
        u8 c=*p++;
        if(c==0){repeat_pad=0;return;}
        xc_readpad();
        /* Computer input is complemented by xc_readpad; set bits are pressed. */
        if((c_joy_edge&(1u<<FIRE_C))||(c_joy_edge&(1u<<FIRE_A))||(c_joy_edge&(1u<<FIRE_B)))return;
        if(c==TTYPE_ON){typeflag=1;continue;}
        if(c==TTYPE_OFF){typeflag=0;continue;}
        if(c==NEW_XY){if((uintptr_t)p&1u)++p;goto getcoords;}
        s_delay^=1;
        if(!s_delay&&o->play_sfx)o->play_sfx(o->user,0); /* typ1b backend mapping */
        if(c==' ')x_print_pos=(s16)(x_print_pos+6);
        else if(c>=FIRST_CHAR){
            unsigned i=(unsigned)c-FIRST_CHAR;
            AvpFontMetric m=chrlist[i<256u?i:0u];
            if((s32)x_print_pos+(s32)m.width>319){y_print_pos=(s16)(y_print_pos+15);x_print_pos=0;}
            emit_glyph(c,0);
            x_print_pos=(s16)(x_print_pos+(s16)m.width);
        }
        if(typeflag){
            /* DBRA with d0=2 executes three iterations. */
            for(unsigned n=0;n<TTYPE_SPEED+1u;n++){VSync();cursor();}
            hide_cursor();
        }
    }
}

static void line_evt(unsigned event,s16 x,s16 y,s16 len)
{
    const AvpRuntimeOps *o=avp_runtime_ops();
    if(o->frontend_event)o->frontend_event(o->user,event,((s32)x<<16)|(u16)y,len);
}
void horiz_line(s16 width){line_evt(0x484c,(s16)x_line_pos,(s16)y_line_pos,width);}
void vert_line(s16 height){line_evt(0x564c,(s16)x_line_pos,(s16)y_line_pos,height);}
void box_draw(const s16 box[4])
{
    if(!box)return;
    x_line_pos=box[0];y_line_pos=box[1];line_width=box[2];line_height=box[3];
    s16 xs=x_line_pos,ys=y_line_pos;
    horiz_line(line_width);
    y_line_pos=(s16)(ys+line_height);horiz_line((s16)(line_width+1));
    x_line_pos=xs;y_line_pos=ys;vert_line(line_height);
    x_line_pos=(s16)(xs+line_width);vert_line(line_height);
}
