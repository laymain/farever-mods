package pew.panel;

import imgui.Enums.ImGuiCol;
import imgui.Enums.ImGuiStyleVar;
import imgui.Structs.ImVec2;
import imgui.Structs.ImVec4;
import imgui.Theme;

typedef ThemeColor = {r:Float, g:Float, b:Float, a:Float}
typedef ThemeVec2 = {x:Float, y:Float}

typedef PanelThemeConfig = {
	var windowBg:ThemeColor;
	var childBg:ThemeColor;
	var popupBg:ThemeColor;
	var border:ThemeColor;

	var text:ThemeColor;
	var textDisabled:ThemeColor;

	var header:ThemeColor;
	var headerHovered:ThemeColor;
	var headerActive:ThemeColor;

	var button:ThemeColor;
	var buttonHovered:ThemeColor;
	var buttonActive:ThemeColor;

	// frameBg also doubles as the share-bar track.
	var frameBg:ThemeColor;
	var frameBgHovered:ThemeColor;
	var frameBgActive:ThemeColor;

	var plotHistogram:ThemeColor;
	var plotHistogramHovered:ThemeColor;

	// Rollup row background in CombatantDetailPanel's table - applied directly via tableSetBgColor(), not part of build()'s push/pop batch.
	var rollupRowBg:ThemeColor;

	var tab:ThemeColor;
	var tabHovered:ThemeColor;
	var tabSelected:ThemeColor;
	var tabDimmed:ThemeColor;
	var tabDimmedSelected:ThemeColor;

	var titleBg:ThemeColor;
	var titleBgActive:ThemeColor;
	var titleBgCollapsed:ThemeColor;

	var tableHeaderBg:ThemeColor;

	var scrollbarBg:ThemeColor;
	var scrollbarGrab:ThemeColor;
	var scrollbarGrabHovered:ThemeColor;
	var scrollbarGrabActive:ThemeColor;

	var checkMark:ThemeColor;

	var sliderGrab:ThemeColor;
	var sliderGrabActive:ThemeColor;

	var resizeGrip:ThemeColor;
	var resizeGripHovered:ThemeColor;
	var resizeGripActive:ThemeColor;

	var separator:ThemeColor;
	var separatorHovered:ThemeColor;
	var separatorActive:ThemeColor;

	var menuBarBg:ThemeColor;

	var dragDropTarget:ThemeColor;

	var windowRounding:Float;
	var childRounding:Float;
	var frameRounding:Float;
	var popupRounding:Float;
	var scrollbarRounding:Float;
	var grabRounding:Float;
	var tabRounding:Float;

	var windowBorderSize:Float;
	var frameBorderSize:Float;
	var popupBorderSize:Float;

	var windowPadding:ThemeVec2;
	var framePadding:ThemeVec2;
	var itemSpacing:ThemeVec2;
	var itemInnerSpacing:ThemeVec2;
	var indentSpacing:Float;
}

