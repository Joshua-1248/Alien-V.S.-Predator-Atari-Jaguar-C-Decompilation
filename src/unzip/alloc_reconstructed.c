/*
 * Compatibility reconstruction from the final AvP Jaguar alloc.o machine code.
 * NOT claimed to be the literal original Club Drive/Rebellion ALLOC.C text.
 *
 * Historical behavior recovered at M64:
 *   - 8-byte Header: pointer/magic at +0, 32-bit unit count at +4.
 *   - allocation magic 0x000A110C is written into the first long of allocated headers.
 *   - free() validates that magic and calls fatal() on a mismatch.
 *   - morecore() rounds requests to 256 Header units and obtains bytes via sbrk().
 *   - sbrk() lazily establishes an arena at align8(_alloc_b), size 0x20000,
 *     clamped so its end never exceeds address 0x00200000.
 *   - sbrk() returns NULL on failure (not (void *)-1).
 *   - init_alloc() takes no arguments and clears all allocator state.
 */

typedef union Header Header;
union Header {
    struct {
        Header *next;
        unsigned long units;
    } s;
    unsigned long align;
};

#define ALLOC_MAGIC ((Header *)0x000A110CUL)
#define NALLOC_UNITS 256UL
#define ARENA_BYTES  0x00020000UL
#define ARENA_LIMIT  ((unsigned char *)0x00200000UL)

extern unsigned char _alloc_b[];
extern void fatal(unsigned long);

static unsigned short alloc_started;
static Header base;
static Header *freep;

/* Link-visible in the historical ROM symbol table. */
unsigned char *memptr;
unsigned char *memtop;

void free(void *ap);
void *sbrk(long nbytes);

static Header *morecore(unsigned long nu)
{
    Header *up;

    nu = (nu + 255UL) & ~255UL;
    up = (Header *)sbrk((long)(nu * sizeof(Header)));
    if (up == 0)
        return 0;

    up->s.units = nu;
    up->s.next = ALLOC_MAGIC;
    free((void *)(up + 1));
    return freep;
}

void *malloc(unsigned long nbytes)
{
    Header *p, *prevp;
    unsigned long nunits;

    /* Exact opening arithmetic visible in alloc.o. */
    nunits = ((nbytes + 15UL) >> 3) + 1UL;

    prevp = freep;
    if (prevp == 0) {
        base.s.next = &base;
        freep = prevp = &base;
        base.s.units = 0;
    }

    for (p = prevp->s.next;; prevp = p, p = p->s.next) {
        if (p->s.units >= nunits) {
            if (p->s.units == nunits) {
                prevp->s.next = p->s.next;
            } else {
                p->s.units -= nunits;
                p += p->s.units;
                p->s.units = nunits;
            }
            freep = prevp;
            p->s.next = ALLOC_MAGIC;
            return (void *)(p + 1);
        }
        if (p == freep) {
            p = morecore(nunits);
            if (p == 0)
                return 0;
        }
    }
}

void free(void *ap)
{
    Header *bp, *p;

    if (ap == 0)
        return;

    bp = (Header *)ap - 1;

    if (bp->s.next != ALLOC_MAGIC)
        fatal((unsigned long)bp->s.next);

    /* Exact object clears the magic before free-list insertion. */
    bp->s.next = 0;

    for (p = freep; !(bp > p && bp < p->s.next); p = p->s.next) {
        if (p >= p->s.next && (bp > p || bp < p->s.next))
            break;
    }

    if (bp + bp->s.units == p->s.next) {
        bp->s.units += p->s.next->s.units;
        bp->s.next = p->s.next->s.next;
    } else {
        bp->s.next = p->s.next;
    }

    if (p + p->s.units == bp) {
        p->s.units += bp->s.units;
        p->s.next = bp->s.next;
    } else {
        p->s.next = bp;
    }

    freep = p;
}

void *sbrk(long nbytes)
{
    unsigned char *old;
    unsigned char *candidate;

    if (!alloc_started) {
        unsigned long start = (unsigned long)_alloc_b;
        unsigned long end;

        if ((long)start <= 0)
            return 0;

        start = (start + 7UL) & ~7UL;
        memptr = (unsigned char *)start;

        /* Machine code computes start + 0x20000 then clamps to 0x200000. */
        end = start + ARENA_BYTES;
        memtop = (unsigned char *)end;
        if (memtop > ARENA_LIMIT)
            memtop = ARENA_LIMIT;

        alloc_started = 1;
    }

    candidate = memptr + nbytes;
    if (candidate > memtop)
        return 0;

    old = memptr;
    memptr = candidate;
    return old;
}

void init_alloc(void)
{
    alloc_started = 0;
    memptr = 0;
    memtop = 0;
    freep = 0;
    base.s.units = 0;
    base.s.next = 0;
}
