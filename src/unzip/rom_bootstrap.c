/* Readable C representation of MAIN/ROM.S bootstrap orchestration.
 * Hardware setup, allocator/inflater implementation, and final transfer of
 * control are explicit seams so a hosted build never dereferences Jaguar ROM
 * or RAM addresses directly.
 */
#include "rom_bootstrap.h"

int avp_rom_bootstrap_run(const AvpRomBootstrapOps *ops,
                          const AvpRomBootstrapImage *image)
{
    u8 *out;
    int rc;
    if (!ops || !image || !ops->init_allocator || !ops->init_inflater ||
        !ops->inflate_stream) return -1;

    if (ops->prepare_hardware) ops->prepare_hardware(ops->user);

    rc=ops->init_allocator(ops->user);
    if (rc) return rc;
    rc=ops->init_inflater(ops->user);
    if (rc) return rc;

    out=image->ram_start;
    rc=ops->inflate_stream(ops->user,image->text_gzip,&out);
    if (rc) return rc;
    rc=ops->inflate_stream(ops->user,image->data_gzip,&out);
    if (rc) return rc;

    if (ops->jump_to_entry) ops->jump_to_entry(ops->user,image->entry);
    return 0;
}
