// Copyright (C) 1996-1997 Id Software, Inc. GPLv3 See LICENSE for details.
#include "quakedef.h"
// key up events are sent even if in console mode

static s32 shift_down = false;
static s32 history_line = 0;
static bool consolekeys[256]; // if true, can't be rebound while in console
static bool menubound[256]; // if true, can't be rebound while in menu
static s32 keyshift[256]; // key to map to if shift held down in console
static bool keydown[256];

keyname_t keynames[] = {
	{ "TAB", K_TAB }, { "ENTER", K_ENTER }, { "ESCAPE", K_ESCAPE },
	{ "SPACE", K_SPACE }, { "BACKSPACE", K_BACKSPACE },
	{ "UPARROW", K_UPARROW }, { "DOWNARROW", K_DOWNARROW },
	{ "LEFTARROW", K_LEFTARROW }, { "RIGHTARROW", K_RIGHTARROW },

	{ "ALT", K_ALT }, { "CTRL", K_CTRL }, { "SHIFT", K_SHIFT },

	{ "F1", K_F1 }, { "F2", K_F2 }, { "F3", K_F3 }, { "F4", K_F4 },
	{ "F5", K_F5 }, { "F6", K_F6 }, { "F7", K_F7 }, { "F8", K_F8 },
	{ "F9", K_F9 }, { "F10", K_F10 }, { "F11", K_F11 }, { "F12", K_F12 },

	{ "INS", K_INS }, { "DEL", K_DEL }, { "PGDN", K_PGDN },
	{ "PGUP", K_PGUP }, { "HOME", K_HOME }, { "END", K_END },

	{ "MOUSE1", K_MOUSE1 }, { "MOUSE2", K_MOUSE2 }, { "MOUSE3", K_MOUSE3 },
	{ "MOUSE4", K_MOUSE4 }, { "MOUSE5", K_MOUSE5 },

	{ "JOY1", K_JOY1 }, { "JOY2", K_JOY2 },
	{ "JOY3", K_JOY3 }, { "JOY4", K_JOY4 },

	{ "AUX1", K_AUX1 }, { "AUX2", K_AUX2 }, { "AUX3", K_AUX3 },
	{ "AUX4", K_AUX4 }, { "AUX5", K_AUX5 }, { "AUX6", K_AUX6 },
	{ "AUX7", K_AUX7 }, { "AUX8", K_AUX8 }, { "AUX9", K_AUX9 },
	{ "AUX10", K_AUX10 }, { "AUX11", K_AUX11 }, { "AUX12", K_AUX12 },
	{ "AUX13", K_AUX13 }, { "AUX14", K_AUX14 }, { "AUX15", K_AUX15 },
	{ "AUX16", K_AUX16 }, { "AUX17", K_AUX17 }, { "AUX18", K_AUX18 },
	{ "AUX19", K_AUX19 }, { "AUX20", K_AUX20 }, { "AUX21", K_AUX21 },
	{ "AUX22", K_AUX22 }, { "AUX23", K_AUX23 }, { "AUX24", K_AUX24 },
	{ "AUX25", K_AUX25 }, { "AUX26", K_AUX26 }, { "AUX27", K_AUX27 },
	{ "AUX28", K_AUX28 }, { "AUX29", K_AUX29 }, { "AUX30", K_AUX30 },
	{ "AUX31", K_AUX31 }, { "AUX32", K_AUX32 }, { "PAUSE", K_PAUSE },

	{ "MWHEELUP", K_MWHEELUP }, { "MWHEELDOWN", K_MWHEELDOWN },

	{ "SEMICOLON", ';' }, // because a raw semicolon seperates commands

	{ NULL, 0 }
};

