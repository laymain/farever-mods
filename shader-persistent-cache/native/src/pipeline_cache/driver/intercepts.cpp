#include "../types.h"
#include "../common.h"
#include "../store.h"
#include "../hash.h"
#include "symbols.h"
#include "../log.h"

// The 4 natives with real cache-integration logic, as opposed to forwarding.cpp's 94 plain
// forwards.

// NATIVE.md Section 7: "forward-and-observe, not a full custom implementation" - calls through to
// dx12_original.hdll's real get_device(), and the first time it returns non-null, captures that
// ID3D12Device* and hands it to the PSO cache (NATIVE.md Section 12 milestone 4) without changing
// what the caller sees.
static dx_device GCapturedDevice = NULL;

HL_PRIM dx_device HL_NAME(get_device)() {
	dx_device result = GReal_get_device ? GReal_get_device() : NULL;
	if (result && !GCapturedDevice) {
		GCapturedDevice = result;
		Dx12ShadowLog("dx12_proxy", "get_device: captured real ID3D12Device* = %p", result);
		Dx12PsoCache_OnDeviceCaptured(result, GProxySelfDir);
	}
	return result;
}

DEFINE_PRIM(_DEVICE, get_device, _NO_ARG);

// Intercepted (not a plain forward) so the PSO cache key can hash root signatures by CONTENT -
// see hash.h's own comment on pRootSignature for why.
HL_PRIM dx_resource HL_NAME(rootsignature_create)(vbyte * a0, int a1) {
	dx_resource created = GReal_rootsignature_create ? GReal_rootsignature_create(a0, a1) : NULL;
	if (created) RecordRootSignatureHash(created, a0, a1);
	return created;
}

DEFINE_PRIM(_RESOURCE, rootsignature_create, _BYTES _I32);

HL_PRIM dx_resource HL_NAME(create_graphics_pipeline_state)(void * a0) {
	return Dx12PsoCache_CreateGraphicsPipeline(a0, GReal_create_graphics_pipeline_state);
}

DEFINE_PRIM(_RESOURCE, create_graphics_pipeline_state, _STRUCT);

HL_PRIM dx_resource HL_NAME(create_compute_pipeline_state)(void * a0) {
	return Dx12PsoCache_CreateComputePipeline(a0, GReal_create_compute_pipeline_state);
}

DEFINE_PRIM(_RESOURCE, create_compute_pipeline_state, _STRUCT);
