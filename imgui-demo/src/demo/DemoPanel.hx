package demo;

import imgui.ImGui;
import imgui.Theme;
import imgui.Enums.ImGuiTreeNodeFlags;
import imgui.Enums.ImGuiTableFlags;
import imgui.Enums.ImGuiStyleVar;
import imgui.Enums.ImGuiColorEditFlags;
import imgui.Enums.ImGuiCol;
import imgui.Enums.ImGuiChildFlags;
import imgui.ref.BoolRef;
import imgui.ref.DoubleRef;
import imgui.ref.FloatRef;
import imgui.ref.IntRef;

class DemoPanel {
	static inline var PLOT_SAMPLES = 32;
	static inline var NAME_BUF_SIZE = 64;
	static inline var MULTILINE_BUF_SIZE = 256;

	var clickCount = 0;
	var opened = new BoolRef(true);
	var checked = new BoolRef();
	var speed = new FloatRef(1);
	var count = new IntRef();
	var radioChoice = new IntRef();

	var comboItems = ["Alpha", "Bravo", "Charlie"];
	var comboIndex = new IntRef();
	var listItems = ["Sword", "Shield", "Bow", "Staff"];
	var listIndex = new IntRef();

	var dragFloatVal = new FloatRef();
	var dragIntVal = new IntRef();

	// sliderFloat3/colorEdit3/colorEdit4 take a raw multi-float hl.Bytes array pointer, not a
	// single hl.Ref<T> scalar - so unlike FloatRef/IntRef these need no wrapper type at all, just
	// ImGui.v3/v4 (imgui.ImGui's own helpers, same idea as vec2()) to build the buffer.
	var position = ImGui.v3(1, 2, 3);
	var color3 = ImGui.v3(0.2, 0.6, 0.9);
	var color4 = ImGui.v4(0.9, 0.3, 0.3, 1);
	var nameBuf = new hl.Bytes(NAME_BUF_SIZE);
	var hintBuf = new hl.Bytes(NAME_BUF_SIZE);
	var multilineBuf = new hl.Bytes(MULTILINE_BUF_SIZE);

	var inputIntVal = new IntRef();
	var inputFloatVal = new FloatRef();
	var inputDoubleVal = new DoubleRef();
	var dragVec3Val = ImGui.v3(0, 0, 0);
	var angleVal = new FloatRef();
	var vSliderVal = new FloatRef(50);

	var multiSelectItems = ["Fire", "Ice", "Poison", "Lightning"];
	var multiSelectChecked = [false, false, false, false];
	var pinnedSelected = new BoolRef();
	var menuChecked = new BoolRef();

	var swatchColor = ImGui.vec4(0.8, 0.3, 0.1, 1);
	var pickerColor = ImGui.v3(0.3, 0.7, 0.5);

	var waveValues = new hl.Bytes(PLOT_SAMPLES * 4);
	var histValues = new hl.Bytes(PLOT_SAMPLES * 4);

	var tableRows = [
		{name: "Aria", level: 12, klass: "Warrior"},
		{name: "Nyx", level: 7, klass: "Rogue"},
		{name: "Peregrine", level: 20, klass: "Mage"}
	];

	public function new() {
		nameBuf.setUI8(0, 0);
		hintBuf.setUI8(0, 0);
		multilineBuf.setUI8(0, 0);
		for (i in 0...PLOT_SAMPLES) {
			waveValues.setF32(i * 4, Math.sin(i / PLOT_SAMPLES * Math.PI * 2));
			histValues.setF32(i * 4, Math.abs(Math.sin(i / PLOT_SAMPLES * Math.PI * 4)));
		}
	}

	public function draw():Void {
		if (!opened.get())
			return;
		if (ImGui.begin("hl-imgui demo", opened)) {
			if (ImGui.beginTabBar("DemoTabs")) {
				if (ImGui.beginTabItem("Basics")) {
					drawBasics();
					ImGui.endTabItem();
				}
				if (ImGui.beginTabItem("Widgets")) {
					drawWidgets();
					ImGui.endTabItem();
				}
				if (ImGui.beginTabItem("Inputs")) {
					drawInputs();
					ImGui.endTabItem();
				}
				if (ImGui.beginTabItem("Selection")) {
					drawSelection();
					ImGui.endTabItem();
				}
				if (ImGui.beginTabItem("Style")) {
					drawStyle();
					ImGui.endTabItem();
				}
				if (ImGui.beginTabItem("Layout")) {
					drawLayout();
					ImGui.endTabItem();
				}
				if (ImGui.beginTabItem("Table")) {
					drawTable();
					ImGui.endTabItem();
				}
				if (ImGui.beginTabItem("Plots")) {
					drawPlots();
					ImGui.endTabItem();
				}
				if (ImGui.beginTabItem("Popups")) {
					drawPopups();
					ImGui.endTabItem();
				}
				if (ImGui.beginTabItem("Canvas")) {
					drawCanvas();
					ImGui.endTabItem();
				}
				ImGui.endTabBar();
			}
		}
		ImGui.end();
	}

