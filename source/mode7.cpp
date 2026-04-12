#include "mode7.hpp"


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

M7_CAM m7_cam_default = {
	{ 0x10000, 0x2000, 0x7FFFFFFF },
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
M7_SPRITE m7_sprites[32];

M7_LEVEL m7_level;

// Note manager for guitar hero gameplay
//NoteManager note_manager;

void m7_translate_level(M7_CAM *cam, const VECTOR *dir)
{
	cam->pos.x += (cam->u.x * dir->x - cam->u.z * dir->z)>>8;
	cam->pos.y += dir->y;
	cam->pos.z += (cam->u.z * dir->x + cam->u.x * dir->z)>>8;
}
