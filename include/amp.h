#ifndef AVP_AMP_H
#define AVP_AMP_H
#include "avp_types.h"

#define AVP_NUM_AMPS 300
#define AVP_AMP_GRID_W 64
#define AVP_AMP_GRID_H 64
#define AVP_AMP_OBJ_ROW_BYTES 128
#define AVP_AMP_OBJ_H 64

typedef struct AvpAmp AvpAmp;
typedef void (*AvpAmpModeFn)(AvpAmp *amp);

struct AvpAmp {
    s32 xpos,ypos;
    AvpAmpModeFn mode;
    u16 creature;
    u16 animseq;
    u16 animframe;
    u16 angle;
    s32 timer;
    s16 xvel,yvel;
    s16 ldir;
    s32 xvector,yvector;
    s16 energy,oldenergy;
    u16 flags;
    s16 yoffset;
    s16 astype;
    void *aux_ptr; /* host-readable form of pointer aliases carried in amp_timer */
    u8 host_static; /* source amp_mode == -1 */
};

enum AvpAmpFlag {
    AMP_KILLABLE=0,AMP_DEADGEN,AMP_PLAYER,AMP_DEAD,AMP_COLLECT,AMP_MOTION,AMP_STUN,
    AMP_PHIT,AMP_CHATFLAG,AMP_EGGOPEN,AMP_CLOSEHIT,AMP_INVHIT,AMP_SSHIELD,AMP_ALTFRAME=15
};

enum AvpAnimSeq {
    AS_HIDDEN=0,AS_STAND,AS_DEATH,AS_RUN,AS_WALK,AS_KNOCKBACK,
    AS_FIGHT1,AS_FIGHT2,AS_FIGHT3,AS_FIGHT4,AS_FIGHT5,AS_FIGHT6
};

enum AvpCreatureType {
    AC_ALIEN=0,AC_PREDATOR,AC_HUMAN,AC_EGG,AC_FACEHUG,AC_ACRAWL,AC_SMART,AC_BOLT,
    AC_FIRE,AC_SPARK1,AC_SPARK2,AC_AID,AC_AMMO1,AC_AMMO2,AC_BAG,AC_CAN,AC_CART,
    AC_CHAIN,AC_COCOON,AC_DISK,AC_DRUM,AC_FOOD,AC_GAS,AC_GREN,AC_GROUP,AC_SHELLS,
    AC_TABLE1,AC_TABLE2,AC_TABLE3,AC_DEADOFF,AC_DEADMED,AC_ABLOOD,AC_PBLOOD,
    AC_MBLOOD,AC_CRYO,AC_OPTABLE,AC_BED,AC_AQUEEN,AC_GEN,AC_AQSHIELD
};

void InitAMPs(void);
void initialise(void);
void wipe_flgs(void);
void build_level(void);
AvpAmp *amp_req(void);
void amp_release(unsigned current_amp_number_1based);
void save_level(void);
void restore_level(void);

void avp_amp_set_save_callback(void (*fn)(AvpAmp *amps,unsigned count));
void avp_amp_set_restore_callback(void (*fn)(AvpAmp *amps,unsigned count));

extern AvpAmp level1amps[AVP_NUM_AMPS];
extern AvpAmp *amp_data,*amps_at;
extern u16 collmap[AVP_AMP_GRID_W*AVP_AMP_GRID_H];
extern u8 objmap[AVP_AMP_OBJ_ROW_BYTES*AVP_AMP_OBJ_H];
extern u16 levels_visit,discflag;


/* Source-level explicit-argument forms of register-call AMP helpers. */
AvpAmp *player_weapon(s32 heading_x,s32 heading_y,u16 projectile_kind,u16 reset_counter,
                      s32 sin_angle,s32 cos_angle);
void lostplayer(AvpAmp *amp);
void UpdateAMPs(void);
void chase_player(AvpAmp *amp);
void pre_discmove(AvpAmp *amp);
void discmove(AvpAmp *amp);
void pre_flame_on(AvpAmp *amp);
void flame_on(AvpAmp *amp);
void make_spark_at(s32 x,s32 y,int blood);
void explosion_at(s32 x,s32 y);
void avp_amp_set_chase_callback(void (*fn)(AvpAmp *amp,s32 target_x,s32 target_y));

extern u16 ngens;
extern u16 avp_reset_counter;
void avp_amp_set_reset_counter(u16 counter);
AvpAmp *weapon_hit(AvpAmp *projectile);
void cgenmode(AvpAmp *amp);
void generate(AvpAmp *amp);
void side_shield(AvpAmp *amp,s32 dx,s32 dy);
AvpAmp *make_shield(AvpAmp *owner,s32 dx,s32 dy);
void shield2(AvpAmp *amp);
void genmode(AvpAmp *amp);
void exp_handle(AvpAmp *amp);

#endif
