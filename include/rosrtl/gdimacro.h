/*
 * gdimacro.h
 */

#ifndef ROSRTL_GDIMACRO_H
#define ROSRTL_GDIMACRO_H

#define IN_RECT(r,x,y) \
( \
 (x) >= (r).left && \
 (y) >= (r).top && \
 (x) < (r).right && \
 (y) < (r).bottom \
)

#define RECT_OVERLAP(a,b) \
( \
  (a).left < (b).right && \
  (b).left < (a).right && \
  (a).top < (b).bottom && \
  (b).top < (a).bottom \
)

#endif /* ROSRTL_GDIMACRO_H */
