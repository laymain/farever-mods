#include "symbols.h"

#include "refresh.h"
#include "../log.h"

#include <windows.h>
#include <cstdio>

Fn_get_device GReal_get_device = NULL;
Fn_resource_release GReal_resource_release = NULL;
Fn_resource_set_name GReal_resource_set_name = NULL;
Fn_resource_get_gpu_virtual_address GReal_resource_get_gpu_virtual_address = NULL;
Fn_get_required_intermediate_size GReal_get_required_intermediate_size = NULL;
Fn_resource_map GReal_resource_map = NULL;
Fn_resource_unmap GReal_resource_unmap = NULL;
Fn_command_queue_create GReal_command_queue_create = NULL;
Fn_command_queue_execute_command_list GReal_command_queue_execute_command_list = NULL;
Fn_command_queue_execute_command_lists GReal_command_queue_execute_command_lists = NULL;
Fn_command_queue_signal GReal_command_queue_signal = NULL;
Fn_command_queue_wait GReal_command_queue_wait = NULL;
Fn_command_allocator_create GReal_command_allocator_create = NULL;
Fn_command_allocator_reset GReal_command_allocator_reset = NULL;
Fn_command_list_create GReal_command_list_create = NULL;
Fn_command_list_close GReal_command_list_close = NULL;
Fn_command_list_execute GReal_command_list_execute = NULL;
Fn_command_list_clear_render_target_view GReal_command_list_clear_render_target_view = NULL;
Fn_command_list_clear_depth_stencil_view GReal_command_list_clear_depth_stencil_view = NULL;
Fn_command_list_reset GReal_command_list_reset = NULL;
Fn_command_list_resource_barrier GReal_command_list_resource_barrier = NULL;
Fn_command_list_resource_barriers GReal_command_list_resource_barriers = NULL;
Fn_command_list_set_pipeline_state GReal_command_list_set_pipeline_state = NULL;
Fn_command_list_set_descriptor_heaps GReal_command_list_set_descriptor_heaps = NULL;
Fn_command_list_copy_buffer_region GReal_command_list_copy_buffer_region = NULL;
Fn_command_list_copy_texture_region GReal_command_list_copy_texture_region = NULL;
Fn_command_list_set_graphics_root_signature GReal_command_list_set_graphics_root_signature = NULL;
Fn_command_list_set_graphics_root32_bit_constants GReal_command_list_set_graphics_root32_bit_constants = NULL;
Fn_command_list_set_graphics_root_constant_buffer_view GReal_command_list_set_graphics_root_constant_buffer_view = NULL;
Fn_command_list_set_graphics_root_descriptor_table GReal_command_list_set_graphics_root_descriptor_table = NULL;
Fn_command_list_set_graphics_root_shader_resource_view GReal_command_list_set_graphics_root_shader_resource_view = NULL;
Fn_command_list_set_graphics_root_unordered_access_view GReal_command_list_set_graphics_root_unordered_access_view = NULL;
Fn_command_list_ia_set_primitive_topology GReal_command_list_ia_set_primitive_topology = NULL;
Fn_command_list_ia_set_vertex_buffers GReal_command_list_ia_set_vertex_buffers = NULL;
Fn_command_list_ia_set_index_buffer GReal_command_list_ia_set_index_buffer = NULL;
Fn_command_list_draw_instanced GReal_command_list_draw_instanced = NULL;
Fn_command_list_draw_indexed_instanced GReal_command_list_draw_indexed_instanced = NULL;
Fn_command_list_execute_indirect GReal_command_list_execute_indirect = NULL;
Fn_command_list_om_set_render_targets GReal_command_list_om_set_render_targets = NULL;
Fn_command_list_om_set_stencil_ref GReal_command_list_om_set_stencil_ref = NULL;
Fn_command_list_rs_set_viewports GReal_command_list_rs_set_viewports = NULL;
Fn_command_list_rs_set_scissor_rects GReal_command_list_rs_set_scissor_rects = NULL;
Fn_command_list_begin_query GReal_command_list_begin_query = NULL;
Fn_command_list_end_query GReal_command_list_end_query = NULL;
Fn_command_list_resolve_query_data GReal_command_list_resolve_query_data = NULL;
Fn_command_list_set_predication GReal_command_list_set_predication = NULL;
Fn_command_list_set_compute_root_signature GReal_command_list_set_compute_root_signature = NULL;
Fn_command_list_set_compute_root32_bit_constants GReal_command_list_set_compute_root32_bit_constants = NULL;
Fn_command_list_set_compute_root_constant_buffer_view GReal_command_list_set_compute_root_constant_buffer_view = NULL;
Fn_command_list_set_compute_root_descriptor_table GReal_command_list_set_compute_root_descriptor_table = NULL;
Fn_command_list_set_compute_root_shader_resource_view GReal_command_list_set_compute_root_shader_resource_view = NULL;
Fn_command_list_set_compute_root_unordered_access_view GReal_command_list_set_compute_root_unordered_access_view = NULL;
Fn_command_list_dispatch GReal_command_list_dispatch = NULL;
Fn_fence_create GReal_fence_create = NULL;
Fn_fence_get_completed_value GReal_fence_get_completed_value = NULL;
Fn_fence_set_event GReal_fence_set_event = NULL;
Fn_waitevent_create GReal_waitevent_create = NULL;
Fn_waitevent_wait GReal_waitevent_wait = NULL;
Fn_compiler_create GReal_compiler_create = NULL;
Fn_compiler_compile GReal_compiler_compile = NULL;
Fn_descriptor_heap_create GReal_descriptor_heap_create = NULL;
Fn_descriptor_heap_get_handle GReal_descriptor_heap_get_handle = NULL;
Fn_rootsignature_create GReal_rootsignature_create = NULL;
Fn_create GReal_create = NULL;
Fn_get_adapter GReal_get_adapter = NULL;
Fn_flush_messages GReal_flush_messages = NULL;
Fn_suppress_debug_messages GReal_suppress_debug_messages = NULL;
Fn_get_descriptor_handle_increment_size GReal_get_descriptor_handle_increment_size = NULL;
Fn_create_graphics_pipeline_state GReal_create_graphics_pipeline_state = NULL;
Fn_create_compute_pipeline_state GReal_create_compute_pipeline_state = NULL;
Fn_serialize_root_signature GReal_serialize_root_signature = NULL;
Fn_get_back_buffer GReal_get_back_buffer = NULL;
Fn_get_current_back_buffer_index GReal_get_current_back_buffer_index = NULL;
Fn_create_render_target_view GReal_create_render_target_view = NULL;
Fn_create_depth_stencil_view GReal_create_depth_stencil_view = NULL;
Fn_create_constant_buffer_view GReal_create_constant_buffer_view = NULL;
Fn_create_unordered_access_view GReal_create_unordered_access_view = NULL;
Fn_create_shader_resource_view GReal_create_shader_resource_view = NULL;
Fn_create_query_heap GReal_create_query_heap = NULL;
Fn_get_copyable_footprints GReal_get_copyable_footprints = NULL;
Fn_create_sampler GReal_create_sampler = NULL;
Fn_create_committed_resource GReal_create_committed_resource = NULL;
Fn_create_command_signature GReal_create_command_signature = NULL;
Fn_resize GReal_resize = NULL;
Fn_update_sub_resource GReal_update_sub_resource = NULL;
Fn_signal GReal_signal = NULL;
Fn_wait GReal_wait = NULL;
Fn_present GReal_present = NULL;
Fn_suspend GReal_suspend = NULL;
Fn_resume GReal_resume = NULL;
Fn_get_constant GReal_get_constant = NULL;
Fn_copy_descriptors_simple GReal_copy_descriptors_simple = NULL;
Fn_check_feature_support GReal_check_feature_support = NULL;
Fn_get_device_name GReal_get_device_name = NULL;
Fn_list_devices GReal_list_devices = NULL;
Fn_get_timestamp_frequency GReal_get_timestamp_frequency = NULL;
Fn_get_driver_version GReal_get_driver_version = NULL;
Fn_query_video_memory_info GReal_query_video_memory_info = NULL;

