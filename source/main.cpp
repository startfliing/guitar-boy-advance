#include "tonc.h"
#include "maxmod.h"
#include "math.h"

#include "terminal.hpp"
#include "mode7.hpp"

#include "image.h"
#include "rainbow.h"
#include "soundbank.h"
#include "soundbank_bin.h"

#include "careless.h"

#define CONTROLS_ENABLED false

void m7_rotate(M7_CAM *cam, int phi, int theta)
{
	theta= clamp(theta, -0x3FFF, 0x4001);
	cam->phi= phi;
	cam->theta= theta;

	FIXED cf, sf, ct, st;

	cf= lu_cos(phi)>>4;		sf= lu_sin(phi)>>4;
	ct= lu_cos(theta)>>4;	st= lu_sin(theta)>>4;

	// camera X-axis (right)
	cam->u.x= cf;
	cam->u.y= 0;
	cam->u.z= sf;

	// camera Y-axis (up)
	cam->v.x= sf*st>>8;
	cam->v.y= ct;
	cam->v.z= -cf*st>>8;

	// camera Z-axis (back)
	cam->w.x= -sf*ct>>8;
	cam->w.y= st;
	cam->w.z= cf*ct>>8;
}

M7_CAM m7_cam_default ={
	{ 0x0FFFF, 0x4900, 0x7FFFFFFF },
	0x9C0, 0x000,
	{ 256, 0, 0 }, {0, 256, 0}, {0, 0, 256}
}; 

void m7_prep_horizon(M7_LEVEL *level){
	int horz;
	M7_CAM *cam= level->camera;

	if(cam->v.y != 0)
	{
		horz= M7_FAR_BG*cam->w.y - cam->pos.y;
		horz= M7_TOP - Div(horz*M7_D, M7_FAR_BG*cam->v.y);
	}
	else horz= cam->w.y > 0 ? INT_MIN : INT_MAX;

	level->horizon= horz;
}

void m7_update_sky(const M7_LEVEL *level)
{
	REG_BG2HOFS= (level->camera->phi>>6)+M7_LEFT;
	REG_BG2VOFS= -m7_horizon_line(level)-1;
}

void m7_init(M7_LEVEL *level, M7_CAM *cam, BG_AFFINE bgaff[],
	M7_SPRITE sprites[], u16 floorcnt)
{
	level->camera= cam;
	level->bgaff= bgaff;
	level->sprites= sprites;
	level->bgcnt_floor= floorcnt;
	level->horizon= 80;

    m7_rotate(cam, cam->phi, cam->theta);

	REG_BG2CNT= floorcnt;
	REG_BG_AFFINE[2]= bg_aff_default;
}

M7_CAM m7_cam;
BG_AFFINE m7_bgaffs[SCREEN_HEIGHT+1];
M7_SPRITE m7_sprites[24];

M7_LEVEL m7_level;

void m7_translate_level(M7_CAM *cam, const VECTOR *dir)
{
	cam->pos.x += (cam->u.x * dir->x - cam->u.z * dir->z)>>8;
	cam->pos.y += dir->y;
	cam->pos.z += (cam->u.z * dir->x + cam->u.x * dir->z)>>8;
}

FIXED VEL_H = careless_BPM;
FIXED curr_z = 0x7FFFFFFF;

void input_game()
{
	const FIXED OMEGA= 0x140;
	VECTOR dir= {0, 0, 0};
	M7_CAM *cam= m7_level.camera;

	if(CONTROLS_ENABLED){
		dir.z= -VEL_H*key_tri_fire();	// B/A : back/forward

		// Change camera orientation
		m7_rotate(cam,
			cam->phi,	// look left/right
			cam->theta - OMEGA*key_tri_vert());	// look up.down

		m7_translate_level(cam, &dir);
	}else{
		curr_z -= VEL_H;
		//crazy equation to balance tempo and highway speed
		cam->pos.z = ((cam->u.x * (curr_z/871)) - 700000)>>4;
	}

}

int main(){

    // Init mode 7
	m7_init(&m7_level, &m7_cam_default, m7_bgaffs, m7_sprites,
		BG_CBB(2) | BG_SBB(24) | BG_AFF_64x64 | BG_WRAP | BG_PRIO(2));

    //load palette
    memcpy16(pal_bg_mem, imagePal, imagePalLen/2);

    //load tiles
    LZ77UnCompVram(imageTiles, tile_mem[2]);
    
    //load image
    memcpy16(&se_mem[24], imageMap, imageMapLen/2);

	//memcpy16(&pal_bg_mem[16], rainbowPal, rainbowPalLen/2);
	//LZ77UnCompVram(rainbowTiles, tile_mem[0]); 
	//for(int i = 0; i < 1024; i++){
	//	memset16(&se_mem[31][i], ((u16*)(rainbowMap))[i] | SE_PALBANK(1), 1);
	//}

	//REG_BG1CNT = BG_BUILD(0, 31, 0, 0, 3, 0, 0);
    //enable Text BG
    REG_BG1CNT = Terminal::setCNT(1, 0, 31);
    REG_DISPCNT = DCNT_BG1 | DCNT_BG2 | DCNT_MODE1;

    // Initialize Interrupts
    irq_init(nullptr);

	// Maxmod requires the vblank interrupt to reset sound DMA. 
	irq_set( II_VBLANK, mmVBlank, 0);
    irq_add(II_HBLANK, (fnptr)m7_hbl_floor);
	irq_enable(II_VBLANK);

	// initialise maxmod with soundbank and 8 channels
    mmInitDefault( (mm_addr)soundbank_bin, 16 );
    
    //Setup is done. Lets put it into action!
    Terminal::log("Hello World!");
    mmStart( MOD_CARELESS_WHISPER, MM_PLAY_LOOP );

    while(1){
        input_game();

        m7_prep_horizon(&m7_level);
		m7_update_sky(&m7_level);

		//update_sprites();
		m7_prep_affines(&m7_level);

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