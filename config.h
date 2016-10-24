#ifndef CONFIG_H
#define CONFIG_H

#include "jwm.h"
#include "window.h"

/* appearance */
const unsigned int borderpx  = 0;        /* border pixel of windows */
const unsigned int snap      = 32;       /* snap pixel */
const char *fonts[1]         = { "monospace:size=10" };
const char dmenufont[]       = "monospace:size=10";
const char col_gray1[]       = "#222222";
const char col_gray2[]       = "#444444";
const char col_gray3[]       = "#bbbbbb";
const char col_gray4[]       = "#eeeeee";
const char col_cyan[]        = "#005577";
const char col_orange[]      = "#ffa500";
const char *colors[SchemeLast][3]      = {
	/*               fg         bg         border   */
	[SchemeNorm] = { col_gray3, col_gray1, col_gray2 },
 	[SchemeSel] =  { col_gray4, col_cyan,  col_cyan  },
};

/* tagging */
const char *tags[4] = { "Emacs", "Web", "Term", "Extras"};

/* layout(s) */
const float mfact     = 0.50; /* factor of master area size [0.05..0.95] */
const int nmaster     = 1;    /* number of clients in master area */
const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */

const Layout layouts[3] = {
	/* symbol     arrange function */
	{ "[ T ]",      tile },    /* first entry is default */
	{ "[ F ]",      NULL },    /* no layout function means floating behavior */
	{ "[ M ]",      monocle },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \

/* commands */
char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_orange, "-sf", col_gray4, NULL };
const char *termcmd[]  = { "urxvt", NULL };
const char *emacscmd[]  = { "emacs", NULL };
const char *firefoxcmd[]  = { "firefox", NULL };

Key keys[19] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_d,      spawn,          {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_e,      spawn,          {.v = emacscmd } },
	{ MODKEY|ShiftMask,             XK_f,      spawn,          {.v = firefoxcmd } },
	{ MODKEY,                       XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
	TAGKEYS(                        XK_ampersand,              0)
	TAGKEYS(                        XK_eacute,                 1)
	TAGKEYS(                        XK_quotedbl,               2)
	TAGKEYS(                        XK_apostrophe,             3)
	{ MODKEY|ShiftMask,             XK_q,      quit,           {0} },
};

/* button definitions */
/* click can be ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
Button buttons[4] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
};

#endif