// Turns the mod's user-editable theme config (round-tripped by @:hlx.config) into an imgui.Theme push/pop batch.
class PanelTheme {
	public static function build(cfg:PanelThemeConfig):Theme {
		return new Theme()
			.color(ImGuiCol.WindowBg, color(cfg.windowBg))
			.color(ImGuiCol.ChildBg, color(cfg.childBg))
			.color(ImGuiCol.PopupBg, color(cfg.popupBg))
			.color(ImGuiCol.Border, color(cfg.border))

			.color(ImGuiCol.Text, color(cfg.text))
			.color(ImGuiCol.TextDisabled, color(cfg.textDisabled))

			.color(ImGuiCol.Header, color(cfg.header))
			.color(ImGuiCol.HeaderHovered, color(cfg.headerHovered))
			.color(ImGuiCol.HeaderActive, color(cfg.headerActive))

			.color(ImGuiCol.Button, color(cfg.button))
			.color(ImGuiCol.ButtonHovered, color(cfg.buttonHovered))
			.color(ImGuiCol.ButtonActive, color(cfg.buttonActive))

			.color(ImGuiCol.FrameBg, color(cfg.frameBg))
			.color(ImGuiCol.FrameBgHovered, color(cfg.frameBgHovered))
			.color(ImGuiCol.FrameBgActive, color(cfg.frameBgActive))

			.color(ImGuiCol.PlotHistogram, color(cfg.plotHistogram))
			.color(ImGuiCol.PlotHistogramHovered, color(cfg.plotHistogramHovered))

			.color(ImGuiCol.Tab, color(cfg.tab))
			.color(ImGuiCol.TabHovered, color(cfg.tabHovered))
			.color(ImGuiCol.TabSelected, color(cfg.tabSelected))
			.color(ImGuiCol.TabDimmed, color(cfg.tabDimmed))
			.color(ImGuiCol.TabDimmedSelected, color(cfg.tabDimmedSelected))

			.color(ImGuiCol.TitleBg, color(cfg.titleBg))
			.color(ImGuiCol.TitleBgActive, color(cfg.titleBgActive))
			.color(ImGuiCol.TitleBgCollapsed, color(cfg.titleBgCollapsed))

			.color(ImGuiCol.TableHeaderBg, color(cfg.tableHeaderBg))

			.color(ImGuiCol.ScrollbarBg, color(cfg.scrollbarBg))
			.color(ImGuiCol.ScrollbarGrab, color(cfg.scrollbarGrab))
			.color(ImGuiCol.ScrollbarGrabHovered, color(cfg.scrollbarGrabHovered))
			.color(ImGuiCol.ScrollbarGrabActive, color(cfg.scrollbarGrabActive))

			.color(ImGuiCol.CheckMark, color(cfg.checkMark))

			.color(ImGuiCol.SliderGrab, color(cfg.sliderGrab))
			.color(ImGuiCol.SliderGrabActive, color(cfg.sliderGrabActive))

			.color(ImGuiCol.ResizeGrip, color(cfg.resizeGrip))
			.color(ImGuiCol.ResizeGripHovered, color(cfg.resizeGripHovered))
			.color(ImGuiCol.ResizeGripActive, color(cfg.resizeGripActive))

			.color(ImGuiCol.Separator, color(cfg.separator))
			.color(ImGuiCol.SeparatorHovered, color(cfg.separatorHovered))
			.color(ImGuiCol.SeparatorActive, color(cfg.separatorActive))

			.color(ImGuiCol.MenuBarBg, color(cfg.menuBarBg))

			.color(ImGuiCol.DragDropTarget, color(cfg.dragDropTarget))

			.varF(ImGuiStyleVar.WindowRounding, cfg.windowRounding)
			.varF(ImGuiStyleVar.ChildRounding, cfg.childRounding)
			.varF(ImGuiStyleVar.FrameRounding, cfg.frameRounding)
			.varF(ImGuiStyleVar.PopupRounding, cfg.popupRounding)
			.varF(ImGuiStyleVar.ScrollbarRounding, cfg.scrollbarRounding)
			.varF(ImGuiStyleVar.GrabRounding, cfg.grabRounding)
			.varF(ImGuiStyleVar.TabRounding, cfg.tabRounding)

			.varF(ImGuiStyleVar.WindowBorderSize, cfg.windowBorderSize)
			.varF(ImGuiStyleVar.FrameBorderSize, cfg.frameBorderSize)
			.varF(ImGuiStyleVar.PopupBorderSize, cfg.popupBorderSize)

			.varV(ImGuiStyleVar.WindowPadding, vec2(cfg.windowPadding))
			.varV(ImGuiStyleVar.FramePadding, vec2(cfg.framePadding))
			.varV(ImGuiStyleVar.ItemSpacing, vec2(cfg.itemSpacing))
			.varV(ImGuiStyleVar.ItemInnerSpacing, vec2(cfg.itemInnerSpacing))
			.varF(ImGuiStyleVar.IndentSpacing, cfg.indentSpacing);
	}

