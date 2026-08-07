package imgui;

import hlx.runtime.HlxPrefixControl;

// The single, idempotent frame-lifecycle hook every hl-imgui-consuming mod gets for free just by
// depending on this library - no host mod, no per-mod election, no explicit priority needed.
//
// This class itself is compiled into EVERY consuming mod's own separate HL module (mods don't
// share Haxe statics with each other - only the native imgui64.hdll and hlx-runtime's own hook
// registry are genuinely process-wide) - so `constructed` and the @:hlx.prefix registration below
// both happen once PER MOD, not once per process. That's fine: hlx-runtime's own HlxRuntime.dispatch
// already fires every registered prefix contributor for h3d.impl.DX12Driver.present once per real
// call, so drivePresent below runs once per real frame PER MOD - what makes that safe rather than
// N-times-duplicated work is entirely on the native side (ImGui.init/initWin32 are themselves
// idempotent past the first real call, and ImGui.runFrame's own g_frameOpenThisTick guard in
// imgui_native.cpp ensures only the first mod's copy to run in a given real frame does anything).
@:build(hlx.runtime.Mod.build())
class ImGuiFrame {
	static var constructed = false;

	@:hlx.prefix(h3d.impl.DX12Driver.present)
	static function drivePresent(instance:h3d.impl.DX12Driver):HlxPrefixControl {
		construct(instance);
		if (constructed) ImGui.runFrame(instance.frame.commandList);
		return Continue;
	}

	static function construct(driver:h3d.impl.DX12Driver):Void {
		if (constructed) return;
		try {
			ImGui.init(driver.frames.length);
			ImGui.initWin32(driver.window.win);
			constructed = true;
		} catch (e:Dynamic) {
			trace('ImGui construction failed, will retry next frame: $e');
		}
	}
}
