#include "tonc.h"
#include "textDisplay.hpp"

textDisplay::textDisplay(u16 x, u16 y, u16 index, bool left_aligned){
    this->xLevel = x;
    this->yLevel = y;
    this->index = index;
    this->endSprite = 0;
    this->left_aligned = left_aligned;
}

textDisplay::~textDisplay(){
    
}

void textDisplay::erase(){
    if(endSprite > index){
        for(u8 i = index; i < endSprite; i++){
            oam_mem[i].attr0 = ATTR0_HIDE;
        }
    }
    endSprite = index;
}   

u16 textDisplay::createSprite(char c, u16 curr_width){
    u16 charInd = CHAR_TO_IND(c);
    obj_set_attr(
        &oam_mem[endSprite++], 
        ATTR0_SQUARE | ATTR0_Y(yLevel),
        ATTR1_SIZE_16 | ATTR1_X(curr_width),
        ((charInd*4) + 0x201) | ATTR2_PRIO(0)
    );
    return textSpriteWidths[charInd];
}

u16 textDisplay::createSprite(u32 num, u16 curr_width){
    u16 start_val = curr_width;
    u32 tens = num / 10;
    if(num > 9){
        start_val += createSprite(tens, curr_width);
    }
    char charInd = NUM_TO_CHAR(num % 10);
    start_val += createSprite(charInd, start_val);
    return start_val - curr_width;

}

void textDisplay::update(u32 streak){
    //erase last text
    erase();
    
    const char* txt = "Streak: ";
    u8 curr_width = xLevel;
    if(!left_aligned){
        char* head = (char*)txt;
        u8 total_width = 0;
        while(*head != '\0'){
            int charInd = CHAR_TO_IND(*head);
            total_width += textSpriteWidths[charInd];
            head++;
        }
        for(u32 num = streak; num > 0; num /= 10){
            int charInd = num%10 + 17;
            total_width += textSpriteWidths[charInd];
        }
        curr_width = xLevel - total_width;
    }

    char* head = (char*)txt;
    while(*head != '\0'){
        //handle spaces
        if(*head == ' '){
            curr_width += 8;
            head++;
            continue;
        }

        curr_width += createSprite(*head, curr_width);

        head++;
    }
    createSprite(streak, curr_width);
}