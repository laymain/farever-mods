#pragma once

#include <d3d12.h>
#include <cstdint>
#include <cwchar>

// ---- PSO cache key (NATIVE.md Section 9) ----
// FNV-1a, folded over every field Section 9 calls out as required for correct pipeline identity
// - never render-state alone (Section 10 point 1 / shader-cache/REPORT.md UPDATE 7: the game's
// own hashSign is only guaranteed unique within one shader's own private lookup map, not
// globally).
//
// pRootSignature needs care - Section 9 lists "root signature identity", but the only handle
// this export has is the raw ID3D12RootSignature* pointer. Two rounds got this wrong before
// landing here:
//   Round 1: hashed the raw pointer directly. Broke cross-session persistence entirely - the
//   root signature object is recreated at a fresh heap address every launch (ASLR + new
//   allocation), so the pointer can never match across runs no matter how identical the actual
//   pipeline is.
//   Round 2: dropped it from the key entirely, reasoning shader bytecode content is already a
//   strong-enough proxy. A live test disproved this: the SAME hash showed up as a cache hit and
//   then, shortly after, a cache miss - two calls that hash identically under VS/PS/render-state
//   alone actually needed two DIFFERENT root signature objects (plausible if the engine creates a
//   fresh one per shader/material instantiation rather than deduplicating them itself), and
//   LoadGraphicsPipeline's own full-desc exact-match validation correctly rejected the mismatch -
//   after which StorePipeline under the same, already-claimed name failed as a duplicate (benign),
//   so that second variant could never actually get cached.
//   Fix: hash the root signature's own CONTENT, not its pointer or its absence. There's no API to
//   recover an ID3D12RootSignature's serialized bytes from its pointer after creation, so
//   RecordRootSignatureHash captures the blob at the one place it's actually available -
//   rootsignature_create's own input bytes - keyed by the pointer rootsignature_create returns.
//   Same content -> same hash regardless of which ID3D12RootSignature object or which process
//   produced it.

// Called from rootsignature_create's own intercept, with the same input blob it was created from.
void RecordRootSignatureHash(void *rootSig, const void *blob, int blobSize);

uint64_t HashGraphicsPipelineKey(const D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc);
uint64_t HashComputePipelineKey(const D3D12_COMPUTE_PIPELINE_STATE_DESC *desc);

// LoadGraphicsPipeline/LoadComputePipeline/StorePipeline all key by LPCWSTR name - format the
// 64-bit hash as a fixed-width hex string rather than reusing it as a raw binary name.
void FormatCacheKeyName(uint64_t hash, wchar_t *outName, size_t outNameCount);
