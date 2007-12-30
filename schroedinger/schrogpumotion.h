#ifndef __SCHRO_GPUMOTION_H__
#define __SCHRO_GPUMOTION_H__

#include <schroedinger/schromotion.h>
#include <schroedinger/schrogpuframe.h>

SCHRO_BEGIN_DECLS

typedef struct _SchroGPUMotion SchroGPUMotion;

SchroGPUMotion *schro_gpumotion_new(cudaStream_t stream);
void schro_gpumotion_free(SchroGPUMotion *rv);

/** Initialize GPU structures */
void schro_gpumotion_init(SchroGPUMotion *self, SchroMotion *motion);
/** Copy CPU to GPU structure */
void schro_gpumotion_copy(SchroGPUMotion *self, SchroMotion *motion);
/** Render at GPU */
void schro_gpumotion_render(SchroGPUMotion *self, SchroMotion *motion, SchroGPUFrame *gdest);

SCHRO_END_DECLS

#endif
