#include "../types.h"
#include "../common.h"
#include "symbols.h"
#include "../log.h"

// ---- Forwarding exports (Section 7) ----
// The 94 dx12 natives with no cache-integration logic: each just forwards straight through to
// dx12_original.hdll's own real implementation. get_device, rootsignature_create,
// create_graphics_pipeline_state and create_compute_pipeline_state are the 4 exceptions - see
// intercepts.cpp.

HL_PRIM void HL_NAME(resource_release)(dx_resource a0) {
	if (GReal_resource_release) GReal_resource_release(a0);
}

DEFINE_PRIM(_VOID, resource_release, _RESOURCE);

HL_PRIM void HL_NAME(resource_set_name)(dx_resource a0, vbyte * a1) {
	if (GReal_resource_set_name) GReal_resource_set_name(a0, a1);
}

DEFINE_PRIM(_VOID, resource_set_name, _RESOURCE _BYTES);

HL_PRIM int64 HL_NAME(resource_get_gpu_virtual_address)(dx_resource a0) {
	if (GReal_resource_get_gpu_virtual_address) return GReal_resource_get_gpu_virtual_address(a0);
	return 0;
}

DEFINE_PRIM(_I64, resource_get_gpu_virtual_address, _RESOURCE);

HL_PRIM int64 HL_NAME(get_required_intermediate_size)(dx_resource a0, int a1, int a2) {
	if (GReal_get_required_intermediate_size) return GReal_get_required_intermediate_size(a0, a1, a2);
	return 0;
}

DEFINE_PRIM(_I64, get_required_intermediate_size, _RESOURCE _I32 _I32);

HL_PRIM vbyte * HL_NAME(resource_map)(dx_resource a0, int a1, void * a2) {
	if (GReal_resource_map) return GReal_resource_map(a0, a1, a2);
	return NULL;
}

DEFINE_PRIM(_BYTES, resource_map, _RESOURCE _I32 _STRUCT);

HL_PRIM void HL_NAME(resource_unmap)(dx_resource a0, int a1, void * a2) {
	if (GReal_resource_unmap) GReal_resource_unmap(a0, a1, a2);
}

DEFINE_PRIM(_VOID, resource_unmap, _RESOURCE _I32 _STRUCT);

HL_PRIM dx_resource HL_NAME(command_queue_create)(int a0) {
	if (GReal_command_queue_create) return GReal_command_queue_create(a0);
	return NULL;
}

DEFINE_PRIM(_RESOURCE, command_queue_create, _I32);

HL_PRIM void HL_NAME(command_queue_execute_command_list)(dx_resource a0, dx_resource a1) {
	if (GReal_command_queue_execute_command_list) GReal_command_queue_execute_command_list(a0, a1);
}

DEFINE_PRIM(_VOID, command_queue_execute_command_list, _RESOURCE _RESOURCE);

