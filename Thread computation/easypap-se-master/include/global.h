
#ifndef GLOBAL_IS_DEF
#define GLOBAL_IS_DEF

#ifdef __cplusplus
extern "C" {
#endif

extern char *easypap_image_file;
extern char *easypap_mesh_file;
extern char *config_param;

extern unsigned gpu_used;
extern unsigned easypap_gl_buffer_sharing;
extern unsigned picking_enabled;

extern char *kernel_name, *variant_name, *tile_name;

typedef enum
{
  EASYPAP_MODE_UNDEFINED,
  EASYPAP_MODE_2D_IMAGES,
  EASYPAP_MODE_3D_MESHES
} easypap_mode_t;

extern easypap_mode_t easypap_mode;

#ifdef __cplusplus
}
#endif

#endif