	function drawBasics():Void {
		ImGui.text("Hello from hl-imgui!");
		if (ImGui.button("Click me"))
			clickCount++;
		ImGui.sameLine();
		if (ImGui.smallButton("Reset"))
			clickCount = 0;
		ImGui.text('Clicked $clickCount times');

		ImGui.separatorText("Toggles");
		ImGui.checkbox("A checkbox", checked);

		ImGui.separatorText("Radio buttons");
		ImGui.radioButton("One", radioChoice, 0);
		ImGui.sameLine();
		ImGui.radioButton("Two", radioChoice, 1);
		ImGui.sameLine();
		ImGui.radioButton("Three", radioChoice, 2);

		ImGui.separatorText("Sliders");
		ImGui.sliderFloat("Speed", speed, 0, 10);
		ImGui.sliderInt("Count", count, 0, 100);
	}

	function drawWidgets():Void {
		ImGui.separatorText("Combo / list box");
		if (ImGui.beginCombo("Combo", comboItems[comboIndex.get()])) {
			for (i in 0...comboItems.length) {
				var isSelected = i == comboIndex.get();
				if (ImGui.selectable(comboItems[i], isSelected))
					comboIndex.set(i);
				if (isSelected)
					ImGui.setItemDefaultFocus();
			}
			ImGui.endCombo();
		}
		if (ImGui.beginListBox("List", ImGui.vec2(0, 80))) {
			for (i in 0...listItems.length) {
				if (ImGui.selectable(listItems[i], i == listIndex.get()))
					listIndex.set(i);
			}
			ImGui.endListBox();
		}

		ImGui.separatorText("Drag");
		ImGui.dragFloat("Drag float", dragFloatVal, 0.1, 0, 100, "%.1f");
		ImGui.dragInt("Drag int", dragIntVal, 1, 0, 100);

		ImGui.separatorText("Multi-component");
		ImGui.sliderFloat3("Position", position, -10, 10, "%.2f");

		ImGui.separatorText("Color");
		ImGui.colorEdit3("Color3", color3);
		ImGui.colorEdit4("Color4", color4, ImGuiColorEditFlags.AlphaBar);

		ImGui.separatorText("Text input");
		ImGui.inputText("Name", nameBuf, NAME_BUF_SIZE);
	}

	function drawInputs():Void {
		ImGui.separatorText("Text");
		ImGui.inputTextWithHint("Hinted", "Type something...", hintBuf, NAME_BUF_SIZE);
		ImGui.inputTextMultiline("Multiline", multilineBuf, MULTILINE_BUF_SIZE, ImGui.vec2(0, 100));

		ImGui.separatorText("Numeric input");
		ImGui.inputInt("Input int", inputIntVal);
		ImGui.inputFloat("Input float", inputFloatVal);
		ImGui.inputDouble("Input double", inputDoubleVal);

		ImGui.separatorText("Drag / slider");
		ImGui.dragFloat3("Drag vec3", dragVec3Val, 0.1, 0, 100, "%.2f");
		ImGui.sliderAngle("Angle", angleVal);
		ImGui.sameLine();
		ImGui.vSliderFloat("V-slider", ImGui.vec2(20, 100), vSliderVal, 0, 100);
	}

	function drawSelection():Void {
		ImGui.separatorText("Multi-select");
		for (i in 0...multiSelectItems.length) {
			var isSelected = multiSelectChecked[i];
			if (ImGui.selectable(multiSelectItems[i], isSelected))
				multiSelectChecked[i] = !isSelected;
		}

		ImGui.separatorText("Persistent selectable");
		ImGui.selectable("Pinned row", pinnedSelected);

		ImGui.separatorText("Tree");
		if (ImGui.treeNodeEx("Flagged node", ImGuiTreeNodeFlags.Bullet)) {
			ImGui.text("Rendered with the Bullet flag");
			ImGui.treePop();
		}

		ImGui.separatorText("Menu item");
		ImGui.menuItem("Enable feature", null, menuChecked);
	}

