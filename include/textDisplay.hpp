#ifndef __TEXT_DISPLAY__
#define __TEXT_DISPLAY__

#include "tonc.h"
#include "textSpriteWidths.hpp"

#define CHAR_TO_IND(c) ((int)(c) - 31)
#define NUM_TO_CHAR(n) ((char)(n+48))

class textDisplay{
    private:
        u16 xLevel;
        u16 yLevel;
        u16 endSprite;
        u16 index;
        bool left_aligned;

        void erase();

        u16 createSprite(char c, u16 curr_width);
        u16 createSprite(u32 num, u16 curr_width);


    public:
        textDisplay(u16 x, u16 y, u16 index, bool left_aligned = true);

        ~textDisplay();

        void update(u32 streak);

        void update(u32 notes_hit, u32 total_notes){
            //erase last text
            erase();
            //get width of both numbers and the slash
            u8 curr_width = xLevel;
            if(!left_aligned){
                u8 total_width = 0;
                u32 num = notes_hit;
                do{
                    int charInd = num%10 + 17;
                    total_width += textSpriteWidths[charInd];
                    num /= 10;
                }while(num > 0);
                total_width += textSpriteWidths[CHAR_TO_IND('/')]; //width of slash
                for(u32 num = total_notes; num > 0; num /= 10){
                    int charInd = num%10 + 17;
                    total_width += textSpriteWidths[charInd];
                }
                curr_width = xLevel - total_width;
            }

            curr_width += createSprite(notes_hit, curr_width);
            curr_width += createSprite('/', curr_width);
            curr_width += createSprite(total_notes, curr_width);
        }

};

#endif