// Line typing into the console
void Key_Console(s32 key) // Interactive line editing and console scrollback
{
	if(key == K_ENTER){
		Cbuf_AddText(key_lines[edit_line] + 1);	// skip the >
		Cbuf_AddText("\n");
		Con_Printf("%s\n", key_lines[edit_line]);
		edit_line = (edit_line + 1) & 31;
		history_line = edit_line;
		key_lines[edit_line][0] = ']';
		key_linepos = 1;
		if(cls.state == ca_disconnected) // force an update because
			SCR_UpdateScreen(); // the command may take some time

		return;
	}
	if(key == K_TAB){ // command completion
		const s8 *cmd = Cmd_CompleteCommand(key_lines[edit_line] + 1);
		if(!cmd)
			cmd = Cvar_CompleteVariable(key_lines[edit_line] + 1);
		if(cmd){
			Q_strcpy(key_lines[edit_line] + 1, cmd);
			key_linepos = Q_strlen(cmd) + 1;
			key_lines[edit_line][key_linepos] = ' ';
			key_linepos++;
			key_lines[edit_line][key_linepos] = 0;
			return;
		}
	}
	if(key == K_BACKSPACE || key == K_LEFTARROW){
		if(key_linepos > 1)
			key_linepos--;
		return;
	}
	if(key == K_UPARROW){
		do {
			history_line = (history_line - 1) & 31;
		} while(history_line != edit_line
			 && !key_lines[history_line][1]);
		if(history_line == edit_line)
			history_line = (edit_line + 1) & 31;
		Q_strcpy(key_lines[edit_line], key_lines[history_line]);
		key_linepos = Q_strlen(key_lines[edit_line]);
		return;
	}
	if(key == K_DOWNARROW){
		if(history_line == edit_line)
			return;
		do {
			history_line = (history_line + 1) & 31;
		}
		while(history_line != edit_line
		       && !key_lines[history_line][1]);
		if(history_line == edit_line){
			key_lines[edit_line][0] = ']';
			key_linepos = 1;
		} else {
			Q_strcpy(key_lines[edit_line], key_lines[history_line]);
			key_linepos = Q_strlen(key_lines[edit_line]);
		}
		return;
	}
	if(key == K_PGUP || key == K_MWHEELUP){
		con_backscroll += 2;
		if((u32)con_backscroll
				> con_totallines - (vid.height >> 3) - 1)
			con_backscroll = con_totallines - (vid.height >> 3) - 1;
		return;
	}
	if(key == K_PGDN || key == K_MWHEELDOWN){
		con_backscroll -= 2;
		if(con_backscroll < 0)
			con_backscroll = 0;
		return;
	}
	if(key == K_HOME){
		con_backscroll = con_totallines - (vid.height >> 3) - 1;
		return;
	}
	if(key == K_END){
		con_backscroll = 0;
		return;
	}
	if(key < 32 || key > 127)
		return; // non printable
	if(key_linepos < MAXCMDLINE - 1){
		key_lines[edit_line][key_linepos] = key;
		key_linepos++;
		key_lines[edit_line][key_linepos] = 0;
	}
}

void Key_Message(s32 key)
{
	static s32 chat_bufferlen = 0;
	if(key == K_ENTER){
		if(team_message)
			Cbuf_AddText("say_team \"");
		else
			Cbuf_AddText("say \"");
		Cbuf_AddText(chat_buffer);
		Cbuf_AddText("\"\n");

		key_dest = key_game;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}
	if(key == K_ESCAPE){
		key_dest = key_game;
		chat_bufferlen = 0;
		chat_buffer[0] = 0;
		return;
	}
	if(key < 32 || key > 127)
		return; // non printable
	if(key == K_BACKSPACE){
		if(chat_bufferlen){
			chat_bufferlen--;
			chat_buffer[chat_bufferlen] = 0;
		}
		return;
	}
	if(chat_bufferlen == 31)
		return;	// all full
	chat_buffer[chat_bufferlen++] = key;
	chat_buffer[chat_bufferlen] = 0;
}

// Returns a key number to be used to index keybindings[] by looking at
// the given string. Single ascii characters return themselves, while
// the K_* names are matched up.
s32 Key_StringToKeynum(s8 *str)
{

	if(!str || !str[0])
		return -1;
	if(!str[1])
		return str[0];
	for(keyname_t *kn = keynames; kn->name; kn++){
		if(!q_strcasecmp(str, kn->name))
			return kn->keynum;
	}
	return -1;
}

