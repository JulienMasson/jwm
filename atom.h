#ifndef ATOM_H
#define ATOM_H

#include "jwm.h"

void setup_atoms(void);
void support_ewmh(void);
Atom getatomprop(Client *c, Atom prop);

#endif