struct NativeForward {
	const char *name;
	void **slot;
};

static NativeForward GForwardTable[] = {
	{ "get_device", (void **)&GReal_get_device },
	{ "resource_release", (void **)&GReal_resource_release },
	{ "resource_set_name", (void **)&GReal_resource_set_name },
	{ "resource_get_gpu_virtual_address", (void **)&GReal_resource_get_gpu_virtual_address },
	{ "get_required_intermediate_size", (void **)&GReal_get_required_intermediate_size },
	{ "resource_map", (void **)&GReal_resource_map },
	{ "resource_unmap", (void **)&GReal_resource_unmap },
	{ "command_queue_create", (void **)&GReal_command_queue_create },
	{ "command_queue_execute_command_list", (void **)&GReal_command_queue_execute_command_list },
	{ "command_queue_execute_command_lists", (void **)&GReal_command_queue_execute_command_lists },
	{ "command_queue_signal", (void **)&GReal_command_queue_signal },
	{ "command_queue_wait", (void **)&GReal_command_queue_wait },
	{ "command_allocator_create", (void **)&GReal_command_allocator_create },
	{ "command_allocator_reset", (void **)&GReal_command_allocator_reset },
	{ "command_list_create", (void **)&GReal_command_list_create },
	{ "command_list_close", (void **)&GReal_command_list_close },
	{ "command_list_execute", (void **)&GReal_command_list_execute },
	{ "command_list_clear_render_target_view", (void **)&GReal_command_list_clear_render_target_view },
	{ "command_list_clear_depth_stencil_view", (void **)&GReal_command_list_clear_depth_stencil_view },
	{ "command_list_reset", (void **)&GReal_command_list_reset },
	{ "command_list_resource_barrier", (void **)&GReal_command_list_resource_barrier },
	{ "command_list_resource_barriers", (void **)&GReal_command_list_resource_barriers },
	{ "command_list_set_pipeline_state", (void **)&GReal_command_list_set_pipeline_state },
	{ "command_list_set_descriptor_heaps", (void **)&GReal_command_list_set_descriptor_heaps },
	{ "command_list_copy_buffer_region", (void **)&GReal_command_list_copy_buffer_region },
	{ "command_list_copy_texture_region", (void **)&GReal_command_list_copy_texture_region },
	{ "command_list_set_graphics_root_signature", (void **)&GReal_command_list_set_graphics_root_signature },
	{ "command_list_set_graphics_root32_bit_constants", (void **)&GReal_command_list_set_graphics_root32_bit_constants },
	{ "command_list_set_graphics_root_constant_buffer_view", (void **)&GReal_command_list_set_graphics_root_constant_buffer_view },
	{ "command_list_set_graphics_root_descriptor_table", (void **)&GReal_command_list_set_graphics_root_descriptor_table },
	{ "command_list_set_graphics_root_shader_resource_view", (void **)&GReal_command_list_set_graphics_root_shader_resource_view },
	{ "command_list_set_graphics_root_unordered_access_view", (void **)&GReal_command_list_set_graphics_root_unordered_access_view },
	{ "command_list_ia_set_primitive_topology", (void **)&GReal_command_list_ia_set_primitive_topology },
	{ "command_list_ia_set_vertex_buffers", (void **)&GReal_command_list_ia_set_vertex_buffers },
	{ "command_list_ia_set_index_buffer", (void **)&GReal_command_list_ia_set_index_buffer },
	{ "command_list_draw_instanced", (void **)&GReal_command_list_draw_instanced },
	{ "command_list_draw_indexed_instanced", (void **)&GReal_command_list_draw_indexed_instanced },
	{ "command_list_execute_indirect", (void **)&GReal_command_list_execute_indirect },
	{ "command_list_om_set_render_targets", (void **)&GReal_command_list_om_set_render_targets },
	{ "command_list_om_set_stencil_ref", (void **)&GReal_command_list_om_set_stencil_ref },
	{ "command_list_rs_set_viewports", (void **)&GReal_command_list_rs_set_viewports },
	{ "command_list_rs_set_scissor_rects", (void **)&GReal_command_list_rs_set_scissor_rects },
	{ "command_list_begin_query", (void **)&GReal_command_list_begin_query },
	{ "command_list_end_query", (void **)&GReal_command_list_end_query },
	{ "command_list_resolve_query_data", (void **)&GReal_command_list_resolve_query_data },
	{ "command_list_set_predication", (void **)&GReal_command_list_set_predication },
	{ "command_list_set_compute_root_signature", (void **)&GReal_command_list_set_compute_root_signature },
	{ "command_list_set_compute_root32_bit_constants", (void **)&GReal_command_list_set_compute_root32_bit_constants },
	{ "command_list_set_compute_root_constant_buffer_view", (void **)&GReal_command_list_set_compute_root_constant_buffer_view },
	{ "command_list_set_compute_root_descriptor_table", (void **)&GReal_command_list_set_compute_root_descriptor_table },
	{ "command_list_set_compute_root_shader_resource_view", (void **)&GReal_command_list_set_compute_root_shader_resource_view },
	{ "command_list_set_compute_root_unordered_access_view", (void **)&GReal_command_list_set_compute_root_unordered_access_view },
	{ "command_list_dispatch", (void **)&GReal_command_list_dispatch },
	{ "fence_create", (void **)&GReal_fence_create },
	{ "fence_get_completed_value", (void **)&GReal_fence_get_completed_value },
	{ "fence_set_event", (void **)&GReal_fence_set_event },
	{ "waitevent_create", (void **)&GReal_waitevent_create },
	{ "waitevent_wait", (void **)&GReal_waitevent_wait },
	{ "compiler_create", (void **)&GReal_compiler_create },
	{ "compiler_compile", (void **)&GReal_compiler_compile },
	{ "descriptor_heap_create", (void **)&GReal_descriptor_heap_create },
	{ "descriptor_heap_get_handle", (void **)&GReal_descriptor_heap_get_handle },
	{ "rootsignature_create", (void **)&GReal_rootsignature_create },
	{ "create", (void **)&GReal_create },
	{ "get_adapter", (void **)&GReal_get_adapter },
	{ "flush_messages", (void **)&GReal_flush_messages },
	{ "suppress_debug_messages", (void **)&GReal_suppress_debug_messages },
	{ "get_descriptor_handle_increment_size", (void **)&GReal_get_descriptor_handle_increment_size },
	{ "create_graphics_pipeline_state", (void **)&GReal_create_graphics_pipeline_state },
	{ "create_compute_pipeline_state", (void **)&GReal_create_compute_pipeline_state },
	{ "serialize_root_signature", (void **)&GReal_serialize_root_signature },
	{ "get_back_buffer", (void **)&GReal_get_back_buffer },
	{ "get_current_back_buffer_index", (void **)&GReal_get_current_back_buffer_index },
	{ "create_render_target_view", (void **)&GReal_create_render_target_view },
	{ "create_depth_stencil_view", (void **)&GReal_create_depth_stencil_view },
	{ "create_constant_buffer_view", (void **)&GReal_create_constant_buffer_view },
	{ "create_unordered_access_view", (void **)&GReal_create_unordered_access_view },
	{ "create_shader_resource_view", (void **)&GReal_create_shader_resource_view },
	{ "create_query_heap", (void **)&GReal_create_query_heap },
	{ "get_copyable_footprints", (void **)&GReal_get_copyable_footprints },
	{ "create_sampler", (void **)&GReal_create_sampler },
	{ "create_committed_resource", (void **)&GReal_create_committed_resource },
	{ "create_command_signature", (void **)&GReal_create_command_signature },
	{ "resize", (void **)&GReal_resize },
	{ "update_sub_resource", (void **)&GReal_update_sub_resource },
	{ "signal", (void **)&GReal_signal },
	{ "wait", (void **)&GReal_wait },
	{ "present", (void **)&GReal_present },
	{ "suspend", (void **)&GReal_suspend },
	{ "resume", (void **)&GReal_resume },
	{ "get_constant", (void **)&GReal_get_constant },
	{ "copy_descriptors_simple", (void **)&GReal_copy_descriptors_simple },
	{ "check_feature_support", (void **)&GReal_check_feature_support },
	{ "get_device_name", (void **)&GReal_get_device_name },
	{ "list_devices", (void **)&GReal_list_devices },
	{ "get_timestamp_frequency", (void **)&GReal_get_timestamp_frequency },
	{ "get_driver_version", (void **)&GReal_get_driver_version },
	{ "query_video_memory_info", (void **)&GReal_query_video_memory_info },
};

