#ifndef __TEXT_DISPLAY__
#define __TEXT_DISPLAY__

#include "tonc.h"
#include "textSpriteWidths.hpp"

class textDisplay{
    private:
        u16 xLevel;
        u16 yLevel;
        u16 endSprite;

    public:
        textDisplay(u16 x, u16 y);

        ~textDisplay();

        void update(const char* txt);

};

#endif