#ifndef __MODE_7__
#define __MODE_7__

#include "tonc.h"

#define M7_D		256		//!< Focal length
#define M7_D_SHIFT	  8		//!< Focal shift
#define M7O_NORM	  2		//!< Object renormalization shift (by /4)

// View frustrum limits
#define M7_LEFT		(-120)		//!< Viewport left
#define M7_RIGHT	 120		//!< Viewport right
#define M7_TOP		  80		//!< Viewport top (y-axis up)
#define M7_BOTTOM	(-80)		//!< Viewport bottom (y-axis up!)
#define M7_NEAR		  24		//!< Near plane (objects)
#define M7_FAR		 512		//!< Far plane (objects)

#define M7_FAR_BG	 768		//!< Far plane (floor)

#define INT_MIN     (-2147483647 - 1)
#define INT_MAX       2147483647

//! 3D sprite struct
typedef struct M7_SPRITE
{
	VECTOR pos;		//!< World position.
	POINT anchor;	//!< Sprite anchor.
	OBJ_ATTR obj;	//!< Object attributes.
	s16 phi;		//!< Azimuth angle.
	u8 obj_id;		//!< Object index.
	u8 aff_id;		//!< OBJ_AFFINE index.
	TILE *tiles;	//!< Gfx pointer.
	VECTOR pos2;	//!< Position in cam space (subject to change)
	// To add: shade object
} M7_SPRITE;

typedef struct M7_CAM
{
	VECTOR pos;		//!< World position.
	int theta;		//!< Polar angle.
	int phi;		//!< Azimuth angle.
	VECTOR u;		//!< local x-axis (right)
	VECTOR v;		//!< local y-axis (up)
	VECTOR w;		//!< local z-axis (back)
} M7_CAM;

//! One struct to bind them all
typedef struct M7_LEVEL
{
	M7_CAM *camera;			//!< Camera variables
	BG_AFFINE *bgaff;		//!< Affine parameter array
	M7_SPRITE *sprites;		//!< 3D sprites
	int horizon;			//!< Horizon scanline (sorta)
	u16 bgcnt_sky;			//!< BGxCNT for backdrop
	u16 bgcnt_floor;		//!< BGxCNT for floor
} M7_LEVEL;

extern M7_LEVEL m7_level;

INLINE int m7_horizon_line(const M7_LEVEL *level);

// IWRAM functions
IWRAM_CODE void m7_prep_affines(M7_LEVEL *level);
IWRAM_CODE void m7_prep_sprite(M7_LEVEL *level, M7_SPRITE *spr);

IWRAM_CODE void m7_hbl_floor();


INLINE int m7_horizon_line(const M7_LEVEL *level)
{	return clamp(level->horizon, 0, 228);	}

#endif