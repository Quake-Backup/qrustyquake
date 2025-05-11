// Copyright (C) 1996-1997 Id Software, Inc. GPLv3 See LICENSE for details.

// client.h

typedef struct
{
	int		length;
	char	map[MAX_STYLESTRING];
	char    average; //johnfitz
        char    peak; //johnfitz
} lightstyle_t;

typedef struct
{
	char	name[MAX_SCOREBOARDNAME];
	float	entertime;
	int		frags;
	int		colors;			// two 4 bit fields
	byte	translations[VID_GRADES*256];
} scoreboard_t;

typedef struct
{
	int		destcolor[3];
	int		percent;		// 0-256
} cshift_t;



//
// client_state_t should hold all pieces of the client state
//

typedef struct
{
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
	int		key;
	vec3_t  color;                          //johnfitz -- lit support via lordhavoc
} dlight_t;


typedef struct
{
	int		entity;
	struct model_s	*model;
	float	endtime;
	vec3_t	start, end;
} beam_t;


typedef enum {
ca_dedicated, 		// a dedicated server with no ability to start a client
ca_disconnected, 	// full screen console with no connection
ca_connected		// valid netcon, talking to a server
} cactive_t;

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct
{
        cactive_t       state;

// personalization data sent to server
        char            spawnparms[MAX_MAPSTRING];      // to restart a level

// demo loop control
        int             demonum;                // -1 = don't play demos
        char            demos[MAX_DEMOS][MAX_DEMONAME]; // when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
        qboolean        demorecording;
        qboolean        demoplayback;

// did the user pause demo playback? (separate from cl.paused because we don't
// want a svc_setpause inside the demo to actually pause demo playback).
        qboolean        demopaused;

        qboolean        timedemo;
        int             forcetrack;             // -1 = use normal cd track
        FILE            *demofile;
        int             td_lastframe;           // to meter out one message a frame
        int             td_startframe;          // host_framecount at start
        float           td_starttime;           // realtime at second frame of timedemo

// connection information
        int             signon;                 // 0 to SIGNONS
        struct qsocket_s        *netcon;
        sizebuf_t       message;                // writing buffer to send to server

} client_static_t;

extern client_static_t	cls;

//
// the client_state_t structure is wiped completely at every
// server signon
//
typedef struct
{
        int                     movemessages;   // since connecting to this server
                                                                // throw out the first couple, so the player
                                                                // doesn't accidentally do something the
                                                                // first frame
        usercmd_t       cmd;                    // last command sent to the server
	usercmd_t   pendingcmd;     // accumulated state from mice+joysticks.

// information for local display
        int                     stats[MAX_CL_STATS];    // health, etc
        int                     items;                  // inventory bit flags
        float   item_gettime[32];       // cl.time of aquiring item, for blinking
        float           faceanimtime;   // use anim frame if cl.time < this

        cshift_t        cshifts[NUM_CSHIFTS];   // color shifts for damage, powerups
        cshift_t        prev_cshifts[NUM_CSHIFTS];      // and content types

// the client maintains its own idea of view angles, which are
// sent to the server each frame.  The server sets punchangle when
// the view is temporarliy offset, and an angle reset commands at the start
// of each level and after teleporting.
        vec3_t          mviewangles[2]; // during demo playback viewangles is lerped
                                                                // between these
        vec3_t          viewangles;

        vec3_t          mvelocity[2];   // update by server, used for lean+bob
                                                                // (0 is newest)
        vec3_t          velocity;               // lerped between mvelocity[0] and [1]

        vec3_t          punchangle;             // temporary offset

// pitch drifting vars
        float           idealpitch;
        float           pitchvel;
        qboolean        nodrift;
        float           driftmove;
        double          laststop;

        float           viewheight;
        float           crouch;                 // local amount for smoothing stepups

        qboolean        paused;                 // send over by server
        qboolean        onground;
        qboolean        inwater;

        int                     intermission;   // don't change view angle, full screen, etc
        int                     completed_time; // latched at intermission start

        double          mtime[2];               // the timestamp of last two messages
        double          time;                   // clients view of time, should be between
                                                                // servertime and oldservertime to generate
                                                                // a lerp point for other data
        double          oldtime;                // previous cl.time, time-oldtime is used
                                                                // to decay light values and smooth step ups


        float           last_received_message;  // (realtime) for net trouble icon

//
// information that is static for the entire time connected to a server
//
        struct model_s         *model_precache[MAX_MODELS];
        struct sfx_s            *sound_precache[MAX_SOUNDS];

        char            mapname[128];
        char            levelname[128]; // for display on solo scoreboard //johnfitz -- was 40.
        int                     viewentity;             // cl_entitites[cl.viewentity] = player
        int                     maxclients;
        int                     gametype;

// refresh related state
        struct model_s *worldmodel;    // cl_entitites[0].model
        struct efrag_s  *free_efrags;
        int                     num_efrags;
        int                     num_entities;   // held in cl_entities array
        int                     num_statics;    // held in cl_staticentities array
        entity_t        viewent;                        // the gun model

        int                     cdtrack, looptrack;     // cd audio

// frag scoreboard
        scoreboard_t    *scores;                // [cl.maxclients]

        unsigned        protocol; //johnfitz
        unsigned        protocolflags;
} client_state_t;


//
// cvars
//

extern	client_state_t	cl;

// FIXME, allocate dynamically
extern	efrag_t			cl_efrags[MAX_EFRAGS];
extern	entity_t		cl_entities[MAX_EDICTS];
extern	entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
extern	lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
extern	dlight_t		cl_dlights[MAX_DLIGHTS];
extern	entity_t		cl_temp_entities[MAX_TEMP_ENTITIES];
extern	beam_t			cl_beams[MAX_BEAMS];

//=============================================================================

//
// cl_main
//
dlight_t *CL_AllocDlight (int key);
void	CL_DecayLights (void);

void CL_Init (void);

void CL_EstablishConnection (char *host);
void CL_Signon1 (void);
void CL_Signon2 (void);
void CL_Signon3 (void);
void CL_Signon4 (void);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_NextDemo (void);

extern	int				cl_numvisedicts;
extern	entity_t		*cl_visedicts[MAX_VISEDICTS];

//
// cl_input
//
typedef struct
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_SendMove (const usercmd_t *cmd);

void CL_ParseTEnt (void);
void CL_UpdateTEnts (void);

void CL_ClearState (void);


int  CL_ReadFromServer (void);
void CL_WriteToServer (usercmd_t *cmd);
void CL_BaseMove (usercmd_t *cmd);


float CL_KeyState (kbutton_t *key);
char *Key_KeynumToString (int keynum);

//
// cl_demo.c
//
void CL_StopPlayback (void);
int CL_GetMessage (void);

void CL_Stop_f (void);
void CL_Record_f (void);
void CL_PlayDemo_f (void);
void CL_TimeDemo_f (void);

//
// cl_parse.c
//
void CL_ParseServerMessage (void);
void CL_NewTranslation (int slot);

//
// view
//
void V_StartPitchDrift (void);
void V_StopPitchDrift (void);

void V_RenderView (void);
void V_UpdatePalette (void);
void V_Register (void);
void V_ParseDamage (void);
void V_SetContentsColor (int contents);


//
// cl_tent
//
void CL_InitTEnts (void);
void CL_SignonReply (void);
