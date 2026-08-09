package imgui;

import hlx.runtime.HlxPrefixControl;
import hlx.runtime.PatchTargetKey;

// The single, idempotent frame-lifecycle hook every hl-imgui-consuming mod gets for free just by
// depending on this library - no host mod, no per-mod election, no explicit priority needed.
//
// This class itself is compiled into EVERY consuming mod's own separate HL module (mods don't
// share Haxe statics with each other - only imgui64.hdll and hlx-runtime's own hook registry are
// genuinely process-wide). A first version of this had every mod's own copy register its own
// prefix hook on h3d.impl.DX12Driver.present (via @:hlx.prefix), relying on native-side guards to
// make the redundant copies harmless once dispatched - but the redundant *dispatch* itself (the
// shared hook registry fanning out to N contributors, and therefore N reflective Haxe calls, every
// real frame) turned out to be the actual cause of a GPU-visible rendering corruption bug that
// only reproduced with 2+ hl-imgui-consuming mods loaded together. ensureRegistered() below fixes
// this at the root: only the mod that wins claimPresentHook()'s native, process-wide claim (see
// imgui_native.cpp) ever calls HlxRuntime.registerPrefix at all - every other mod's identical call
// is now a single cheap native bool check and nothing else, so exactly one hook is ever installed
// and exactly one reflective dispatch happens per real frame, regardless of mod count.
class ImGuiFrame {
	static var constructed = false;
	static final KEY = new PatchTargetKey("h3d.impl.DX12Driver", "present");

	@:hlNative("imgui", "claim_present_hook")
	static function claimPresentHook():Bool {
		return false;
	}

	// Called from ImGui.register() (see ImGui.hx) - the one call site every consuming mod already reliably makes, so no mod needs its own explicit setup for this.
	public static function ensureRegistered():Void {
		if (claimPresentHook())
			HlxRuntime.registerPrefix(KEY, drivePresent, __receiver);
	}

	// Hand-written trampoline target
	static function __receiver(instance:h3d.impl.DX12Driver):HlxPrefixControl {
		return HlxRuntime.dispatch(KEY, [instance]);
	}

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
