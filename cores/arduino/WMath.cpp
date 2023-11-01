#include "tal_system.h"

long random(long howbig)
{
    UINT_T rang = 0;

    if (howbig == 0) {
        return 0;
    }

    rang = howbig & 0xFFFFFFFF;

    return tal_system_get_random(rang);
}

long random(long howsmall, long howbig)
{
    if (howsmall >= howbig) {
        return howsmall;
    }
    long diff = howbig - howsmall;
    return random(diff) + howsmall;
}

void randomSeed(unsigned long seed)
{
    // not support
    return;
}