static HMODULE GImplModule = NULL;

// GetProcAddress's real return type (FARPROC) and each GReal_<name> global have different
// C++ types by construction (one per real native's own signature) - this resolves all 98
// through one generic loop via the same void** trick shadercache.cpp and ordinary Win32
// GetProcAddress-table code both already rely on (function pointers and data pointers share
// representation on this platform/ABI; MSVC/x64 only, matching this whole project's own
// Windows-only constraint).
static void ResolveForwards(HMODULE hImpl) {
	int resolved = 0, missing = 0;
	for (size_t i = 0; i < sizeof(GForwardTable) / sizeof(GForwardTable[0]); i++) {
		char symbol[128];
		snprintf(symbol, sizeof(symbol), "dx12_%s", GForwardTable[i].name);
		void *addr = (void *)GetProcAddress(hImpl, symbol);
		*GForwardTable[i].slot = addr;
		if (addr) {
			resolved++;
		} else {
			missing++;
			Dx12ShadowLog("dx12_proxy", "ResolveForwards: GetProcAddress(%s) failed, err=%lu", symbol, GetLastError());
		}
	}
	Dx12ShadowLog("dx12_proxy", "ResolveForwards: %d resolved, %d missing (of %d)", resolved, missing,
		(int)(sizeof(GForwardTable) / sizeof(GForwardTable[0])));
}

void LoadDx12Impl(const char *selfDir) {
	char implPath[MAX_PATH];
	GetDx12ImplPath(selfDir, implPath, sizeof(implPath));

	GImplModule = LoadLibraryA(implPath);
	if (!GImplModule) {
		Dx12ShadowLog("dx12_proxy", "LoadDx12Impl: LoadLibraryA(%s) failed, err=%lu", implPath, GetLastError());
		return;
	}
	Dx12ShadowLog("dx12_proxy", "LoadDx12Impl: loaded %s (handle=%p)", implPath, (void *)GImplModule);

	ResolveForwards(GImplModule);
}
