#include "tonc.h"
#include "maxmod.h"
#include "math.h"

#include "terminal.hpp"
#include "mode7.hpp"
#include "noteManager.hpp"
#include "notes.h"

#include "image.h"
#include "palette.h"
#include "soundbank.h"
#include "soundbank_bin.h"

#include "careless.h"
#include "buttons.h"

#define CONTROLS_ENABLED false
#define BLENDING true

NoteManager<careless_expertsingle_count>* nm = new NoteManager<careless_expertsingle_count>(
	careless_expertsingle_data,
	careless_BPM
);

FIXED VEL_H = CONTROLS_ENABLED ? 0x200 : careless_BPM;

//SPRITES WILL MOVE TO NEW FILE LATER, JUST PUTTING THEM HERE FOR NOW

#define SPR_COUNT	32

u8 sort_ids[SPR_COUNT];
int sort_keys[SPR_COUNT];

//LANE POSITIONS
//m7_level.camera->pos.z - 25000
//{ 0x0E300, 0x0000, -196632/*0x25682FA*/ }, 	// Mario
//{ 0x0F400, 0x0000, -328280 }, 	// Luigi
//{ 0x10400, 0x0000, -459928 }, 	// Princess
//{ 0x11600, 0x0000, -591576 }, 	// Yoshi


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

	//check the next 22 sprites,
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

//------------------------------------------

FIXED curr_z = 0;

void input_game()
{
	const FIXED OMEGA= 0x140;
	VECTOR dir= {0, 0, 0};
	M7_CAM *cam= m7_level.camera;

	if(CONTROLS_ENABLED){
		dir.z= -VEL_H*key_tri_fire();	// B/A : back/forward
		dir.x= VEL_H*key_tri_shoulder();	// strafe

		// Change camera orientation
		m7_rotate(cam,
			cam->phi + OMEGA*key_tri_horz(),	// look left/right
			cam->theta - OMEGA*key_tri_vert());	// look up.down

		m7_translate_level(cam, &dir);
	}else{
		curr_z += VEL_H;
		//crazy equation to balance tempo and highway speed
		cam->pos.z = ((cam->u.x * (-curr_z/871)) - 640000)>>4;
	}

}

//green button 	(3, 16)
//red button 	(9, 16)
//yellow button (15, 16)
//blue button 	(21, 16)
void drawButton(int b, bool active){
	int palette = b+1;
	int start_x = (b*6)+3;
	int start_ind = 32*13 + start_x;
	//y
	for(int i = 0; i < 3; i++){
		int row_start = start_ind + (i*32);
		//x
		for(int j = 0; j < 6; j++){
			u16 map_start = active ? buttonsMap[(i*6)+j] + 18 : buttonsMap[(i*6)+j];
			se_mem[30][row_start+j] = map_start | SE_PALBANK(palette);
		}

	}
}

void drawButtons(){
	//iterate through buttons
	for(int b = 0; b < 4; b++){
		drawButton(b, false);
	}
}

void updateButtons(u16 hit, u16 released){
	u16 buttons[4] = {KEY_L, KEY_B, KEY_A, KEY_R};
	for(int i = 0; i < 4; i ++){
		if( buttons[i] & hit ){
			drawButton(i, true);
		}

		if( buttons[i] & released ){
			drawButton(i, false);
		}
	}
}

void update(){
	mmVBlank();
	recent_sprite = nm->update();
}

int main(){

    // Init mode 7
	m7_init(&m7_level, &m7_cam_default, m7_bgaffs, m7_sprites,
		BG_CBB(2) | BG_SBB(24) | BG_AFF_64x64 | BG_WRAP | BG_PRIO(2));

    //load palette
    memcpy16(pal_bg_mem, palettePal, palettePalLen/2);

    //load tiles
    LZ77UnCompVram(imageTiles, tile_mem[2]);
    
    //load image
    memcpy16(&se_mem[24], imageMap, imageMapLen/2);

	memcpy16(&tile_mem[1], buttonsTiles, buttonsTilesLen/2);
	drawButtons();

	if(BLENDING){
		memset16(&se_mem[31], 0x0001, 32*32);
		memset16(&tile_mem[0][1], 0x6666, 16);
		memset16(&tile_mem[1][0x4F], 0x6666, 16);
		memset16(&se_mem[30], 0x004F, 32*3);
		REG_BLDCNT= BLD_BUILD(BLD_BG1, BLD_OBJ | BLD_BG2, 1);
	}
	

	REG_BG0CNT = BG_BUILD(1, 30, 0, 0, 0, 0, 0);
	
    //enable Text BG
    REG_BG1CNT = BLENDING ? BG_BUILD(0, 31, 0, 0, 0, 0, 0) : Terminal::setCNT(1, 0, 31);
    REG_DISPCNT = DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_MODE1;

	memcpy16(pal_obj_mem, palettePal, palettePalLen/2);
	memcpy16(tile_mem_obj, notesTiles, notesTilesLen/2);

	oam_init(obj_mem, 128);
	init_sprites();
    // Initialize Interrupts
    irq_init(nullptr);

	// Maxmod requires the vblank interrupt to reset sound DMA. 
	irq_set( II_VBLANK, update, 0);
    irq_add(II_HBLANK, (fnptr)m7_hbl_floor);
	irq_enable(II_VBLANK);

	// initialise maxmod with soundbank and 8 channels
    mmInitDefault( (mm_addr)soundbank_bin, 16 );
    
    //Setup is done. Lets put it into action!
    //Terminal::log("Hello World! %%", careless_expertsingle_data[0].tick);
    mmStart( MOD_CARELESS_WHISPER, MM_PLAY_LOOP );

    while(1){
        input_game();

        m7_prep_horizon(&m7_level);
		m7_update_sky(&m7_level);

		update_sprites();
		m7_prep_affines(&m7_level);

		u16 buttons_hit = key_hit(KEY_L | KEY_A | KEY_B | KEY_R);
		u16 buttons_released = key_released(KEY_L | KEY_A | KEY_B | KEY_R);
		u16 buttons_down = key_is_down(KEY_L | KEY_A | KEY_B | KEY_R);
		updateButtons(buttons_hit, buttons_released);

		//if(key_hit(KEY_START)) Terminal::log("Tick = %%", m7_level.camera->pos.z);

        //update song
        mmFrame();

        //update random nunmber
        qran();

        //poll what keys are down
        key_poll();

        //helps with visual tearing
        vid_vsync();
    }
}