	public static function defaultConfig():PanelThemeConfig return {
		windowBg: {r: 0.12, g: 0.13, b: 0.15, a: 0.35},
		childBg: {r: 0.14, g: 0.15, b: 0.17, a: 0.35},
		popupBg: {r: 0.10, g: 0.10, b: 0.12, a: 0.35},
		border: {r: 0.30, g: 0.33, b: 0.42, a: 0.25},

		text: {r: 0.90, g: 0.93, b: 0.95, a: 1.00},
		textDisabled: {r: 0.60, g: 0.65, b: 0.70, a: 1.00},

		header: {r: 0.36, g: 0.42, b: 0.55, a: 0.35},
		headerHovered: {r: 0.44, g: 0.50, b: 0.68, a: 0.35},
		headerActive: {r: 0.46, g: 0.55, b: 0.75, a: 0.35},

		button: {r: 0.28, g: 0.34, b: 0.48, a: 0.70},
		buttonHovered: {r: 0.36, g: 0.45, b: 0.65, a: 0.85},
		buttonActive: {r: 0.40, g: 0.50, b: 0.70, a: 1.00},

		frameBg: {r: 0.20, g: 0.22, b: 0.28, a: 0.35},
		frameBgHovered: {r: 0.28, g: 0.32, b: 0.42, a: 1.00},
		frameBgActive: {r: 0.32, g: 0.38, b: 0.50, a: 1.00},

		plotHistogram: {r: 0.46, g: 0.55, b: 0.75, a: 0.35},
		plotHistogramHovered: {r: 0.55, g: 0.64, b: 0.85, a: 0.35},
		rollupRowBg: {r: 0.55, g: 0.55, b: 0.58, a: 0.65},

		tab: {r: 0.26, g: 0.30, b: 0.42, a: 0.80},
		tabHovered: {r: 0.36, g: 0.42, b: 0.58, a: 1.00},
		tabSelected: {r: 0.42, g: 0.50, b: 0.68, a: 1.00},
		tabDimmed: {r: 0.20, g: 0.24, b: 0.32, a: 0.80},
		tabDimmedSelected: {r: 0.30, g: 0.36, b: 0.50, a: 1.00},

		titleBg: {r: 0.20, g: 0.25, b: 0.30, a: 0.35},
		titleBgActive: {r: 0.25, g: 0.30, b: 0.40, a: 0.35},
		titleBgCollapsed: {r: 0.10, g: 0.12, b: 0.15, a: 0.35},

		tableHeaderBg: {r: 0.20, g: 0.25, b: 0.30, a: 0.35},

		scrollbarBg: {r: 0.13, g: 0.14, b: 0.18, a: 1.00},
		scrollbarGrab: {r: 0.25, g: 0.30, b: 0.38, a: 0.60},
		scrollbarGrabHovered: {r: 0.35, g: 0.40, b: 0.50, a: 0.80},
		scrollbarGrabActive: {r: 0.45, g: 0.50, b: 0.65, a: 1.00},

		checkMark: {r: 0.80, g: 0.85, b: 1.00, a: 1.00},

		sliderGrab: {r: 0.50, g: 0.65, b: 0.90, a: 1.00},
		sliderGrabActive: {r: 0.60, g: 0.75, b: 1.00, a: 1.00},

		resizeGrip: {r: 0.30, g: 0.40, b: 0.50, a: 0.60},
		resizeGripHovered: {r: 0.40, g: 0.50, b: 0.60, a: 0.80},
		resizeGripActive: {r: 0.50, g: 0.60, b: 0.80, a: 1.00},

		separator: {r: 0.35, g: 0.40, b: 0.48, a: 0.7},
		separatorHovered: {r: 0.50, g: 0.60, b: 0.72, a: 0.9},
		separatorActive: {r: 0.65, g: 0.70, b: 0.85, a: 1.0},

		menuBarBg: {r: 0.14, g: 0.15, b: 0.17, a: 1.00},

		dragDropTarget: {r: 0.50, g: 0.85, b: 1.00, a: 0.90},

		windowRounding: 8.0,
		childRounding: 6.0,
		frameRounding: 5.0,
		popupRounding: 6.0,
		scrollbarRounding: 5.0,
		grabRounding: 4.0,
		tabRounding: 5.0,

		windowBorderSize: 0.0,
		frameBorderSize: 0.0,
		popupBorderSize: 1.0,

		windowPadding: {x: 16, y: 16},
		framePadding: {x: 10, y: 6},
		itemSpacing: {x: 10, y: 10},
		itemInnerSpacing: {x: 6, y: 4},
		indentSpacing: 20.0,
	};

	// Public - also used by PewPewMeterMod to convert rollupRowBg for CombatantDetailPanel.
	public static function color(c:ThemeColor):ImVec4 return new ImVec4(c.r, c.g, c.b, c.a);
	static function vec2(v:ThemeVec2):ImVec2 return new ImVec2(v.x, v.y);
}
