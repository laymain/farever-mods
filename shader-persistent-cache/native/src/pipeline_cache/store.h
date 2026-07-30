#pragma once

#include "types.h"

// ---- PSO pipeline library (NATIVE.md Section 8, Section 12 milestones 4-6) ----
// Called from get_device's own intercept the first time a real ID3D12Device* is captured: loads
// any persisted cache blob from disk and sets up the ID3D12PipelineLibrary.
void Dx12PsoCache_OnDeviceCaptured(dx_device device, const char *selfDir);

// Called from rootsignature_create/create_graphics_pipeline_state/create_compute_pipeline_state's
// own intercepts (NATIVE.md Section 7/Section 10 point 6, Section 12 milestone 5): hash rawDesc,
// try the pipeline library first, call through to realFn on a miss, then store the result -
// realFn is dx12_original.hdll's own real create_*_pipeline_state, called only on a cache miss.
dx_resource Dx12PsoCache_CreateGraphicsPipeline(void *rawDesc, dx_resource (*realFn)(void *));
dx_resource Dx12PsoCache_CreateComputePipeline(void *rawDesc, dx_resource (*realFn)(void *));

// NATIVE.md Section 12 milestone 6: DLL_PROCESS_DETACH alone isn't reliable enough to depend on
// for persistence (many real shutdown paths skip it entirely), so a background thread flushes
// periodically whenever the cache is dirty. Started once from DllMain's DLL_PROCESS_ATTACH.
void Dx12PsoCache_StartFlushThread();

// Called from DllMain's DLL_PROCESS_DETACH - an unconditional final flush attempt (unlike the
// periodic thread, which only flushes when dirty), in case that ever actually runs.
void Dx12PsoCache_Shutdown(const char *selfDir);
