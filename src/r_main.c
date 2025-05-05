// Copyright (C) 1996-1997 Id Software, Inc. GPLv3 See LICENSE for details.

#include "quakedef.h"
#include "r_local.h"

#define	VIEWMODNAME_LENGTH 256

byte r_foundtranswater, r_wateralphapass; // Manoel Kasimier - translucent water
int r_pass; // CyanBun96: 1 - cutout textures 0 - everything else
void *colormap;
vec3_t viewlightvec;
alight_t r_viewlighting = { 128, 192, viewlightvec };
float r_time1;
int r_numallocatededges;
qboolean r_recursiveaffinetriangles = true;
int r_pixbytes = 1;
float r_aliasuvscale = 1.0;
int r_outofsurfaces;
int r_outofedges;
qboolean r_dowarp, r_dowarpold, r_viewchanged;
int numbtofpolys;
btofpoly_t *pbtofpolys;
mvertex_t *r_pcurrentvertbase;
int c_surf;
int r_maxsurfsseen, r_maxedgesseen, r_cnumsurfs;
qboolean r_surfsonstack;
int r_clipflags;
byte *r_warpbuffer;
qboolean r_fov_greater_than_90;
// view origin
vec3_t vup, base_vup;
vec3_t vpn, base_vpn;
vec3_t vright, base_vright;
vec3_t r_origin;
// screen size info
refdef_t r_refdef;
float xcenter, ycenter;
float xscale, yscale;
float xscaleinv, yscaleinv;
float xscaleshrink, yscaleshrink;
float aliasxscale, aliasyscale, aliasxcenter, aliasycenter;
int screenwidth;
float pixelAspect;
float screenAspect;
float verticalFieldOfView;
float xOrigin, yOrigin;
mplane_t screenedge[4];
// refresh flags
unsigned int r_framecount = 1; // so frame counts initialized to 0 don't match
unsigned int r_visframecount;
int d_spanpixcount;
int r_polycount;
int r_drawnpolycount;
int r_wholepolycount;
char viewmodname[VIEWMODNAME_LENGTH + 1];
int modcount;
int *pfrustum_indexes[4];
int r_frustum_indexes[4 * 6];
int reinit_surfcache = 1; // if 1, surface cache is currently empty and
			// must be reinitialized for current cache size
mleaf_t *r_viewleaf, *r_oldviewleaf;
texture_t *r_notexture_mip;
float r_aliastransition, r_resfudge;
int d_lightstylevalue[256]; // 8.8 fraction of base light value
float dp_time1, dp_time2, db_time1, db_time2, rw_time1, rw_time2;
float se_time1, se_time2, de_time1, de_time2, dv_time1, dv_time2;
int colored_aliaslight;

cvar_t r_draworder = { "r_draworder", "0", false, false, 0, NULL };
cvar_t r_speeds = { "r_speeds", "0", false, false, 0, NULL };
cvar_t r_timegraph = { "r_timegraph", "0", false, false, 0, NULL };
cvar_t r_graphheight = { "r_graphheight", "10", false, false, 0, NULL };
cvar_t r_clearcolor = { "r_clearcolor", "2", false, false, 0, NULL };
cvar_t r_waterwarp = { "r_waterwarp", "1", false, false, 0, NULL };
cvar_t r_fullbright = { "r_fullbright", "0", false, false, 0, NULL };
cvar_t r_drawentities = { "r_drawentities", "1", false, false, 0, NULL };
cvar_t r_drawviewmodel = { "r_drawviewmodel", "1", false, false, 0, NULL };
cvar_t r_aliasstats = { "r_polymodelstats", "0", false, false, 0, NULL };
cvar_t r_dspeeds = { "r_dspeeds", "0", false, false, 0, NULL };
cvar_t r_drawflat = { "r_drawflat", "0", false, false, 0, NULL };
cvar_t r_ambient = { "r_ambient", "0", false, false, 0, NULL };
cvar_t r_reportsurfout = { "r_reportsurfout", "0", false, false, 0, NULL };
cvar_t r_maxsurfs = { "r_maxsurfs", "0", false, false, 0, NULL };
cvar_t r_numsurfs = { "r_numsurfs", "0", false, false, 0, NULL };
cvar_t r_reportedgeout = { "r_reportedgeout", "0", false, false, 0, NULL };
cvar_t r_maxedges = { "r_maxedges", "0", false, false, 0, NULL };
cvar_t r_numedges = { "r_numedges", "0", false, false, 0, NULL };
cvar_t r_aliastransbase = { "r_aliastransbase", "200", false, false, 0, NULL };
cvar_t r_aliastransadj = { "r_aliastransadj", "100", false, false, 0, NULL };
cvar_t r_wateralpha = { "r_wateralpha", "1", true, false, 0, NULL };
cvar_t r_slimealpha = { "r_slimealpha", "1", true, false, 0, NULL };
cvar_t r_lavaalpha = { "r_lavaalpha", "1", true, false, 0, NULL };
cvar_t r_telealpha = { "r_telealpha", "1", true, false, 0, NULL };
cvar_t r_twopass = { "r_twopass", "1", true, false, 0, NULL }; // CyanBun96
	// 0 - off (smart) 1 - on (smart) 2 - force off 3 - force on
	// smart gets set on map load if cutouts were found