// Returns a string (either a single ascii s8, or a K_* name) for the
// given keynum.
// FIXME: handle quote special (general escape sequence?)
s8 *Key_KeynumToString(s32 keynum)
{
	static s8 tinystr[2];
	if(keynum == -1)
		return "<KEY NOT FOUND>";
	if(keynum > 32 && keynum < 127){ // printable ascii
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}
	for(keyname_t *kn = keynames; kn->name; kn++)
		if(keynum == kn->keynum)
			return kn->name;
	return "<UNKNOWN KEYNUM>";
}

void Key_SetBinding(s32 keynum, s8 *binding)
{
	if(keynum == -1)
		return;
	if(keybindings[keynum]){ // free old bindings
		Z_Free(keybindings[keynum]);
		keybindings[keynum] = NULL;
	}
	s32 l = Q_strlen(binding); // allocate memory for new binding
	s8 *new = Z_Malloc(l + 1);
	Q_strcpy(new, binding);
	new[l] = 0;
	keybindings[keynum] = new;
}

void Key_Unbind_f()
{
	if(Cmd_Argc() != 2){
		Con_Printf("unbind <key> : remove commands from a key\n");
		return;
	}
	s32 b = Key_StringToKeynum(Cmd_Argv(1));
	if(b == -1){
		Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}
	Key_SetBinding(b, "");
}

void Key_Unbindall_f()
{
	for(s32 i = 0; i < 256; i++)
		if(keybindings[i])
			Key_SetBinding(i, "");
}

void Key_Bind_f()
{
	s8 cmd[1024];
	s32 c = Cmd_Argc();
	if(c != 2 && c != 3){
		Con_Printf
		    ("bind <key> [command] : attach a command to a key\n");
		return;
	}
	s32 b = Key_StringToKeynum(Cmd_Argv(1));
	if(b == -1){
		Con_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}
	if(c == 2){
		if(keybindings[b])
			Con_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(1),
				   keybindings[b]);
		else
			Con_Printf("\"%s\" is not bound\n", Cmd_Argv(1));
		return;
	}
	// copy the rest of the command line
	cmd[0] = 0; // start out with a null string
	for(s32 i = 2; i < c; i++){
		if(i > 2)
			strcat(cmd, " ");
		strcat(cmd, Cmd_Argv(i));
	}
	Key_SetBinding(b, cmd);
}


void Key_WriteBindings(FILE *f) // Writes lines containing "bind key value"
{
	for(s32 i = 0; i < 256; i++)
		if(keybindings[i] && *keybindings[i])
			fprintf(f, "bind \"%s\" \"%s\"\n",
				Key_KeynumToString(i), keybindings[i]);
}

void Key_Init()
{
	for(s32 i = 0; i < 32; i++){
		key_lines[i][0] = ']';
		key_lines[i][1] = 0;
	}
	key_linepos = 1;
	for(s32 i = 32; i < 128; i++) // init ascii characters in console mode
		consolekeys[i] = true;
	consolekeys[K_ENTER] = true;
	consolekeys[K_TAB] = true;
	consolekeys[K_LEFTARROW] = true;
	consolekeys[K_RIGHTARROW] = true;
	consolekeys[K_UPARROW] = true;
	consolekeys[K_DOWNARROW] = true;
	consolekeys[K_BACKSPACE] = true;
	consolekeys[K_PGUP] = true;
	consolekeys[K_PGDN] = true;
	consolekeys[K_SHIFT] = true;
	consolekeys[K_MWHEELUP] = true;
	consolekeys[K_MWHEELDOWN] = true;
	consolekeys['`'] = false;
	consolekeys['~'] = false;
	for(s32 i = 0; i < 256; i++)
		keyshift[i] = i;
	for(s32 i = 'a'; i <= 'z'; i++)
		keyshift[i] = i - 'a' + 'A';
	keyshift['1'] = '!';
	keyshift['2'] = '@';
	keyshift['3'] = '#';
	keyshift['4'] = '$';
	keyshift['5'] = '%';
	keyshift['6'] = '^';
	keyshift['7'] = '&';
	keyshift['8'] = '*';
	keyshift['9'] = '(';
	keyshift['0'] = ')';
	keyshift['-'] = '_';
	keyshift['='] = '+';
	keyshift[','] = '<';
	keyshift['.'] = '>';
	keyshift['/'] = '?';
	keyshift[';'] = ':';
	keyshift['\''] = '"';
	keyshift['['] = '{';
	keyshift[']'] = '}';
	keyshift['`'] = '~';
	keyshift['\\'] = '|';
	menubound[K_ESCAPE] = true;
	for(s32 i = 0; i < 12; i++)
		menubound[K_F1 + i] = true;
	Cmd_AddCommand("bind", Key_Bind_f);
	Cmd_AddCommand("unbind", Key_Unbind_f);
	Cmd_AddCommand("unbindall", Key_Unbindall_f);

}

