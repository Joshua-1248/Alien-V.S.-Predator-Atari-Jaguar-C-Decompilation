/* Machine-code-derived compatibility reconstruction.
 * NOT claimed as literal original Club Drive/Rebellion source text.
 */
extern unsigned char *slide;
extern unsigned char *outptr;
extern void loadgpu(const void *);
extern unsigned char gpunzip[];
extern void *malloc(unsigned short);

void flush(unsigned short n)
{
    unsigned char *dst = outptr;
    unsigned char *src = slide;
    outptr += n;
    while (n--) *dst++ = *src++;
}

unsigned long m2il(const unsigned char *p)
{
    return ((unsigned long)p[0]) |
           ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24);
}

int init_in(void)
{
    loadgpu(gpunzip);
    slide = (unsigned char *)malloc(0x8000);
    return slide == 0;
}
