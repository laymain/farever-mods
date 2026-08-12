package pew.panel;

import imgui.ImGui;
import imgui.Enums.ImGuiCond;
import imgui.Enums.ImGuiTableColumnFlags;
import imgui.Enums.ImGuiTableFlags;
import imgui.Enums.ImGuiWindowFlags;
import imgui.ref.BoolRef;
import pew.panel.MeterPanel.MeterRowData;

typedef DetailPanelState = {
	var x:Float;
	var y:Float;
	var opened:Bool;
	var collapsed:Bool;
}

// Singleton detail window; re-resolves its row live via findRowByName() every frame instead of caching a MeterRowData, since MeterPanel.rows is rebuilt from scratch on every refresh().
class CombatantDetailPanel {
	public var onStateChanged:DetailPanelState->Void;

	var meterPanel:MeterPanel;
	var state:DetailPanelState;
	var open:BoolRef;
	var selectedName:String;

	public function new(meterPanel:MeterPanel, state:DetailPanelState) {
		this.meterPanel = meterPanel;
		this.state = state;
		open = new BoolRef(state.opened);
	}

	public function show(name:String):Void {
		selectedName = name;
		open.set(true);
	}

	public function draw():Void {
		// getWindowPos()/isWindowCollapsed() in reportState() are only valid between Begin()/End().
		if (!open.get()) return;

		var game = GameApp.get();
		if (game == null) return;

		// Default to the local player once they have an active row; never overrides an explicit selection.
		if (selectedName == null && game.me != null && game.me.hero != null) {
			var myName = game.me.hero.name;
			if (meterPanel.findRowByName(myName) != null) selectedName = myName;
		}

		ImGui.setNextWindowPos(ImGui.vec2(state.x, state.y), ImGuiCond.FirstUseEver);
		ImGui.setNextWindowSize(ImGui.vec2(240, 160), ImGuiCond.FirstUseEver);
		ImGui.setNextWindowCollapsed(state.collapsed, ImGuiCond.FirstUseEver);

		var flags = game.isCursorFree() ? 0 : ImGuiWindowFlags.NoInputs;
		if (ImGui.begin("Combatant Details", open, flags)) {
			var row = selectedName == null ? null : meterPanel.findRowByName(selectedName);
			if (row == null) {
				ImGui.text('${selectedName == null ? "Combatant" : selectedName} is no longer active.');
			} else {
				drawContent(row);
			}
		}
		reportState();
		ImGui.end();
	}

	function drawContent(row:MeterRowData):Void {
		if (ImGui.beginTable("CombatantDetailRows", 6, ImGuiTableFlags.SizingStretchProp | ImGuiTableFlags.RowBg | ImGuiTableFlags.NoSavedSettings)) {
			ImGui.tableSetupColumn(" C", ImGuiTableColumnFlags.WidthFixed, 20);
			ImGui.tableSetupColumn("Name");
			ImGui.tableSetupColumn("Dmg");
			ImGui.tableSetupColumn("DPS");
			ImGui.tableSetupColumn("Hits/Crits");
			ImGui.tableSetupColumn("%", ImGuiTableColumnFlags.WidthStretch, 1.75);
			ImGui.tableHeadersRow();

			ImGui.tableNextRow();
			MeterPanel.drawRow(row, true, false);

			for (skillRow in meterPanel.findSkillRows(row.name)) {
				ImGui.tableNextRow();
				MeterPanel.drawRow(skillRow, false);
			}

			ImGui.endTable();
		}
	}

	function reportState():Void {
		if (onStateChanged == null) return;

		var pos = ImGui.getWindowPos();
		var next:DetailPanelState = {
			x: pos.x,
			y: pos.y,
			opened: open.get(),
			collapsed: ImGui.isWindowCollapsed(),
		};
		if (next.x == state.x && next.y == state.y && next.opened == state.opened && next.collapsed == state.collapsed) return;

		state = next;
		onStateChanged(next);
	}
}
