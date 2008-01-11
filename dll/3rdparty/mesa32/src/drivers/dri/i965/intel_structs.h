#ifndef INTEL_STRUCTS_H
#define INTEL_STRUCTS_H

struct br0 {
   GLuint length:8;
   GLuint pad0:3;
   GLuint dst_tiled:1;
   GLuint pad1:8;
   GLuint write_rgb:1;
   GLuint write_alpha:1;
   GLuint opcode:7;
   GLuint client:3;
};

   
struct br13 {
   GLint dest_pitch:16;
   GLuint rop:8;
   GLuint color_depth:2;
   GLuint pad1:3;
   GLuint mono_source_transparency:1;
   GLuint clipping_enable:1;
   GLuint pad0:1;
};



/* This is an attempt to move some of the 2D interaction in this
 * driver to using structs for packets rather than a bunch of #defines
 * and dwords.
 */
struct xy_color_blit {
   struct br0 br0;
   struct br13 br13;

   struct {
      GLuint dest_x1:16;
      GLuint dest_y1:16;
   } dw2;

   struct {
      GLuint dest_x2:16;
      GLuint dest_y2:16;
   } dw3;
   
   GLuint dest_base_addr;
   GLuint color;
};

struct xy_src_copy_blit {
   struct br0 br0;
   struct br13 br13;

   struct {
      GLuint dest_x1:16;
      GLuint dest_y1:16;
   } dw2;

   struct {
      GLuint dest_x2:16;
      GLuint dest_y2:16;
   } dw3;
   
   GLuint dest_base_addr;

   struct {
      GLuint src_x1:16;
      GLuint src_y1:16;
   } dw5;

   struct {
      GLint src_pitch:16;
      GLuint pad:16;
   } dw6;
   
   GLuint src_base_addr;
};

struct xy_setup_blit {
   struct br0 br0;
   struct br13 br13;

   struct {
      GLuint clip_x1:16;
      GLuint clip_y1:16;
   } dw2;

   struct {
      GLuint clip_x2:16;
      GLuint clip_y2:16;
   } dw3;
      
   GLuint dest_base_addr;
   GLuint background_color;
   GLuint foreground_color;
   GLuint pattern_base_addr;
};


struct xy_text_immediate_blit {
   struct {
      GLuint length:8;
      GLuint pad2:3;
      GLuint dst_tiled:1;
      GLuint pad1:4;
      GLuint byte_packed:1;
      GLuint pad0:5;
      GLuint opcode:7;
      GLuint client:3;
   } dw0;

   struct {
      GLuint dest_x1:16;
      GLuint dest_y1:16;
   } dw1;

   struct {
      GLuint dest_x2:16;
      GLuint dest_y2:16;
   } dw2;   

   /* Src bitmap data follows as inline dwords.
    */
};


#define CLIENT_2D 0x2
#define OPCODE_XY_SETUP_BLT 0x1
#define OPCODE_XY_COLOR_BLT 0x50
#define OPCODE_XY_TEXT_IMMEDIATE_BLT 0x31

#endif
