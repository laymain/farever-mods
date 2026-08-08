package demo;

import imgui.Enums.ImGuiCol;
import imgui.Enums.ImGuiStyleVar;
import imgui.Structs.ImVec2;
import imgui.Structs.ImVec4;
import imgui.Theme;

class DarkPastelTheme {

	public static var theme(default, null):Theme = build();

	static function v4(x:Single, y:Single, z:Single, w:Single):ImVec4 {
		var v = new ImVec4();
		v.x = x;
		v.y = y;
		v.z = z;
		v.w = w;
		return v;
	}

	static function v2(x:Single, y:Single):ImVec2 {
		var v = new ImVec2();
		v.x = x;
		v.y = y;
		return v;
	}

	static function build():Theme {
		return new Theme()
			// Backgrounds
			.color(ImGuiCol.WindowBg, v4(0.12, 0.13, 0.15, 0.95))
			.color(ImGuiCol.ChildBg, v4(0.14, 0.15, 0.17, 0.95))
			.color(ImGuiCol.PopupBg, v4(0.10, 0.10, 0.12, 0.90))
			.color(ImGuiCol.Border, v4(0.30, 0.33, 0.42, 0.40))

			// Text
			.color(ImGuiCol.Text, v4(0.90, 0.93, 0.95, 1.00))
			.color(ImGuiCol.TextDisabled, v4(0.60, 0.65, 0.70, 1.00))

			// Headers
			.color(ImGuiCol.Header, v4(0.36, 0.42, 0.55, 0.60))
			.color(ImGuiCol.HeaderHovered, v4(0.44, 0.50, 0.68, 0.80))
			.color(ImGuiCol.HeaderActive, v4(0.46, 0.55, 0.75, 1.00))

			// Buttons
			.color(ImGuiCol.Button, v4(0.28, 0.34, 0.48, 0.70))
			.color(ImGuiCol.ButtonHovered, v4(0.36, 0.45, 0.65, 0.85))
			.color(ImGuiCol.ButtonActive, v4(0.40, 0.50, 0.70, 1.00))

			// Frames
			.color(ImGuiCol.FrameBg, v4(0.20, 0.22, 0.28, 1.00))
			.color(ImGuiCol.FrameBgHovered, v4(0.28, 0.32, 0.42, 1.00))
			.color(ImGuiCol.FrameBgActive, v4(0.32, 0.38, 0.50, 1.00))

			// Tabs
			.color(ImGuiCol.Tab, v4(0.26, 0.30, 0.42, 0.80))
			.color(ImGuiCol.TabHovered, v4(0.36, 0.42, 0.58, 1.00))
			.color(ImGuiCol.TabSelected, v4(0.42, 0.50, 0.68, 1.00))
			.color(ImGuiCol.TabDimmed, v4(0.20, 0.24, 0.32, 0.80))
			.color(ImGuiCol.TabDimmedSelected, v4(0.30, 0.36, 0.50, 1.00))

			// Titles
			.color(ImGuiCol.TitleBg, v4(0.20, 0.25, 0.30, 1.00))
			.color(ImGuiCol.TitleBgActive, v4(0.25, 0.30, 0.40, 1.00))
			.color(ImGuiCol.TitleBgCollapsed, v4(0.10, 0.12, 0.15, 0.75))

			// Scrollbars
			.color(ImGuiCol.ScrollbarBg, v4(0.13, 0.14, 0.18, 1.00))
			.color(ImGuiCol.ScrollbarGrab, v4(0.25, 0.30, 0.38, 0.60))
			.color(ImGuiCol.ScrollbarGrabHovered, v4(0.35, 0.40, 0.50, 0.80))
			.color(ImGuiCol.ScrollbarGrabActive, v4(0.45, 0.50, 0.65, 1.00))

			// Checkboxes / Radios
			.color(ImGuiCol.CheckMark, v4(0.80, 0.85, 1.00, 1.00))

			// Sliders
			.color(ImGuiCol.SliderGrab, v4(0.50, 0.65, 0.90, 1.00))
			.color(ImGuiCol.SliderGrabActive, v4(0.60, 0.75, 1.00, 1.00))

			// Resize Grip
			.color(ImGuiCol.ResizeGrip, v4(0.30, 0.40, 0.50, 0.60))
			.color(ImGuiCol.ResizeGripHovered, v4(0.40, 0.50, 0.60, 0.80))
			.color(ImGuiCol.ResizeGripActive, v4(0.50, 0.60, 0.80, 1.00))

			// Separator
			.color(ImGuiCol.Separator, v4(0.35, 0.40, 0.48, 0.7))
			.color(ImGuiCol.SeparatorHovered, v4(0.50, 0.60, 0.72, 0.9))
			.color(ImGuiCol.SeparatorActive, v4(0.65, 0.70, 0.85, 1.0))

			// Menus and Tooltips
			.color(ImGuiCol.MenuBarBg, v4(0.14, 0.15, 0.17, 1.00))

			// Drag & Drop
			.color(ImGuiCol.DragDropTarget, v4(0.50, 0.85, 1.00, 0.90))

			// Style Metrics
			.varF(ImGuiStyleVar.WindowRounding, 8.0)
			.varF(ImGuiStyleVar.ChildRounding, 6.0)
			.varF(ImGuiStyleVar.FrameRounding, 5.0)
			.varF(ImGuiStyleVar.PopupRounding, 6.0)
			.varF(ImGuiStyleVar.ScrollbarRounding, 5.0)
			.varF(ImGuiStyleVar.GrabRounding, 4.0)
			.varF(ImGuiStyleVar.TabRounding, 5.0)

			.varF(ImGuiStyleVar.WindowBorderSize, 0.0)
			.varF(ImGuiStyleVar.FrameBorderSize, 0.0)
			.varF(ImGuiStyleVar.PopupBorderSize, 1.0)

			.varV(ImGuiStyleVar.WindowPadding, v2(16, 16))
			.varV(ImGuiStyleVar.FramePadding, v2(10, 6))
			.varV(ImGuiStyleVar.ItemSpacing, v2(10, 10))
			.varV(ImGuiStyleVar.ItemInnerSpacing, v2(6, 4))
			.varF(ImGuiStyleVar.IndentSpacing, 20.0);
	}
}
