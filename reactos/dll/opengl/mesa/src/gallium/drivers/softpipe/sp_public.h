#ifndef SP_PUBLIC_H
#define SP_PUBLIC_H

struct pipe_screen;
struct sw_winsys;

struct pipe_screen *
softpipe_create_screen(struct sw_winsys *winsys);

#endif
