#include "tonc.h"
#include "textDisplay.hpp"

textDisplay::textDisplay(u16 x, u16 y){
    this->xLevel = x;
    this->yLevel = y;
    endSprite = 0;
}

textDisplay::~textDisplay(){
    
}

#define CHAR_TO_IND(c) ((int)(c) - 31)

void textDisplay::update(const char* txt){
    if(endSprite > 32){
        for(u8 i = 32; i < endSprite; i++){
            oam_mem[i].attr0 = ATTR0_HIDE;
        }
    }
    char* head = (char*)txt;
    u8 sprite_num = 32;
    u8 curr_width = xLevel; // apply centering
    while(*head != '\0'){
        //handle spaces
        if(*head == ' '){
            curr_width += 8;
            head++;
            continue;
        }

        int charInd = CHAR_TO_IND(*head);
        obj_set_attr(
            &oam_mem[sprite_num++], 
            ATTR0_SQUARE | ATTR0_BLEND | ATTR0_Y(yLevel),
            ATTR1_SIZE_16 | ATTR1_X(curr_width),
            (charInd*4) + 0x201
        );
        curr_width += textSpriteWidths[charInd];
        head++;
    }
    endSprite = sprite_num;
}