cvar_t r_fogstyle = { "r_fogstyle", "3", true, false, 0, NULL }; // CyanBun96
cvar_t r_nofog = { "r_nofog", "0", true, false, 0, NULL }; // CyanBun96
cvar_t r_alphastyle = { "r_alphastyle", "0", true, false, 0, NULL }; // CyanBun96
cvar_t r_entalpha = { "r_entalpha", "1", true, false, 0, NULL }; // CyanBun96
cvar_t r_labmixpal = { "r_labmixpal", "1", true, false, 0, NULL }; // CyanBun96
cvar_t r_rgblighting = { "r_rgblighting", "1", true, false, 0, NULL }; // CyanBun96
cvar_t r_fogbrightness = { "r_fogbrightness", "1", true, false, 0, NULL }; // CyanBun96

// johnfitz -- new cvars TODO actually implement these, they're currently placeholders
cvar_t  r_nolerp_list = {"r_nolerp_list", "progs/flame.mdl,progs/flame2.mdl,progs/braztall.mdl,pro gs/brazshrt.mdl,progs/longtrch.mdl,progs/flame_pyre.mdl,progs/v_saw.mdl,progs/v_xfist.mdl,progs/h2 stuff/newfire.mdl", false, false, 0, NULL};
cvar_t  r_noshadow_list = {"r_noshadow_list", "progs/proj_balllava.mdl,progs/flame2.mdl,progs/flame.mdl,progs/bolt1.mdl,progs/bolt2.mdl,progs/bolt3.mdl,progs/laser.mdl", false, false, 0, NULL};
// johnfitz
cvar_t r_fullbright_list = {"r_fullbright_list", "progs/spike.mdl,progs/s_spike.mdl,progs/missile.mdl,progs/k_spike.mdl,progs/proj_balllava.mdl,progs/flame2.mdl,progs/flame.mdl,progs/bolt1.mdl,progs/bolt2.mdl,progs/bolt3.mdl,progs/laser.mdl", false, false, 0, NULL};

extern cvar_t scr_fov;
extern float fog_density;
extern int fog_initialized;
extern float cur_ent_alpha;

extern void R_DrawFog();
extern void Fog_FogCommand_f();
extern void Fog_ParseWorldspawn();
extern void R_InitSkyBox(); // Manoel Kasimier - skyboxes 
extern void Sky_NewMap();
extern void build_color_mix_lut(cvar_t *cvar);
extern qboolean nameInList(const char *list, const char *name);

void CreatePassages();
void SetVisibilityByPassages();
void R_MarkLeaves();
void Sky_Init();
void Fog_SetPalIndex(cvar_t *cvar);

