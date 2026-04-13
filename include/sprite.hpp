#ifndef __SPRITE__
#define __SPRITE__

#include "tonc.h"
#include "mode7.hpp"
#include "noteManager.hpp"
#include "careless.h"

#define SPR_COUNT	32

extern NoteManager<careless_expertsingle_count>* nm;
extern int first_sprite;
extern int recent_sprite;

void init_sprite(int ii, noteSprite sprite);
void init_sprites();
void update_sprites();


#endif