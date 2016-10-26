#ifndef SCREEN_H
#define SCREEN_H

typedef struct {
	int number;
	int height;
	int width;
} screen_t;

void setup_screen(void);
screen_t *get_screen(void);

#endif