void R_InitTextures()
{ // create a simple checkerboard texture for the default
	r_notexture_mip = Hunk_AllocName(sizeof(texture_t)
			+ 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2, "notexture");
	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof(texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16 * 16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8 * 8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4 * 4;
	for (int m = 0; m < 4; m++) {
		byte *dest = (byte *) r_notexture_mip
			+ r_notexture_mip->offsets[m];
		for (int y = 0; y < (16 >> m); y++)
			for (int x = 0; x < (16 >> m); x++) {
				if ((y < (8 >> m)) ^ (x < (8 >> m)))
					*dest++ = 0;
				else
					*dest++ = 0xff;
			}
	}
}

void R_Init()
{
	R_InitTurb();
	Cmd_AddCommand("timerefresh", R_TimeRefresh_f);
	Cmd_AddCommand("pointfile", R_ReadPointFile_f);
	Cmd_AddCommand("fog", Fog_FogCommand_f);
	Cvar_RegisterVariable(&r_draworder);
	Cvar_RegisterVariable(&r_speeds);
	Cvar_RegisterVariable(&r_timegraph);
	Cvar_RegisterVariable(&r_graphheight);
	Cvar_RegisterVariable(&r_drawflat);
	Cvar_RegisterVariable(&r_ambient);
	Cvar_RegisterVariable(&r_clearcolor);
	Cvar_RegisterVariable(&r_waterwarp);
	Cvar_RegisterVariable(&r_fullbright);
	Cvar_RegisterVariable(&r_drawentities);
	Cvar_RegisterVariable(&r_drawviewmodel);
	Cvar_RegisterVariable(&r_aliasstats);
	Cvar_RegisterVariable(&r_dspeeds);
	Cvar_RegisterVariable(&r_reportsurfout);
	Cvar_RegisterVariable(&r_maxsurfs);
	Cvar_RegisterVariable(&r_numsurfs);
	Cvar_RegisterVariable(&r_reportedgeout);
	Cvar_RegisterVariable(&r_maxedges);
	Cvar_RegisterVariable(&r_numedges);
	Cvar_RegisterVariable(&r_aliastransbase);
	Cvar_RegisterVariable(&r_aliastransadj);
	Cvar_RegisterVariable(&r_wateralpha);
	Cvar_RegisterVariable(&r_slimealpha);
	Cvar_RegisterVariable(&r_lavaalpha);
	Cvar_RegisterVariable(&r_telealpha);
	Cvar_RegisterVariable(&r_twopass);
	Cvar_RegisterVariable(&r_fogstyle);
	Cvar_RegisterVariable(&r_nofog);
	Cvar_RegisterVariable(&r_alphastyle);
	Cvar_RegisterVariable(&r_entalpha);
	Cvar_RegisterVariable(&r_labmixpal);
	Cvar_RegisterVariable(&r_rgblighting);
	Cvar_RegisterVariable(&r_fogbrightness);
	Cvar_SetCallback(&r_labmixpal, build_color_mix_lut);
	Cvar_SetCallback(&r_fogbrightness, Fog_SetPalIndex);
	Cvar_SetValue("r_maxedges", (float)NUMSTACKEDGES);
	Cvar_SetValue("r_maxsurfs", (float)NUMSTACKSURFACES);
	view_clipplanes[0].leftedge = true;
	view_clipplanes[1].rightedge = true;
	view_clipplanes[1].leftedge = view_clipplanes[2].leftedge =
	    view_clipplanes[3].leftedge = false;
	view_clipplanes[0].rightedge = view_clipplanes[2].rightedge =
	    view_clipplanes[3].rightedge = false;
	r_refdef.xOrigin = XCENTERING;
	r_refdef.yOrigin = YCENTERING;
	R_InitParticles();
	D_Init();
	Sky_Init();
}

void R_NewMap()
{
	R_InitSkyBox(); // Manoel Kasimier - skyboxes 
	// clear out efrags in case the level hasn't been reloaded
	// FIXME: is this one short?
	for (int i = 0; i < cl.worldmodel->numleafs; i++)
		cl.worldmodel->leafs[i].efrags = NULL;
	r_viewleaf = NULL;
	R_ClearParticles();
	r_cnumsurfs = r_maxsurfs.value;
	if (r_cnumsurfs <= MINSURFACES)
		r_cnumsurfs = MINSURFACES;
	if (r_cnumsurfs > NUMSTACKSURFACES) {
		surfaces =
		    Hunk_AllocName(r_cnumsurfs * sizeof(surf_t), "surfaces");
		surface_p = surfaces;
		surf_max = &surfaces[r_cnumsurfs];
		r_surfsonstack = false;
		// surface 0 doesn't really exist; it's just a dummy because
		// index 0 is used to indicate no edge attached to surface
		surfaces--;
	} else
		r_surfsonstack = true;
	r_maxedgesseen = 0;
	r_maxsurfsseen = 0;
	r_numallocatededges = r_maxedges.value;
	if (r_numallocatededges < MINEDGES)
		r_numallocatededges = MINEDGES;
	auxedges = r_numallocatededges <= NUMSTACKEDGES ? NULL
		: Hunk_AllocName(r_numallocatededges * sizeof(edge_t), "edges");
	r_dowarpold = false;
	r_viewchanged = false;
	skybox_name[0] = 0;
	r_skymade = 0;
	Sky_NewMap();
	Fog_ParseWorldspawn();
}

void R_SetVrect(vrect_t *pvrectin, vrect_t *pvrect, int lineadj)
{
	float size = scr_viewsize.value > 100 ? 100 : scr_viewsize.value;
	if (cl.intermission) {
		size = 100;
		lineadj = 0;
	}
	size /= 100;
	int h = pvrectin->height - lineadj;
	pvrect->width = pvrectin->width * size;
	if (pvrect->width < 96) {
		size = 96.0 / pvrectin->width;
		pvrect->width = 96; // min for icons
	}
	pvrect->width &= ~7;
	pvrect->height = pvrectin->height * size;
	if (pvrect->height > pvrectin->height - lineadj)
		pvrect->height = pvrectin->height - lineadj;
	pvrect->height &= ~1;
	pvrect->x = (pvrectin->width - pvrect->width) / 2;
	pvrect->y = (h - pvrect->height) / 2;
}

void R_ViewChanged(vrect_t *pvrect, int lineadj, float aspect)
{ // Called every time the vid structure or r_refdef changes.
 // Guaranteed to be called before the first refresh
	r_viewchanged = true;
	R_SetVrect(pvrect, &r_refdef.vrect, lineadj);
	r_refdef.horizontalFieldOfView = 2.0 * tan(r_refdef.fov_x / 360 * M_PI);
	r_refdef.fvrectx = (float)r_refdef.vrect.x;
	r_refdef.fvrectx_adj = (float)r_refdef.vrect.x - 0.5;
	r_refdef.vrect_x_adj_shift20 = (r_refdef.vrect.x << 20) + (1 << 19) - 1;
	r_refdef.fvrecty = (float)r_refdef.vrect.y;
	r_refdef.fvrecty_adj = (float)r_refdef.vrect.y - 0.5;
	r_refdef.vrectright = r_refdef.vrect.x + r_refdef.vrect.width;
	r_refdef.vrectright_adj_shift20 =
	    (r_refdef.vrectright << 20) + (1 << 19) - 1;
	r_refdef.fvrectright = (float)r_refdef.vrectright;
	r_refdef.fvrectright_adj = (float)r_refdef.vrectright - 0.5;
	r_refdef.vrectrightedge = (float)r_refdef.vrectright - 0.99;
	r_refdef.vrectbottom = r_refdef.vrect.y + r_refdef.vrect.height;
	r_refdef.fvrectbottom = (float)r_refdef.vrectbottom;
	r_refdef.fvrectbottom_adj = (float)r_refdef.vrectbottom - 0.5;
	r_refdef.aliasvrect.x = (int)(r_refdef.vrect.x * r_aliasuvscale);
	r_refdef.aliasvrect.y = (int)(r_refdef.vrect.y * r_aliasuvscale);
	r_refdef.aliasvrect.width =
	    (int)(r_refdef.vrect.width * r_aliasuvscale);
	r_refdef.aliasvrect.height =
	    (int)(r_refdef.vrect.height * r_aliasuvscale);
	r_refdef.aliasvrectright =
	    r_refdef.aliasvrect.x + r_refdef.aliasvrect.width;
	r_refdef.aliasvrectbottom =
	    r_refdef.aliasvrect.y + r_refdef.aliasvrect.height;
	pixelAspect = aspect;
	xOrigin = r_refdef.xOrigin;
	yOrigin = r_refdef.yOrigin;
	screenAspect = r_refdef.vrect.width * pixelAspect /
	    r_refdef.vrect.height;
	// 320*200 1.0 pixelAspect = 1.6 screenAspect
	// 320*240 1.0 pixelAspect = 1.3333 screenAspect
	// proper 320*200 pixelAspect = 0.8333333
	verticalFieldOfView = r_refdef.horizontalFieldOfView / screenAspect;
	// values for perspective projection
	// if math were exact, the values would range from 0.5 to to range+0.5
	// hopefully they wll be in the 0.000001 to range+.999999 and truncate
	// the polygon rasterization will never render in the first row or 
	// column but will definately render in the [range] row and column, so
	// adjust the buffer origin to get an exact edge to edge fill
	xcenter = ((float)r_refdef.vrect.width * XCENTERING) +
	    r_refdef.vrect.x - 0.5;
	aliasxcenter = xcenter * r_aliasuvscale;
	ycenter = ((float)r_refdef.vrect.height * YCENTERING) +
	    r_refdef.vrect.y - 0.5;
	aliasycenter = ycenter * r_aliasuvscale;
	xscale = r_refdef.vrect.width / r_refdef.horizontalFieldOfView;
	aliasxscale = xscale * r_aliasuvscale;
	xscaleinv = 1.0 / xscale;
	yscale = xscale * pixelAspect;
	aliasyscale = yscale * r_aliasuvscale;
	yscaleinv = 1.0 / yscale;
	xscaleshrink =
	    (r_refdef.vrect.width - 6) / r_refdef.horizontalFieldOfView;
	yscaleshrink = xscaleshrink * pixelAspect;
	screenedge[0].normal[0] = // left side clip
	    -1.0 / (xOrigin * r_refdef.horizontalFieldOfView);
	screenedge[0].normal[1] = 0;
	screenedge[0].normal[2] = 1;
	screenedge[0].type = PLANE_ANYZ;
	screenedge[1].normal[0] = // right side clip
	    1.0 / ((1.0 - xOrigin) * r_refdef.horizontalFieldOfView);
	screenedge[1].normal[1] = 0;
	screenedge[1].normal[2] = 1;
	screenedge[1].type = PLANE_ANYZ;
	screenedge[2].normal[0] = 0; // top side clip
	screenedge[2].normal[1] = -1.0 / (yOrigin * verticalFieldOfView);
	screenedge[2].normal[2] = 1;
	screenedge[2].type = PLANE_ANYZ;
	screenedge[3].normal[0] = 0; // bottom side clip
	screenedge[3].normal[1] = 1.0 / ((1.0 - yOrigin) * verticalFieldOfView);
	screenedge[3].normal[2] = 1;
	screenedge[3].type = PLANE_ANYZ;
	for (int i = 0; i < 4; i++)
		VectorNormalize(screenedge[i].normal);
	float res_scale =
	    sqrt((double)(r_refdef.vrect.width * r_refdef.vrect.height) /
		 (320.0 * 152.0)) * (2.0 / r_refdef.horizontalFieldOfView);
	r_aliastransition = r_aliastransbase.value * res_scale;
	r_resfudge = r_aliastransadj.value * res_scale;
	r_fov_greater_than_90 = !(scr_fov.value <= 90.0);
	D_ViewChanged();
}

void R_MarkLeaves()
{
	if (r_oldviewleaf == r_viewleaf)
		return;
	r_visframecount++;
	r_oldviewleaf = r_viewleaf;
	byte *vis = Mod_LeafPVS(r_viewleaf, cl.worldmodel);
	for (int i = 0; i < cl.worldmodel->numleafs; i++) {
		if (vis[i >> 3] & (1 << (i & 7))) {
			mnode_t *nd = (mnode_t *) & cl.worldmodel->leafs[i + 1];
			do {
				if (nd->visframe == r_visframecount)
					break;
				nd->visframe = r_visframecount;
				nd = nd->parent;
			} while (nd);
		}
	}
}

void R_DrawEntitiesOnList()
{
	float lightvec[3] = { -1, 0, 0 }; // FIXME: remove and do real lighting
	if (!r_drawentities.value)
		return;
	for (int i = 0; i < cl_numvisedicts; i++) {
		currententity = cl_visedicts[i];
		if (currententity == &cl_entities[cl.viewentity])
			continue; // don't draw the player
		switch (currententity->model->type) {
		case mod_sprite:
			VectorCopy(currententity->origin, r_entorigin);
			VectorSubtract(r_origin, r_entorigin, modelorg);
			R_DrawSprite();
			break;
		case mod_alias:
			VectorCopy(currententity->origin, r_entorigin);
			VectorSubtract(r_origin, r_entorigin, modelorg);
			// see if the bounding box lets us trivially reject, also sets
			// trivial accept status
			if (R_AliasCheckBBox()) {
				int j = R_LightPoint(currententity->origin);
				alight_t lighting;
				lighting.ambientlight = j;
				lighting.shadelight = j;
				lighting.plightvec = lightvec;
				for (int lnum = 0; lnum < MAX_DLIGHTS; lnum++)
					if (cl_dlights[lnum].die >= cl.time) {
						vec3_t dist;
						VectorSubtract(currententity->
							       origin,
							       cl_dlights[lnum].
							       origin, dist);
						float add =
							cl_dlights[lnum].radius
							- Length(dist);
						lighting.ambientlight += add;
					}
				// clamp lighting so it doesn't overbright as much
				if (lighting.ambientlight > 128)
					lighting.ambientlight = 128;
				if (lighting.ambientlight +
				    lighting.shadelight > 192)
					lighting.shadelight =
					    192 - lighting.ambientlight;
				cur_ent_alpha = currententity->alpha && r_entalpha.value == 1 ?
					(float)currententity->alpha/255 : 1;
				if (colored_aliaslight &&
					nameInList(r_fullbright_list.string, currententity->model->name))
					colored_aliaslight = 0;
				R_AliasDrawModel(&lighting);
			}
			break;
		default:
			break;
		}
	}
	cur_ent_alpha = 1;
}

void R_DrawViewModel()
{
	float lightvec[3] = { -1, 0, 0 }; // FIXME: remove and do real lighting
	if (!r_drawviewmodel.value || cl.items & IT_INVISIBILITY
		|| cl.stats[STAT_HEALTH] <= 0 || !cl.viewent.model)
		return;
	currententity = &cl.viewent;
	VectorCopy(currententity->origin, r_entorigin);
	VectorSubtract(r_origin, r_entorigin, modelorg);
	VectorCopy(vup, viewlightvec);
	VectorInverse(viewlightvec);
	int j = R_LightPoint(currententity->origin);
	if (j < 24)
		j = 24; // allways give some light on the gun
	r_viewlighting.ambientlight = j;
	r_viewlighting.shadelight = j;
	colored_aliaslight = 1;
	for (int lnum = 0; lnum < MAX_DLIGHTS; lnum++) { // add dynamic lights
		dlight_t *dl = &cl_dlights[lnum];
		if (!dl->radius || dl->die < cl.time)
			continue;
		vec3_t dist;
		VectorSubtract(currententity->origin, dl->origin, dist);
		float add = dl->radius - Length(dist);
		if (add > 150 && Length(dist) < 50) // hack in the muzzleflash
			colored_aliaslight = 0; // FIXME and do it properly
		if (add > 0)
			r_viewlighting.ambientlight += add;
	}
	// clamp lighting so it doesn't overbright as much
	if (r_viewlighting.ambientlight > 128)
		r_viewlighting.ambientlight = 128;
	if (r_viewlighting.ambientlight + r_viewlighting.shadelight > 192)
		r_viewlighting.shadelight = 192 - r_viewlighting.ambientlight;
	r_viewlighting.plightvec = lightvec;
	R_AliasDrawModel(&r_viewlighting);
}

int R_BmodelCheckBBox(model_t *clmodel, float *minmaxs)
{
	int clipflags = 0;
	if (currententity->angles[0] || currententity->angles[1]
	    || currententity->angles[2]) {
		for (int i = 0; i < 4; i++) {
			double d = DotProduct(currententity->origin,
				       view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;
			if (d <= -clmodel->radius)
				return BMODEL_FULLY_CLIPPED;
			if (d <= clmodel->radius)
				clipflags |= (1 << i);
		}
	} else {
		for (int i = 0; i < 4; i++) {
			// generate accept and reject points
			// FIXME: do with fast look-ups or integer tests
			// based on the sign bit of the floating point values
			int *pindex = pfrustum_indexes[i];
			vec3_t acceptpt, rejectpt;
			rejectpt[0] = minmaxs[pindex[0]];
			rejectpt[1] = minmaxs[pindex[1]];
			rejectpt[2] = minmaxs[pindex[2]];
			double d = DotProduct(rejectpt, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;
			if (d <= 0)
				return BMODEL_FULLY_CLIPPED;
			acceptpt[0] = minmaxs[pindex[3 + 0]];
			acceptpt[1] = minmaxs[pindex[3 + 1]];
			acceptpt[2] = minmaxs[pindex[3 + 2]];
			d = DotProduct(acceptpt, view_clipplanes[i].normal);
			d -= view_clipplanes[i].dist;
			if (d <= 0)
				clipflags |= (1 << i);
		}
	}
	return clipflags;
}

void R_DrawBEntitiesOnList()
{
	float minmaxs[6];
	if (!r_drawentities.value)
		return;
	vec3_t oldorigin;
	VectorCopy(modelorg, oldorigin);
	insubmodel = true;
	r_dlightframecount = r_framecount;
	model_t *clmodel; // keep here for the OpenBSD compiler
	for (int i = 0; i < cl_numvisedicts; i++) {
		currententity = cl_visedicts[i];
		switch (currententity->model->type) {
		case mod_brush:
			if (r_entalpha.value == 1) {
				if (!r_wateralphapass && currententity->alpha) {
					r_foundtranswater = 1;
					continue;
				}
				if (r_wateralphapass)
					cur_ent_alpha = currententity->alpha ?
						(float)currententity->alpha/255 : 1;
			}
			else
				cur_ent_alpha = 1;
			clmodel = currententity->model;
			// see if the bounding box lets us trivially reject
			// also sets trivial accept status
			for (int j = 0; j < 3; j++) {
				minmaxs[j] = currententity->origin[j] +
				    clmodel->mins[j];
				minmaxs[3 + j] = currententity->origin[j] +
				    clmodel->maxs[j];
			}
			int clipflags = R_BmodelCheckBBox(clmodel, minmaxs);
			if (clipflags != BMODEL_FULLY_CLIPPED) {
				VectorCopy(currententity->origin, r_entorigin);
				VectorSubtract(r_origin, r_entorigin, modelorg);
				// FIXME: is this needed?
				VectorCopy(modelorg, r_worldmodelorg);
				r_pcurrentvertbase = clmodel->vertexes;
				// FIXME: stop transforming twice
				R_RotateBmodel();
				// calculate dynamic lighting for bmodel
				// if it's not an instanced model
				if (clmodel->firstmodelsurface != 0) {
					for (int k = 0; k < MAX_DLIGHTS; k++) {
						if ((cl_dlights[k].die <cl.time)
						    || (!cl_dlights[k].radius))
							continue;
						R_MarkLights(&cl_dlights[k],
							     1 << k,
							     clmodel->nodes +
							     clmodel->hulls[0].
							     firstclipnode);
					}
				}
				r_pefragtopnode = NULL;
				for (int j = 0; j < 3; j++) {
					r_emins[j] = minmaxs[j];
					r_emaxs[j] = minmaxs[3 + j];
				}
				R_SplitEntityOnNode2(cl.worldmodel-> nodes);
				if (r_pefragtopnode) {
					currententity->topnode =r_pefragtopnode;
					if (r_pefragtopnode->contents >= 0) {
						// not a leaf; has to be clipped to the world BSP
						r_clipflags = clipflags;
						R_DrawSolidClippedSubmodelPolygons(clmodel);
					} else {
						// falls entirely in one leaf, so we just put all the
						// edges in the edge list and let 1/z sorting handle
						// drawing order
						R_DrawSubmodelPolygons(clmodel, clipflags);
					}
					currententity->topnode = NULL;
				}
				// put back world rotation and frustum clipping         
				// FIXME: R_RotateBmodel should just work off base_vxx
				VectorCopy(base_vpn, vpn);
				VectorCopy(base_vup, vup);
				VectorCopy(base_vright, vright);
				VectorCopy(base_modelorg, modelorg);
				VectorCopy(oldorigin, modelorg);
				R_TransformFrustum();
			}
			break;

		default:
			break;
		}
	}
	insubmodel = false;
	cur_ent_alpha = 1;
}

edge_t ledges[NUMSTACKEDGES + ((CACHE_SIZE - 1) / sizeof(edge_t)) + 1]/*
	__attribute__((aligned(CACHE_SIZE)))*/;
surf_t lsurfs[NUMSTACKSURFACES + ((CACHE_SIZE - 1) / sizeof(surf_t)) +
1] /*__attribute__((aligned(CACHE_SIZE)))*/;

void R_EdgeDrawing()
{
	// CyanBun96: windows would crash all over the place with the original
	// alignment code, this might be compiler-dependent but it works
	// Align the arrays themselves
	// Accessing them directly without pointer adjustment
	r_edges = auxedges ? auxedges : &ledges[0]; // already aligned
	if (r_surfsonstack) {
		// surface 0 doesn't really exist; it's just a dummy because
		// index 0 is used to indicate no edge attached to surface
		surfaces = &lsurfs[1]; // Point to the first "real" surface
		surf_max = &surfaces[r_cnumsurfs];
	}
	R_BeginEdgeFrame();
	if (r_dspeeds.value) rw_time1 = Sys_FloatTime();
	R_RenderWorld();
	if (r_dspeeds.value) db_time1 = rw_time2 = Sys_FloatTime();
	if (r_wateralphapass || r_pass || !((int)r_twopass.value&1)) R_DrawBEntitiesOnList();
	if (r_dspeeds.value) se_time2 = db_time2 = Sys_FloatTime();
	R_ScanEdges();
}

void R_RenderView() // r_refdef must be set before the first call
{ // CyanBun96: three-pass rendering. consider *not* doing that, or doing it less sloppily
	byte warpbuffer[WARP_WIDTH * WARP_HEIGHT];
	r_warpbuffer = warpbuffer;
	R_SetupFrame();
	R_MarkLeaves(); // done here so we know if we're in water
	if (!cl_entities[0].model || !cl.worldmodel)
		Sys_Error("R_RenderView: NULL worldmodel");
	r_foundtranswater =  r_wateralphapass = false; // Manoel Kasimier - translucent water
	r_pass = 0;
	R_EdgeDrawing();
	if ((int)r_twopass.value&1) {
		r_pass = 1;
		R_EdgeDrawing();
	}
	R_DrawEntitiesOnList();
	if (r_foundtranswater && (r_twopass.value + r_entalpha.value)) {
		r_wateralphapass = true;
		R_EdgeDrawing ();
	}
	R_DrawViewModel();
	R_DrawParticles();
	if (r_dowarp)
		D_WarpScreen();
        if (!r_dowarp && fog_density < 1) // broken underwater, fixme?
                R_DrawFog();
	V_SetContentsColor(r_viewleaf->contents);
	if (r_reportsurfout.value && r_outofsurfaces) // TODO r_dspeds and such
		Con_Printf("Short %d surfaces\n", r_outofsurfaces);
	if (r_reportedgeout.value && r_outofedges)
		Con_Printf("Short roughly %d edges\n", r_outofedges * 2 / 3);
}

void R_InitTurb()
{
	for (int i = 0; i < (SIN_BUFFER_SIZE); i++) {
		sintable[i] = AMP + sin(i * 3.14159 * 2 / CYCLE) * AMP;
		intsintable[i] = AMP2 + sin(i * 3.14159 * 2 / CYCLE) * AMP2; // AMP2, not 20
	}
}
