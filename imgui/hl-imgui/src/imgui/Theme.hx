package imgui;

import imgui.Structs.ImVec2;
import imgui.Structs.ImVec4;

// Batches a set of PushStyleColor/PushStyleVar calls with their matching Pop*(count) calls, so a
// mod can't forget a pop (or pop the wrong count) after an early return or an exception mid-draw -
// the classic ImGui style-stack mistake. Build once, then wrap every draw call with it:
//
//   var theme = new Theme()
//       .color(ImGuiCol.Text, someColor)
//       .varF(ImGuiStyleVar.Alpha, 0.5);
//   theme.wrap(() -> panel.draw());
class Theme {
	var colorPushes:Array<Void->Void> = [];
	var varPushes:Array<Void->Void> = [];

	public function new() {}

	public function color(idx:Int, value:ImVec4):Theme {
		colorPushes.push(() -> ImGui.pushStyleColor(idx, value));
		return this;
	}

	public function varF(idx:Int, value:Single):Theme {
		varPushes.push(() -> ImGui.pushStyleVar(idx, value));
		return this;
	}

	public function varV(idx:Int, value:ImVec2):Theme {
		varPushes.push(() -> ImGui.pushStyleVar(idx, value));
		return this;
	}

	public function wrap(draw:Void->Void):Void {
		for (p in varPushes) p();
		for (p in colorPushes) p();
		try {
			draw();
		} catch (e:Dynamic) {
			popAll();
			throw e;
		}
		popAll();
	}

	function popAll():Void {
		if (colorPushes.length > 0) ImGui.popStyleColor(colorPushes.length);
		if (varPushes.length > 0) ImGui.popStyleVar(varPushes.length);
	}
}
