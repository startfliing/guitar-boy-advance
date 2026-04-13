#include "sprite.hpp"

#include "notes.h"

u8 sort_ids[SPR_COUNT];
int sort_keys[SPR_COUNT];

int first_sprite = SPR_COUNT;
int recent_sprite = 0;

void init_sprite(int ii, noteSprite sprite){
	M7_SPRITE *spr= &m7_level.sprites[ii];
	spr->pos= sprite.pos;
	spr->anchor.x= 16;
	spr->anchor.y= 30;
	obj_set_attr(&spr->obj, 
		ATTR0_SQUARE | ATTR0_AFF_DBL | ATTR0_BLEND,
		ATTR1_SIZE_64x32 | ATTR1_AFF_ID(ii), 
		ATTR2_ID(1) | ATTR2_PRIO(2) | ATTR2_PALBANK(sprite.pal));

	spr->obj_id= ii;
	spr->aff_id= ii;
	spr->tiles= (TILE*)notesTiles;
	//Terminal::log("Init sprite %% at z: %%", ii, spr->pos.z);
};

void update_sprites()
{
	int ii;

	M7_SPRITE *spr= m7_level.sprites;

	//check the next 30 sprites,
	if(first_sprite != recent_sprite+SPR_COUNT-2){
		noteSprite* positions = nm->getNoteSprites();
		for(ii=first_sprite; ii<recent_sprite+SPR_COUNT-2; ii++){
			init_sprite(ii%SPR_COUNT, positions[ii]);
		}
		first_sprite = recent_sprite;
	}

	for(ii=0; ii<SPR_COUNT; ii++)
	{
		m7_prep_sprite(&m7_level, &spr[ii]);

		// Create sort key
		if(BFN_GET2(spr[ii].obj.attr0, ATTR0_MODE) != ATTR0_HIDE){
			sort_keys[ii]= spr[ii].pos2.z;
		} else
			sort_keys[ii]= INT_MAX;
	}

	id_sort_shell(sort_keys, sort_ids, SPR_COUNT);

	// Update real OAM
	for(ii=0; ii<SPR_COUNT; ii++)
		obj_copy(&oam_mem[ii], &spr[sort_ids[ii]].obj, 1);
}

void init_sprites()
{
	int ii;
	noteSprite* pos = nm->getNoteSprites();
	// Notes
	for(ii=0; ii<SPR_COUNT; ii++)
	{
		init_sprite(ii, pos[ii]);
	}

	// Setup sorting list
	for(ii=0; ii<SPR_COUNT; ii++)
		sort_ids[ii]= ii;
}