	function drawStyle():Void {
		ImGui.separatorText("Color swatch");
		ImGui.colorButton("Swatch", swatchColor);

		ImGui.separatorText("Color picker");
		ImGui.colorPicker3("Picker", pickerColor);

		ImGui.separatorText("Theme");
		var theme = new Theme()
			.color(ImGuiCol.Button, ImGui.vec4(0.8, 0.3, 0.1, 1))
			.varF(ImGuiStyleVar.FrameRounding, 12);
		theme.wrap(() -> ImGui.button("Themed button"));
	}

	function drawLayout():Void {
		if (ImGui.collapsingHeader("Details", ImGuiTreeNodeFlags.DefaultOpen)) {
			ImGui.indent();
			ImGui.text("Nested content inside a collapsing header.");
			ImGui.unindent();
		}

		if (ImGui.treeNode("Tree node")) {
			ImGui.bullet();
			ImGui.text("A bullet point");
			if (ImGui.treeNode("Nested node")) {
				ImGui.text("Deeper nesting");
				ImGui.treePop();
			}
			ImGui.treePop();
		}

		ImGui.separatorText("Columns");
		ImGui.columns(2, "DemoColumns");
		ImGui.text("Column A");
		ImGui.nextColumn();
		ImGui.text("Column B");
		ImGui.columns(1, null, false);
	}

	function drawTable():Void {
		if (ImGui.beginTable("DemoTable", 3, ImGuiTableFlags.Borders | ImGuiTableFlags.RowBg)) {
			ImGui.tableSetupColumn("Name");
			ImGui.tableSetupColumn("Level");
			ImGui.tableSetupColumn("Class");
			ImGui.tableHeadersRow();
			for (row in tableRows) {
				ImGui.tableNextRow(0, 0);
				ImGui.tableNextColumn();
				ImGui.text(row.name);
				ImGui.tableNextColumn();
				ImGui.text(Std.string(row.level));
				ImGui.tableNextColumn();
				ImGui.text(row.klass);
			}
			ImGui.endTable();
		}
	}

	function drawPlots():Void {
		ImGui.separatorText("Progress");
		ImGui.progressBar((ImGui.getTime() % 2.0 / 2.0 : Single), ImGui.vec2(0, 0), null);

		ImGui.separatorText("Plots");
		ImGui.plotLines("Wave", waveValues, PLOT_SAMPLES, -1.2, 1.2, ImGui.vec2(0, 80));
		ImGui.plotHistogram("Histogram", histValues, PLOT_SAMPLES, 0, 1.2, ImGui.vec2(0, 80));
	}

	function drawPopups():Void {
		if (ImGui.button("Open popup"))
			ImGui.openPopup("DemoPopup");
		if (ImGui.beginPopup("DemoPopup")) {
			ImGui.text("A popup!");
			if (ImGui.menuItem("Close"))
				ImGui.closeCurrentPopup();
			ImGui.endPopup();
		}

		if (ImGui.button("Open modal"))
			ImGui.openPopup("DemoModal");
		if (ImGui.beginPopupModal("DemoModal")) {
			ImGui.text("A modal popup.");
			if (ImGui.button("OK"))
				ImGui.closeCurrentPopup();
			ImGui.endPopup();
		}

		ImGui.text("(hover me)");
		if (ImGui.isItemHovered() && ImGui.beginTooltip()) {
			ImGui.text("A tooltip!");
			ImGui.endTooltip();
		}
	}

	function drawCanvas():Void {
		ImGui.separatorText("Draw list");
		if (ImGui.beginChild("CanvasRegion", ImGui.vec2(200, 150), ImGuiChildFlags.Borders)) {
			var drawList = ImGui.getWindowDrawList();
			var origin = ImGui.getCursorScreenPos();
			var wobble:Single = (Math.cos(ImGui.getTime()) * 60 : Single);
			ImGui.ImDrawList_AddRectFilled(drawList, ImGui.vec2(origin.x, origin.y), ImGui.vec2(origin.x + 180, origin.y + 40), 0xFF3355CC);
			ImGui.ImDrawList_AddLine(drawList, ImGui.vec2(origin.x, origin.y + 60), ImGui.vec2(origin.x + 180, origin.y + 60), 0xFF44CC88, 3);
			ImGui.ImDrawList_AddCircleFilled(drawList, ImGui.vec2(origin.x + 90 + wobble, origin.y + 110), 14, 0xFFCC4433);
		}
		ImGui.endChild();
	}
}