// Called by the system between frames for both key up and key down events
// Should NOT be called during an interrupt!
void Key_Event(s32 key, bool down)
{
	keydown[key] = down;
	if(!down)
		key_repeats[key] = 0;
	key_lastpress = key;
	key_count++;
	if(key_count <= 0){
		return; // just catching keys for Con_NotifyBox
	}
	if(down){ // update auto-repeat status
		key_repeats[key]++;
		if(key != K_BACKSPACE && key != K_PAUSE
		    && key_repeats[key] > 1){
			return;	// ignore most autorepeats
		}
		if(key >= 200 && !keybindings[key])
			Con_Printf("%s is unbound, hit F4 to set.\n",
				   Key_KeynumToString(key));
	}
	if(key == K_SHIFT)
		shift_down = down;
	// handle escape specialy, so the user can never unbind it
	if(key == K_ESCAPE){
		if(!down)
			return;
		switch(key_dest){
		case key_message:
			Key_Message(key);
			break;
		case key_menu:
			M_Keydown(key);
			break;
		case key_game:
		case key_console:
			M_ToggleMenu_f();
			break;
		default:
			Sys_Error("Bad key_dest");
		}
		return;
	}
	// key up events only generate commands if the game key binding is a
	// button command (leading + sign).  These will occur even in console
	// mode,to keep the character from continuing an action started before a
	// console switch. Button commands include the kenum as a parameter, so
	// multiple downs can be matched with ups
	if(!down){
		s8 *kb = keybindings[key];
		s8 cmd[1024];
		if(kb && kb[0] == '+'){
			sprintf(cmd, "-%s %i\n", kb + 1, key);
			Cbuf_AddText(cmd);
		}
		if(keyshift[key] != key){
			kb = keybindings[keyshift[key]];
			if(kb && kb[0] == '+'){
				sprintf(cmd, "-%s %i\n", kb + 1, key);
				Cbuf_AddText(cmd);
			}
		}
		return;
	}
	// during demo playback, most keys bring up the main menu
	if(cls.demoplayback && down && consolekeys[key]
	    && key_dest == key_game){
		M_ToggleMenu_f();
		return;
	}
	// if not a consolekey, send to the interpreter no matter what mode is
	if((key_dest == key_menu && menubound[key])
	    || (key_dest == key_console && !consolekeys[key])
	    || (key_dest == key_game && (!con_forcedup || !consolekeys[key]))){
		s8 *kb = keybindings[key];
		s8 cmd[1024];
		if(kb){
			if(kb[0] == '+'){ // button cmds add keynum as a parm
				sprintf(cmd, "%s %i\n", kb, key);
				Cbuf_AddText(cmd);
			} else {
				Cbuf_AddText(kb);
				Cbuf_AddText("\n");
			}
		}
		return;
	}
	if(!down)
		return; // other systems only care about key down events
	if(shift_down)
		key = keyshift[key];
	switch(key_dest){
	case key_message:
		Key_Message(key);
		break;
	case key_menu:
		M_Keydown(key);
		break;
	case key_game:
	case key_console:
		Key_Console(key);
		break;
	default:
		Sys_Error("Bad key_dest");
	}
}
