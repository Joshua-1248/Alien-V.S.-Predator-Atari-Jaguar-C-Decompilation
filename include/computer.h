#ifndef AVP_COMPUTER_H
#define AVP_COMPUTER_H
#include "avp_types.h"
#include <stddef.h>

typedef enum AvpComputerHandler {
    AVP_COMP_GENERIC=0,
    AVP_COMP_ARMOURY,
    AVP_COMP_MEDICAL,
    AVP_COMP_POD
} AvpComputerHandler;

typedef struct AvpComputerContent {
    u16 pages;
    const char *const *page;
    /* Source pointer-identity decisions expressed as resource metadata. */
    const struct AvpComputerContent *access9_alternate;
    u8 allows_destruct;
} AvpComputerContent;

typedef struct AvpComputerTerminal {
    u16 comp_offset;
    AvpComputerHandler handler;
    const AvpComputerContent *content;
} AvpComputerTerminal;

void avp_computer_bind_level(unsigned level,const AvpComputerTerminal *records,size_t count);
void cls(void);
void InitComp(void);
void Computer(void);
void c_readpad(void);
void xc_readpad(void);
void RestoreCDisplay(void);
void InitCDisplay(void);
void SetCObj1(void);
void SetCObj2(void);
void NewOver(unsigned id);

/* Explicit progression operations used by the source menus and tests. */
int avp_computer_first_aid(void);
int avp_computer_can_destruct(const AvpComputerContent *content);
void avp_computer_arm_destruct(void);
int avp_computer_launch_pod(void);

extern u32 c_joy_cur,c_joy_edge;
extern u8 repeat_pad;
extern s16 cursorpos,picount,pages,menuopts;
#endif