HL_PRIM void HL_NAME(command_queue_execute_command_lists)(dx_resource a0, void * a1, int a2) {
	if (GReal_command_queue_execute_command_lists) GReal_command_queue_execute_command_lists(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_queue_execute_command_lists, _RESOURCE _CARRAY _I32);

HL_PRIM void HL_NAME(command_queue_signal)(dx_resource a0, dx_resource a1, int64 a2) {
	if (GReal_command_queue_signal) GReal_command_queue_signal(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_queue_signal, _RESOURCE _RESOURCE _I64);

HL_PRIM void HL_NAME(command_queue_wait)(dx_resource a0, dx_resource a1, int64 a2) {
	if (GReal_command_queue_wait) GReal_command_queue_wait(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_queue_wait, _RESOURCE _RESOURCE _I64);

HL_PRIM dx_resource HL_NAME(command_allocator_create)(int a0) {
	if (GReal_command_allocator_create) return GReal_command_allocator_create(a0);
	return NULL;
}

DEFINE_PRIM(_RESOURCE, command_allocator_create, _I32);

HL_PRIM void HL_NAME(command_allocator_reset)(dx_resource a0) {
	if (GReal_command_allocator_reset) GReal_command_allocator_reset(a0);
}

DEFINE_PRIM(_VOID, command_allocator_reset, _RESOURCE);

HL_PRIM dx_resource HL_NAME(command_list_create)(int a0, dx_resource a1, dx_resource a2) {
	if (GReal_command_list_create) return GReal_command_list_create(a0, a1, a2);
	return NULL;
}

DEFINE_PRIM(_RESOURCE, command_list_create, _I32 _RESOURCE _RESOURCE);

HL_PRIM void HL_NAME(command_list_close)(dx_resource a0) {
	if (GReal_command_list_close) GReal_command_list_close(a0);
}

DEFINE_PRIM(_VOID, command_list_close, _RESOURCE);

HL_PRIM void HL_NAME(command_list_execute)(dx_resource a0) {
	if (GReal_command_list_execute) GReal_command_list_execute(a0);
}

DEFINE_PRIM(_VOID, command_list_execute, _RESOURCE);

HL_PRIM void HL_NAME(command_list_clear_render_target_view)(dx_resource a0, int64 a1, void * a2) {
	if (GReal_command_list_clear_render_target_view) GReal_command_list_clear_render_target_view(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_clear_render_target_view, _RESOURCE _I64 _STRUCT);

HL_PRIM void HL_NAME(command_list_clear_depth_stencil_view)(dx_resource a0, int64 a1, int a2, float a3, int a4) {
	if (GReal_command_list_clear_depth_stencil_view) GReal_command_list_clear_depth_stencil_view(a0, a1, a2, a3, a4);
}

DEFINE_PRIM(_VOID, command_list_clear_depth_stencil_view, _RESOURCE _I64 _I32 _F32 _I32);

HL_PRIM void HL_NAME(command_list_reset)(dx_resource a0, dx_resource a1, dx_resource a2) {
	if (GReal_command_list_reset) GReal_command_list_reset(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_reset, _RESOURCE _RESOURCE _RESOURCE);

HL_PRIM void HL_NAME(command_list_resource_barrier)(dx_resource a0, void * a1) {
	if (GReal_command_list_resource_barrier) GReal_command_list_resource_barrier(a0, a1);
}

DEFINE_PRIM(_VOID, command_list_resource_barrier, _RESOURCE _STRUCT);

HL_PRIM void HL_NAME(command_list_resource_barriers)(dx_resource a0, void * a1, int a2) {
	if (GReal_command_list_resource_barriers) GReal_command_list_resource_barriers(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_resource_barriers, _RESOURCE _CARRAY _I32);

HL_PRIM void HL_NAME(command_list_set_pipeline_state)(dx_resource a0, dx_resource a1) {
	if (GReal_command_list_set_pipeline_state) GReal_command_list_set_pipeline_state(a0, a1);
}

DEFINE_PRIM(_VOID, command_list_set_pipeline_state, _RESOURCE _RESOURCE);

HL_PRIM void HL_NAME(command_list_set_descriptor_heaps)(dx_resource a0, varray * a1) {
	if (GReal_command_list_set_descriptor_heaps) GReal_command_list_set_descriptor_heaps(a0, a1);
}

DEFINE_PRIM(_VOID, command_list_set_descriptor_heaps, _RESOURCE _ARR);

HL_PRIM void HL_NAME(command_list_copy_buffer_region)(dx_resource a0, dx_resource a1, int64 a2, dx_resource a3, int64 a4, int64 a5) {
	if (GReal_command_list_copy_buffer_region) GReal_command_list_copy_buffer_region(a0, a1, a2, a3, a4, a5);
}

DEFINE_PRIM(_VOID, command_list_copy_buffer_region, _RESOURCE _RESOURCE _I64 _RESOURCE _I64 _I64);

HL_PRIM void HL_NAME(command_list_copy_texture_region)(dx_resource a0, void * a1, int a2, int a3, int a4, void * a5, void * a6) {
	if (GReal_command_list_copy_texture_region) GReal_command_list_copy_texture_region(a0, a1, a2, a3, a4, a5, a6);
}

DEFINE_PRIM(_VOID, command_list_copy_texture_region, _RESOURCE _STRUCT _I32 _I32 _I32 _STRUCT _STRUCT);

HL_PRIM void HL_NAME(command_list_set_graphics_root_signature)(dx_resource a0, dx_resource a1) {
	if (GReal_command_list_set_graphics_root_signature) GReal_command_list_set_graphics_root_signature(a0, a1);
}

DEFINE_PRIM(_VOID, command_list_set_graphics_root_signature, _RESOURCE _RESOURCE);

HL_PRIM void HL_NAME(command_list_set_graphics_root32_bit_constants)(dx_resource a0, int a1, int a2, vbyte * a3, int a4) {
	if (GReal_command_list_set_graphics_root32_bit_constants) GReal_command_list_set_graphics_root32_bit_constants(a0, a1, a2, a3, a4);
}

DEFINE_PRIM(_VOID, command_list_set_graphics_root32_bit_constants, _RESOURCE _I32 _I32 _BYTES _I32);

HL_PRIM void HL_NAME(command_list_set_graphics_root_constant_buffer_view)(dx_resource a0, int a1, int64 a2) {
	if (GReal_command_list_set_graphics_root_constant_buffer_view) GReal_command_list_set_graphics_root_constant_buffer_view(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_set_graphics_root_constant_buffer_view, _RESOURCE _I32 _I64);

HL_PRIM void HL_NAME(command_list_set_graphics_root_descriptor_table)(dx_resource a0, int a1, int64 a2) {
	if (GReal_command_list_set_graphics_root_descriptor_table) GReal_command_list_set_graphics_root_descriptor_table(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_set_graphics_root_descriptor_table, _RESOURCE _I32 _I64);

HL_PRIM void HL_NAME(command_list_set_graphics_root_shader_resource_view)(dx_resource a0, int a1, int64 a2) {
	if (GReal_command_list_set_graphics_root_shader_resource_view) GReal_command_list_set_graphics_root_shader_resource_view(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_set_graphics_root_shader_resource_view, _RESOURCE _I32 _I64);

HL_PRIM void HL_NAME(command_list_set_graphics_root_unordered_access_view)(dx_resource a0, int a1, int64 a2) {
	if (GReal_command_list_set_graphics_root_unordered_access_view) GReal_command_list_set_graphics_root_unordered_access_view(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_set_graphics_root_unordered_access_view, _RESOURCE _I32 _I64);

HL_PRIM void HL_NAME(command_list_ia_set_primitive_topology)(dx_resource a0, int a1) {
	if (GReal_command_list_ia_set_primitive_topology) GReal_command_list_ia_set_primitive_topology(a0, a1);
}

DEFINE_PRIM(_VOID, command_list_ia_set_primitive_topology, _RESOURCE _I32);

HL_PRIM void HL_NAME(command_list_ia_set_vertex_buffers)(dx_resource a0, int a1, int a2, void * a3) {
	if (GReal_command_list_ia_set_vertex_buffers) GReal_command_list_ia_set_vertex_buffers(a0, a1, a2, a3);
}

DEFINE_PRIM(_VOID, command_list_ia_set_vertex_buffers, _RESOURCE _I32 _I32 _STRUCT);

HL_PRIM void HL_NAME(command_list_ia_set_index_buffer)(dx_resource a0, void * a1) {
	if (GReal_command_list_ia_set_index_buffer) GReal_command_list_ia_set_index_buffer(a0, a1);
}

DEFINE_PRIM(_VOID, command_list_ia_set_index_buffer, _RESOURCE _STRUCT);

HL_PRIM void HL_NAME(command_list_draw_instanced)(dx_resource a0, int a1, int a2, int a3, int a4) {
	if (GReal_command_list_draw_instanced) GReal_command_list_draw_instanced(a0, a1, a2, a3, a4);
}

DEFINE_PRIM(_VOID, command_list_draw_instanced, _RESOURCE _I32 _I32 _I32 _I32);

HL_PRIM void HL_NAME(command_list_draw_indexed_instanced)(dx_resource a0, int a1, int a2, int a3, int a4, int a5) {
	if (GReal_command_list_draw_indexed_instanced) GReal_command_list_draw_indexed_instanced(a0, a1, a2, a3, a4, a5);
}

DEFINE_PRIM(_VOID, command_list_draw_indexed_instanced, _RESOURCE _I32 _I32 _I32 _I32 _I32);

HL_PRIM void HL_NAME(command_list_execute_indirect)(dx_resource a0, dx_resource a1, int a2, dx_resource a3, int64 a4, dx_resource a5, int64 a6) {
	if (GReal_command_list_execute_indirect) GReal_command_list_execute_indirect(a0, a1, a2, a3, a4, a5, a6);
}

DEFINE_PRIM(_VOID, command_list_execute_indirect, _RESOURCE _RESOURCE _I32 _RESOURCE _I64 _RESOURCE _I64);

HL_PRIM void HL_NAME(command_list_om_set_render_targets)(dx_resource a0, int a1, vbyte * a2, int a3, vbyte * a4) {
	if (GReal_command_list_om_set_render_targets) GReal_command_list_om_set_render_targets(a0, a1, a2, a3, a4);
}

DEFINE_PRIM(_VOID, command_list_om_set_render_targets, _RESOURCE _I32 _BYTES _I32 _BYTES);

HL_PRIM void HL_NAME(command_list_om_set_stencil_ref)(dx_resource a0, int a1) {
	if (GReal_command_list_om_set_stencil_ref) GReal_command_list_om_set_stencil_ref(a0, a1);
}

DEFINE_PRIM(_VOID, command_list_om_set_stencil_ref, _RESOURCE _I32);

HL_PRIM void HL_NAME(command_list_rs_set_viewports)(dx_resource a0, int a1, void * a2) {
	if (GReal_command_list_rs_set_viewports) GReal_command_list_rs_set_viewports(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_rs_set_viewports, _RESOURCE _I32 _STRUCT);

HL_PRIM void HL_NAME(command_list_rs_set_scissor_rects)(dx_resource a0, int a1, void * a2) {
	if (GReal_command_list_rs_set_scissor_rects) GReal_command_list_rs_set_scissor_rects(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_rs_set_scissor_rects, _RESOURCE _I32 _STRUCT);

HL_PRIM void HL_NAME(command_list_begin_query)(dx_resource a0, dx_resource a1, int a2, int a3) {
	if (GReal_command_list_begin_query) GReal_command_list_begin_query(a0, a1, a2, a3);
}

DEFINE_PRIM(_VOID, command_list_begin_query, _RESOURCE _RESOURCE _I32 _I32);

HL_PRIM void HL_NAME(command_list_end_query)(dx_resource a0, dx_resource a1, int a2, int a3) {
	if (GReal_command_list_end_query) GReal_command_list_end_query(a0, a1, a2, a3);
}

DEFINE_PRIM(_VOID, command_list_end_query, _RESOURCE _RESOURCE _I32 _I32);

HL_PRIM void HL_NAME(command_list_resolve_query_data)(dx_resource a0, dx_resource a1, int a2, int a3, int a4, dx_resource a5, int64 a6) {
	if (GReal_command_list_resolve_query_data) GReal_command_list_resolve_query_data(a0, a1, a2, a3, a4, a5, a6);
}

DEFINE_PRIM(_VOID, command_list_resolve_query_data, _RESOURCE _RESOURCE _I32 _I32 _I32 _RESOURCE _I64);

HL_PRIM void HL_NAME(command_list_set_predication)(dx_resource a0, dx_resource a1, int64 a2, int a3) {
	if (GReal_command_list_set_predication) GReal_command_list_set_predication(a0, a1, a2, a3);
}

DEFINE_PRIM(_VOID, command_list_set_predication, _RESOURCE _RESOURCE _I64 _I32);

HL_PRIM void HL_NAME(command_list_set_compute_root_signature)(dx_resource a0, dx_resource a1) {
	if (GReal_command_list_set_compute_root_signature) GReal_command_list_set_compute_root_signature(a0, a1);
}

DEFINE_PRIM(_VOID, command_list_set_compute_root_signature, _RESOURCE _RESOURCE);

HL_PRIM void HL_NAME(command_list_set_compute_root32_bit_constants)(dx_resource a0, int a1, int a2, vbyte * a3, int a4) {
	if (GReal_command_list_set_compute_root32_bit_constants) GReal_command_list_set_compute_root32_bit_constants(a0, a1, a2, a3, a4);
}

DEFINE_PRIM(_VOID, command_list_set_compute_root32_bit_constants, _RESOURCE _I32 _I32 _BYTES _I32);

HL_PRIM void HL_NAME(command_list_set_compute_root_constant_buffer_view)(dx_resource a0, int a1, int64 a2) {
	if (GReal_command_list_set_compute_root_constant_buffer_view) GReal_command_list_set_compute_root_constant_buffer_view(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_set_compute_root_constant_buffer_view, _RESOURCE _I32 _I64);

HL_PRIM void HL_NAME(command_list_set_compute_root_descriptor_table)(dx_resource a0, int a1, int64 a2) {
	if (GReal_command_list_set_compute_root_descriptor_table) GReal_command_list_set_compute_root_descriptor_table(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_set_compute_root_descriptor_table, _RESOURCE _I32 _I64);

HL_PRIM void HL_NAME(command_list_set_compute_root_shader_resource_view)(dx_resource a0, int a1, int64 a2) {
	if (GReal_command_list_set_compute_root_shader_resource_view) GReal_command_list_set_compute_root_shader_resource_view(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_set_compute_root_shader_resource_view, _RESOURCE _I32 _I64);

HL_PRIM void HL_NAME(command_list_set_compute_root_unordered_access_view)(dx_resource a0, int a1, int64 a2) {
	if (GReal_command_list_set_compute_root_unordered_access_view) GReal_command_list_set_compute_root_unordered_access_view(a0, a1, a2);
}

DEFINE_PRIM(_VOID, command_list_set_compute_root_unordered_access_view, _RESOURCE _I32 _I64);

HL_PRIM void HL_NAME(command_list_dispatch)(dx_resource a0, int a1, int a2, int a3) {
	if (GReal_command_list_dispatch) GReal_command_list_dispatch(a0, a1, a2, a3);
}

DEFINE_PRIM(_VOID, command_list_dispatch, _RESOURCE _I32 _I32 _I32);

HL_PRIM dx_resource HL_NAME(fence_create)(int64 a0, int a1) {
	if (GReal_fence_create) return GReal_fence_create(a0, a1);
	return NULL;
}

DEFINE_PRIM(_RESOURCE, fence_create, _I64 _I32);

HL_PRIM int64 HL_NAME(fence_get_completed_value)(dx_resource a0) {
	if (GReal_fence_get_completed_value) return GReal_fence_get_completed_value(a0);
	return 0;
}

DEFINE_PRIM(_I64, fence_get_completed_value, _RESOURCE);

HL_PRIM void HL_NAME(fence_set_event)(dx_resource a0, int64 a1, dx_event a2) {
	if (GReal_fence_set_event) GReal_fence_set_event(a0, a1, a2);
}

DEFINE_PRIM(_VOID, fence_set_event, _RESOURCE _I64 _EVENT);

HL_PRIM dx_event HL_NAME(waitevent_create)(bool a0) {
	if (GReal_waitevent_create) return GReal_waitevent_create(a0);
	return NULL;
}

DEFINE_PRIM(_EVENT, waitevent_create, _BOOL);

HL_PRIM bool HL_NAME(waitevent_wait)(dx_event a0, int a1) {
	if (GReal_waitevent_wait) return GReal_waitevent_wait(a0, a1);
	return false;
}

DEFINE_PRIM(_BOOL, waitevent_wait, _EVENT _I32);

HL_PRIM dx_compiler HL_NAME(compiler_create)() {
	if (GReal_compiler_create) return GReal_compiler_create();
	return NULL;
}

DEFINE_PRIM(_COMPILER, compiler_create, _NO_ARG);

HL_PRIM vbyte * HL_NAME(compiler_compile)(dx_compiler a0, vbyte * a1, vbyte * a2, varray * a3, int * a4) {
	if (GReal_compiler_compile) return GReal_compiler_compile(a0, a1, a2, a3, a4);
	return NULL;
}

DEFINE_PRIM(_BYTES, compiler_compile, _COMPILER _BYTES _BYTES _ARR _REF(_I32));

HL_PRIM dx_resource HL_NAME(descriptor_heap_create)(void * a0) {
	if (GReal_descriptor_heap_create) return GReal_descriptor_heap_create(a0);
	return NULL;
}

DEFINE_PRIM(_RESOURCE, descriptor_heap_create, _STRUCT);

HL_PRIM int64 HL_NAME(descriptor_heap_get_handle)(dx_resource a0, bool a1) {
	if (GReal_descriptor_heap_get_handle) return GReal_descriptor_heap_get_handle(a0, a1);
	return 0;
}

DEFINE_PRIM(_I64, descriptor_heap_get_handle, _RESOURCE _BOOL);

HL_PRIM dx_driver HL_NAME(create)(dx_window a0, int a1, vbyte * a2) {
	if (GReal_create) return GReal_create(a0, a1, a2);
	return NULL;
}

DEFINE_PRIM(_DRIVER, create, _WINDOW _I32 _BYTES);

HL_PRIM dx_adapter HL_NAME(get_adapter)() {
	if (GReal_get_adapter) return GReal_get_adapter();
	return NULL;
}

DEFINE_PRIM(_ADAPTER, get_adapter, _NO_ARG);

HL_PRIM void HL_NAME(flush_messages)() {
	if (GReal_flush_messages) GReal_flush_messages();
}

DEFINE_PRIM(_VOID, flush_messages, _NO_ARG);

HL_PRIM void HL_NAME(suppress_debug_messages)(void * a0) {
	if (GReal_suppress_debug_messages) GReal_suppress_debug_messages(a0);
}

DEFINE_PRIM(_VOID, suppress_debug_messages, _STRUCT);

HL_PRIM int HL_NAME(get_descriptor_handle_increment_size)(int a0) {
	if (GReal_get_descriptor_handle_increment_size) return GReal_get_descriptor_handle_increment_size(a0);
	return 0;
}

DEFINE_PRIM(_I32, get_descriptor_handle_increment_size, _I32);

HL_PRIM vbyte * HL_NAME(serialize_root_signature)(void * a0, int a1, int * a2) {
	if (GReal_serialize_root_signature) return GReal_serialize_root_signature(a0, a1, a2);
	return NULL;
}

DEFINE_PRIM(_BYTES, serialize_root_signature, _STRUCT _I32 _REF(_I32));

HL_PRIM dx_resource HL_NAME(get_back_buffer)(int a0) {
	if (GReal_get_back_buffer) return GReal_get_back_buffer(a0);
	return NULL;
}

DEFINE_PRIM(_RESOURCE, get_back_buffer, _I32);

HL_PRIM int HL_NAME(get_current_back_buffer_index)() {
	if (GReal_get_current_back_buffer_index) return GReal_get_current_back_buffer_index();
	return 0;
}

DEFINE_PRIM(_I32, get_current_back_buffer_index, _NO_ARG);

HL_PRIM void HL_NAME(create_render_target_view)(dx_resource a0, void * a1, int64 a2) {
	if (GReal_create_render_target_view) GReal_create_render_target_view(a0, a1, a2);
}

DEFINE_PRIM(_VOID, create_render_target_view, _RESOURCE _STRUCT _I64);

HL_PRIM void HL_NAME(create_depth_stencil_view)(dx_resource a0, void * a1, int64 a2) {
	if (GReal_create_depth_stencil_view) GReal_create_depth_stencil_view(a0, a1, a2);
}

DEFINE_PRIM(_VOID, create_depth_stencil_view, _RESOURCE _STRUCT _I64);

HL_PRIM void HL_NAME(create_constant_buffer_view)(void * a0, int64 a1) {
	if (GReal_create_constant_buffer_view) GReal_create_constant_buffer_view(a0, a1);
}

DEFINE_PRIM(_VOID, create_constant_buffer_view, _STRUCT _I64);

HL_PRIM void HL_NAME(create_unordered_access_view)(dx_resource a0, dx_resource a1, void * a2, int64 a3) {
	if (GReal_create_unordered_access_view) GReal_create_unordered_access_view(a0, a1, a2, a3);
}

DEFINE_PRIM(_VOID, create_unordered_access_view, _RESOURCE _RESOURCE _STRUCT _I64);

HL_PRIM void HL_NAME(create_shader_resource_view)(dx_resource a0, void * a1, int64 a2) {
	if (GReal_create_shader_resource_view) GReal_create_shader_resource_view(a0, a1, a2);
}

DEFINE_PRIM(_VOID, create_shader_resource_view, _RESOURCE _STRUCT _I64);

HL_PRIM dx_resource HL_NAME(create_query_heap)(void * a0) {
	if (GReal_create_query_heap) return GReal_create_query_heap(a0);
	return NULL;
}

DEFINE_PRIM(_RESOURCE, create_query_heap, _STRUCT);

HL_PRIM void HL_NAME(get_copyable_footprints)(void * a0, int a1, int a2, int64 a3, void * a4, vbyte * a5, vbyte * a6, vbyte * a7) {
	if (GReal_get_copyable_footprints) GReal_get_copyable_footprints(a0, a1, a2, a3, a4, a5, a6, a7);
}

DEFINE_PRIM(_VOID, get_copyable_footprints, _STRUCT _I32 _I32 _I64 _STRUCT _BYTES _BYTES _BYTES);

HL_PRIM void HL_NAME(create_sampler)(void * a0, int64 a1) {
	if (GReal_create_sampler) GReal_create_sampler(a0, a1);
}

DEFINE_PRIM(_VOID, create_sampler, _STRUCT _I64);

HL_PRIM dx_resource HL_NAME(create_committed_resource)(void * a0, int a1, void * a2, int a3, void * a4) {
	if (GReal_create_committed_resource) return GReal_create_committed_resource(a0, a1, a2, a3, a4);
	return NULL;
}

DEFINE_PRIM(_RESOURCE, create_committed_resource, _STRUCT _I32 _STRUCT _I32 _STRUCT);

HL_PRIM dx_resource HL_NAME(create_command_signature)(void * a0, dx_resource a1) {
	if (GReal_create_command_signature) return GReal_create_command_signature(a0, a1);
	return NULL;
}

DEFINE_PRIM(_RESOURCE, create_command_signature, _STRUCT _RESOURCE);

HL_PRIM void HL_NAME(resize)(int a0, int a1, int a2, int a3) {
	if (GReal_resize) GReal_resize(a0, a1, a2, a3);
}

DEFINE_PRIM(_VOID, resize, _I32 _I32 _I32 _I32);

HL_PRIM bool HL_NAME(update_sub_resource)(dx_resource a0, dx_resource a1, dx_resource a2, int64 a3, int a4, int a5, void * a6) {
	if (GReal_update_sub_resource) return GReal_update_sub_resource(a0, a1, a2, a3, a4, a5, a6);
	return false;
}

DEFINE_PRIM(_BOOL, update_sub_resource, _RESOURCE _RESOURCE _RESOURCE _I64 _I32 _I32 _STRUCT);

HL_PRIM void HL_NAME(signal)(dx_resource a0, int64 a1) {
	if (GReal_signal) GReal_signal(a0, a1);
}

DEFINE_PRIM(_VOID, signal, _RESOURCE _I64);

HL_PRIM void HL_NAME(wait)(dx_resource a0, int64 a1) {
	if (GReal_wait) GReal_wait(a0, a1);
}

DEFINE_PRIM(_VOID, wait, _RESOURCE _I64);

HL_PRIM void HL_NAME(present)(bool a0) {
	if (GReal_present) GReal_present(a0);
}

DEFINE_PRIM(_VOID, present, _BOOL);

HL_PRIM void HL_NAME(suspend)() {
	if (GReal_suspend) GReal_suspend();
}

DEFINE_PRIM(_VOID, suspend, _NO_ARG);

HL_PRIM void HL_NAME(resume)() {
	if (GReal_resume) GReal_resume();
}

DEFINE_PRIM(_VOID, resume, _NO_ARG);

HL_PRIM int HL_NAME(get_constant)(int a0) {
	if (GReal_get_constant) return GReal_get_constant(a0);
	return 0;
}

DEFINE_PRIM(_I32, get_constant, _I32);

HL_PRIM void HL_NAME(copy_descriptors_simple)(int a0, int64 a1, int64 a2, int a3) {
	if (GReal_copy_descriptors_simple) GReal_copy_descriptors_simple(a0, a1, a2, a3);
}

DEFINE_PRIM(_VOID, copy_descriptors_simple, _I32 _I64 _I64 _I32);

HL_PRIM void HL_NAME(check_feature_support)(int a0, vbyte * a1, int a2) {
	if (GReal_check_feature_support) GReal_check_feature_support(a0, a1, a2);
}

DEFINE_PRIM(_VOID, check_feature_support, _I32 _BYTES _I32);

HL_PRIM vbyte * HL_NAME(get_device_name)() {
	if (GReal_get_device_name) return GReal_get_device_name();
	return NULL;
}

DEFINE_PRIM(_BYTES, get_device_name, _NO_ARG);

HL_PRIM varray * HL_NAME(list_devices)() {
	if (GReal_list_devices) return GReal_list_devices();
	return NULL;
}

DEFINE_PRIM(_ARR, list_devices, _NO_ARG);

HL_PRIM int64 HL_NAME(get_timestamp_frequency)() {
	if (GReal_get_timestamp_frequency) return GReal_get_timestamp_frequency();
	return 0;
}

DEFINE_PRIM(_I64, get_timestamp_frequency, _NO_ARG);

HL_PRIM int64 HL_NAME(get_driver_version)() {
	if (GReal_get_driver_version) return GReal_get_driver_version();
	return 0;
}

DEFINE_PRIM(_I64, get_driver_version, _NO_ARG);

HL_PRIM void HL_NAME(query_video_memory_info)(int a0, void * a1) {
	if (GReal_query_video_memory_info) GReal_query_video_memory_info(a0, a1);
}

DEFINE_PRIM(_VOID, query_video_memory_info, _I32 _STRUCT);
