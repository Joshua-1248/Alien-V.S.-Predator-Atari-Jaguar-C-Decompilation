#ifndef AVP_ROM_BOOTSTRAP_H
#define AVP_ROM_BOOTSTRAP_H
#include "avp_types.h"

typedef int (*AvpRomInitFn)(void *user);
typedef int (*AvpRomInflateFn)(void *user, const u8 *input, u8 **output_io);
typedef void (*AvpRomJumpFn)(void *user, void *entry);
typedef void (*AvpRomPrepareHardwareFn)(void *user);

typedef struct AvpRomBootstrapOps {
    AvpRomPrepareHardwareFn prepare_hardware;
    AvpRomInitFn init_allocator;
    AvpRomInitFn init_inflater;
    AvpRomInflateFn inflate_stream;
    AvpRomJumpFn jump_to_entry;
    void *user;
} AvpRomBootstrapOps;

typedef struct AvpRomBootstrapImage {
    const u8 *text_gzip;
    const u8 *data_gzip;
    u8 *ram_start;
    void *entry;
} AvpRomBootstrapImage;

/* Readable representation of MAIN/ROM.S orchestration. Returns zero on the
 * success path; a nonzero inflate/init result is propagated to the caller. */
int avp_rom_bootstrap_run(const AvpRomBootstrapOps *ops,
                          const AvpRomBootstrapImage *image);

#endif
