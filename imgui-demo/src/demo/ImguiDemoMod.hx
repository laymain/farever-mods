package demo;

import imgui.ImGui;

@:build(hlx.runtime.Mod.build())
class ImguiDemoMod {
	static function main():Void {
		var panel = new DemoPanel();
		ImGui.register(HlxRuntime.moduleName(), () -> DarkPastelTheme.theme.wrap(panel.draw));
	}
}
