#include "hash.h"

#include <windows.h>
#include <cstring>
#include <unordered_map>

static const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
static const uint64_t FNV_PRIME = 1099511628211ULL;

static uint64_t FnvHashBytes(uint64_t hash, const void *data, size_t len) {
	const unsigned char *p = (const unsigned char *)data;
	for (size_t i = 0; i < len; i++) {
		hash ^= p[i];
		hash *= FNV_PRIME;
	}
	return hash;
}

template <typename T>
static uint64_t FnvHashValue(uint64_t hash, const T &value) {
	return FnvHashBytes(hash, &value, sizeof(T));
}

// Populated by rootsignature_create's own intercept (driver/intercepts.cpp), read by
// HashGraphicsPipelineKey/HashComputePipelineKey below.
static SRWLOCK GRootSigHashLock = SRWLOCK_INIT;
static std::unordered_map<void *, uint64_t> GRootSigHashes;

void RecordRootSignatureHash(void *rootSig, const void *blob, int blobSize) {
	if (!rootSig || !blob || blobSize <= 0) return;
	uint64_t contentHash = FnvHashBytes(FNV_OFFSET_BASIS, blob, (size_t)blobSize);
	AcquireSRWLockExclusive(&GRootSigHashLock);
	GRootSigHashes[rootSig] = contentHash;
	ReleaseSRWLockExclusive(&GRootSigHashLock);
}

static uint64_t LookupRootSignatureHash(void *rootSig) {
	if (!rootSig) return 0;
	AcquireSRWLockExclusive(&GRootSigHashLock);
	auto it = GRootSigHashes.find(rootSig);
	// Not found should not happen in practice (rootsignature_create is the only creation path),
	// but if it ever does, the bare pointer is still strictly better than nothing here: it's at
	// least self-consistent for the rest of this one session, just not cross-session-stable.
	uint64_t result = (it != GRootSigHashes.end()) ? it->second : (uint64_t)(uintptr_t)rootSig;
	ReleaseSRWLockExclusive(&GRootSigHashLock);
	return result;
}

static uint64_t FnvHashBytecode(uint64_t hash, const D3D12_SHADER_BYTECODE &bc) {
	hash = FnvHashValue(hash, bc.BytecodeLength);
	if (bc.pShaderBytecode && bc.BytecodeLength > 0)
		hash = FnvHashBytes(hash, bc.pShaderBytecode, (size_t)bc.BytecodeLength);
	return hash;
}

static uint64_t FnvHashInputLayout(uint64_t hash, const D3D12_INPUT_LAYOUT_DESC &layout) {
	hash = FnvHashValue(hash, layout.NumElements);
	for (UINT i = 0; i < layout.NumElements; i++) {
		const D3D12_INPUT_ELEMENT_DESC &e = layout.pInputElementDescs[i];
		if (e.SemanticName) hash = FnvHashBytes(hash, e.SemanticName, strlen(e.SemanticName));
		hash = FnvHashValue(hash, e.SemanticIndex);
		hash = FnvHashValue(hash, e.Format);
		hash = FnvHashValue(hash, e.InputSlot);
		hash = FnvHashValue(hash, e.AlignedByteOffset);
		hash = FnvHashValue(hash, e.InputSlotClass);
		hash = FnvHashValue(hash, e.InstanceDataStepRate);
	}
	return hash;
}

uint64_t HashGraphicsPipelineKey(const D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc) {
	uint64_t hash = FNV_OFFSET_BASIS;
	hash = FnvHashValue(hash, LookupRootSignatureHash(desc->pRootSignature));
	hash = FnvHashBytecode(hash, desc->VS);
	hash = FnvHashBytecode(hash, desc->PS);
	hash = FnvHashBytecode(hash, desc->DS);
	hash = FnvHashBytecode(hash, desc->HS);
	hash = FnvHashBytecode(hash, desc->GS);
	hash = FnvHashInputLayout(hash, desc->InputLayout);
	hash = FnvHashValue(hash, desc->BlendState);
	hash = FnvHashValue(hash, desc->RasterizerState);
	hash = FnvHashValue(hash, desc->DepthStencilState);
	hash = FnvHashValue(hash, desc->SampleMask);
	hash = FnvHashValue(hash, desc->IBStripCutValue);
	hash = FnvHashValue(hash, desc->PrimitiveTopologyType);
	hash = FnvHashValue(hash, desc->NumRenderTargets);
	hash = FnvHashBytes(hash, desc->RTVFormats, sizeof(desc->RTVFormats));
	hash = FnvHashValue(hash, desc->DSVFormat);
	hash = FnvHashValue(hash, desc->SampleDesc);
	hash = FnvHashValue(hash, desc->Flags);
	return hash;
}

uint64_t HashComputePipelineKey(const D3D12_COMPUTE_PIPELINE_STATE_DESC *desc) {
	uint64_t hash = FNV_OFFSET_BASIS;
	hash = FnvHashValue(hash, LookupRootSignatureHash(desc->pRootSignature));
	hash = FnvHashBytecode(hash, desc->CS);
	hash = FnvHashValue(hash, desc->Flags);
	return hash;
}

void FormatCacheKeyName(uint64_t hash, wchar_t *outName, size_t outNameCount) {
	swprintf_s(outName, outNameCount, L"%016llx", (unsigned long long)hash);
}
