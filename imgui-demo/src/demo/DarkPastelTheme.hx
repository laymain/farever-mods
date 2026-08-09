package demo;

import imgui.Enums.ImGuiCol;
import imgui.Enums.ImGuiStyleVar;
import imgui.Structs.ImVec2;
import imgui.Structs.ImVec4;
import imgui.Theme;

class DarkPastelTheme {

	public static var theme(default, null):Theme = build();

	static function build():Theme {
		return new Theme()
			// Backgrounds
			.color(ImGuiCol.WindowBg, new ImVec4(0.12, 0.13, 0.15, 0.95))
			.color(ImGuiCol.ChildBg, new ImVec4(0.14, 0.15, 0.17, 0.95))
			.color(ImGuiCol.PopupBg, new ImVec4(0.10, 0.10, 0.12, 0.90))
			.color(ImGuiCol.Border, new ImVec4(0.30, 0.33, 0.42, 0.40))

			// Text
			.color(ImGuiCol.Text, new ImVec4(0.90, 0.93, 0.95, 1.00))
			.color(ImGuiCol.TextDisabled, new ImVec4(0.60, 0.65, 0.70, 1.00))

			// Headers
			.color(ImGuiCol.Header, new ImVec4(0.36, 0.42, 0.55, 0.60))
			.color(ImGuiCol.HeaderHovered, new ImVec4(0.44, 0.50, 0.68, 0.80))
			.color(ImGuiCol.HeaderActive, new ImVec4(0.46, 0.55, 0.75, 1.00))

			// Buttons
			.color(ImGuiCol.Button, new ImVec4(0.28, 0.34, 0.48, 0.70))
			.color(ImGuiCol.ButtonHovered, new ImVec4(0.36, 0.45, 0.65, 0.85))
			.color(ImGuiCol.ButtonActive, new ImVec4(0.40, 0.50, 0.70, 1.00))

			// Frames
			.color(ImGuiCol.FrameBg, new ImVec4(0.20, 0.22, 0.28, 1.00))
			.color(ImGuiCol.FrameBgHovered, new ImVec4(0.28, 0.32, 0.42, 1.00))
			.color(ImGuiCol.FrameBgActive, new ImVec4(0.32, 0.38, 0.50, 1.00))

			// Tabs
			.color(ImGuiCol.Tab, new ImVec4(0.26, 0.30, 0.42, 0.80))
			.color(ImGuiCol.TabHovered, new ImVec4(0.36, 0.42, 0.58, 1.00))
			.color(ImGuiCol.TabSelected, new ImVec4(0.42, 0.50, 0.68, 1.00))
			.color(ImGuiCol.TabDimmed, new ImVec4(0.20, 0.24, 0.32, 0.80))
			.color(ImGuiCol.TabDimmedSelected, new ImVec4(0.30, 0.36, 0.50, 1.00))

			// Titles
			.color(ImGuiCol.TitleBg, new ImVec4(0.20, 0.25, 0.30, 1.00))
			.color(ImGuiCol.TitleBgActive, new ImVec4(0.25, 0.30, 0.40, 1.00))
			.color(ImGuiCol.TitleBgCollapsed, new ImVec4(0.10, 0.12, 0.15, 0.75))

			// Scrollbars
			.color(ImGuiCol.ScrollbarBg, new ImVec4(0.13, 0.14, 0.18, 1.00))
			.color(ImGuiCol.ScrollbarGrab, new ImVec4(0.25, 0.30, 0.38, 0.60))
			.color(ImGuiCol.ScrollbarGrabHovered, new ImVec4(0.35, 0.40, 0.50, 0.80))
			.color(ImGuiCol.ScrollbarGrabActive, new ImVec4(0.45, 0.50, 0.65, 1.00))

			// Checkboxes / Radios
			.color(ImGuiCol.CheckMark, new ImVec4(0.80, 0.85, 1.00, 1.00))

			// Sliders
			.color(ImGuiCol.SliderGrab, new ImVec4(0.50, 0.65, 0.90, 1.00))
			.color(ImGuiCol.SliderGrabActive, new ImVec4(0.60, 0.75, 1.00, 1.00))

			// Resize Grip
			.color(ImGuiCol.ResizeGrip, new ImVec4(0.30, 0.40, 0.50, 0.60))
			.color(ImGuiCol.ResizeGripHovered, new ImVec4(0.40, 0.50, 0.60, 0.80))
			.color(ImGuiCol.ResizeGripActive, new ImVec4(0.50, 0.60, 0.80, 1.00))

			// Separator
			.color(ImGuiCol.Separator, new ImVec4(0.35, 0.40, 0.48, 0.7))
			.color(ImGuiCol.SeparatorHovered, new ImVec4(0.50, 0.60, 0.72, 0.9))
			.color(ImGuiCol.SeparatorActive, new ImVec4(0.65, 0.70, 0.85, 1.0))

			// Menus and Tooltips
			.color(ImGuiCol.MenuBarBg, new ImVec4(0.14, 0.15, 0.17, 1.00))

			// Drag & Drop
			.color(ImGuiCol.DragDropTarget, new ImVec4(0.50, 0.85, 1.00, 0.90))

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

			.varV(ImGuiStyleVar.WindowPadding, new ImVec2(16, 16))
			.varV(ImGuiStyleVar.FramePadding, new ImVec2(10, 6))
			.varV(ImGuiStyleVar.ItemSpacing, new ImVec2(10, 10))
			.varV(ImGuiStyleVar.ItemInnerSpacing, new ImVec2(6, 4))
			.varF(ImGuiStyleVar.IndentSpacing, 20.0);
	}
}
