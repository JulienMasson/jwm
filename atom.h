#ifndef ATOM_H
#define ATOM_H

#include "jwm.h"

typedef enum {
	NetSupported,
	NetWMName,
	NetWMState,
	NetWMFullscreen,
	NetActiveWindow,
	NetWMWindowType,
	NetWMWindowTypeDialog,
	NetClientList,
	NetLast
} net_atom_t; /* EWMH atoms */

typedef enum {
	WMProtocols,
	WMDelete,
	WMState,
	WMTakeFocus,
	WMLast
} wm_atom_t; /* default atoms */

void setup_atoms(void);
void support_ewmh(void);
Atom getatomprop(Client *c, Atom prop);

#endif
