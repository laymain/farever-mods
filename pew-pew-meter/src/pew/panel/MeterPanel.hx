package pew.panel;

import imgui.ImGui;
import imgui.Enums.ImGuiCond;
import imgui.Enums.ImGuiPopupFlags;
import imgui.Enums.ImGuiSortDirection;
import imgui.Enums.ImGuiStyleVar;
import imgui.Enums.ImGuiTableFlags;
import imgui.Enums.ImGuiTableColumnFlags;
import imgui.Enums.ImGuiTableRowFlags;
import imgui.Enums.ImGuiWindowFlags;
import pew.panel.IconCache.Icon;
import pew.tracking.Encounter;
import pew.roster.Combatant;

typedef MeterPanelState = {
	var x:Float;
	var y:Float;
	var width:Int;
	var height:Int;
	var collapsed:Bool;
}

typedef MeterRowData = {
	var name:String;
	var className:String;
	var icon:Icon;
	var total:Float;
	var dps:Float;
	var pct:Float;
	var hits:Int;
	var crits:Int;
	var share:Float;
}

// Renders the live DPS table as an ImGui window. onStateChanged only fires when position/size/open actually differ from last frame, since imgui reports all three unconditionally.
class MeterPanel {
	public var onStateChanged:MeterPanelState->Void;
	public var onOpenDetail:String->Void;

	var state:MeterPanelState;
	// rows/skillRowsByName are rebuilt from scratch every refresh() - never cache a MeterRowData across frames.
	var rows:Array<MeterRowData> = [];
	var skillRowsByName = new Map<String, Array<MeterRowData>>();

	public function new(state:MeterPanelState) {
		this.state = state;
	}

	public function refresh(encounter:Encounter, active:Array<Combatant>, elapsed:Float):Void {
		var maxTotal = active.length > 0 ? active[0].stats.total : 0.0;
		var totalDamage = 0.0;
		for (c in active) totalDamage += c.stats.total;

		rows = [
			for (c in active) {
				{
					name: c.name,
					className: c.className,
					icon: c.icon,
					total: c.stats.total,
					dps: c.stats.dps(elapsed),
					pct: totalDamage > 0 ? c.stats.total / totalDamage * 100 : 0,
					hits: c.stats.hits,
					crits: c.stats.crits,
					share: maxTotal > 0 ? Math.max(0, Math.min(1, c.stats.total / maxTotal)) : 0,
				};
			}
		];

		skillRowsByName = [for (c in active) c.name => buildSkillRows(c, elapsed)];
	}

	// share is relative to this combatant's own top skill (not pct/100) - intentional, not a bug.
	function buildSkillRows(c:Combatant, elapsed:Float):Array<MeterRowData> {
		var maxSkillTotal = 0.0;
		for (skill in c.skills) if (skill.stats.total > maxSkillTotal) maxSkillTotal = skill.stats.total;

		var skillRows:Array<MeterRowData> = [
			for (skill in c.skills) {
				{
					name: skill.name,
					className: "",
					icon: skill.icon,
					total: skill.stats.total,
					dps: skill.stats.dps(elapsed),
					pct: c.stats.total > 0 ? skill.stats.total / c.stats.total * 100 : 0,
					hits: skill.stats.hits,
					crits: skill.stats.crits,
					share: maxSkillTotal > 0 ? Math.max(0, Math.min(1, skill.stats.total / maxSkillTotal)) : 0,
				};
			}
		];
		skillRows.sort((a, b) -> a.total == b.total ? 0 : (a.total > b.total ? -1 : 1));
		return skillRows;
	}

	public function draw():Void {
		var game = GameApp.get();
		if (game == null) return;

		ImGui.setNextWindowPos(ImGui.vec2(state.x, state.y), ImGuiCond.FirstUseEver);
		ImGui.setNextWindowSize(ImGui.vec2(state.width, state.height), ImGuiCond.FirstUseEver);
		ImGui.setNextWindowCollapsed(state.collapsed, ImGuiCond.FirstUseEver);

		// NoInputs while the cursor is hidden for camera control, so the meter doesn't steal gameplay clicks.
		var flags = game.isCursorFree() ? 0 : ImGuiWindowFlags.NoInputs;
		if (ImGui.begin("Pew Pew Meter", null, flags)) {
			if (ImGui.beginPopupContextItem("PewPewMeterTitleContext", ImGuiPopupFlags.MouseButtonRight)) {
				if (ImGui.menuItem("Reset")) {
					pew.tracking.DpsTracker.instance.manualReset();
					rows = [];
				}
				ImGui.endPopup();
			}
			drawContent();
		}
		reportState();
		ImGui.end();
	}

	function drawContent():Void {
		if (rows.length == 0) {
			ImGui.text("No damage recorded yet.");
			return;
		}

		// NoSavedSettings - the column set has changed shape during dev; a stale persisted layout would scramble display order.
		if (ImGui.beginTable("MeterRows", 7,
			ImGuiTableFlags.SizingStretchProp | ImGuiTableFlags.RowBg | ImGuiTableFlags.Sortable | ImGuiTableFlags.NoSavedSettings)) {
			ImGui.tableSetupColumn(" C", ImGuiTableColumnFlags.WidthFixed | ImGuiTableColumnFlags.NoSort, 20);
			ImGui.tableSetupColumn("Name");
			ImGui.tableSetupColumn("Dmg");
			ImGui.tableSetupColumn("DPS");
			ImGui.tableSetupColumn("Hits/Crits");
			ImGui.tableSetupColumn("%",
				ImGuiTableColumnFlags.WidthStretch | ImGuiTableColumnFlags.DefaultSort | ImGuiTableColumnFlags.PreferSortDescending, 1.75);
			ImGui.tableSetupColumn("", ImGuiTableColumnFlags.WidthFixed | ImGuiTableColumnFlags.NoSort, 20);

			ImGui.tableNextRow(ImGuiTableRowFlags.Headers);
			ImGui.tableSetColumnIndex(0);
			ImGui.tableHeader(" C");
			ImGui.tableSetColumnIndex(1);
			ImGui.tableHeader("Name");
			ImGui.tableSetColumnIndex(2);
			ImGui.tableHeader("Dmg");
			ImGui.tableSetColumnIndex(3);
			ImGui.tableHeader("DPS");
			ImGui.tableSetColumnIndex(4);
			ImGui.tableHeader("Hits/Crits");
			ImGui.tableSetColumnIndex(5);
			ImGui.tableHeader("%");
			ImGui.tableSetColumnIndex(6);
			ImGui.tableHeader("");

			applySortSpecs();

			for (i in 0...rows.length) {
				var row = rows[i];
				ImGui.tableNextRow();
				drawRow(row, true);
				ImGui.tableNextColumn();
				drawOpenDetailButton(row, i);
			}

			ImGui.endTable();
		}
	}

	// pushID_Int/popID keeps each row's button ID distinct - button() hashes its ID from the label alone, and every row shares the same label.
	function drawOpenDetailButton(row:MeterRowData, index:Int):Void {
		ImGui.pushStyleVar(ImGuiStyleVar.FramePadding, ImGui.vec2(2, 0));
		ImGui.pushID_Int(index);
		if (ImGui.button(">", ImGui.vec2(20, 0)) && onOpenDetail != null) onOpenDetail(row.name);
		ImGui.popID();
		ImGui.popStyleVar();
	}

	// Shared with CombatantDetailPanel; showProgressBar=false only for its rollup row (a self-relative bar there would be a no-op 100% fill).
	public static function drawRow(row:MeterRowData, withTooltip:Bool, showProgressBar:Bool = true):Void {
		ImGui.tableNextColumn();
		if (row.icon != null) {
			var offset = (ImGui.getContentRegionAvail().x - 16) / 2;
			if (offset > 0) ImGui.setCursorPosX(ImGui.getCursorPosX() + offset);
			ImGui.image(row.icon.texId, ImGui.vec2(16, 16), row.icon.uv0, row.icon.uv1);
			if (withTooltip && ImGui.beginItemTooltip()) {
				ImGui.text(row.className);
				ImGui.endTooltip();
			}
		}
		ImGui.tableNextColumn();
		ImGui.text(row.name);
		if (ImGui.beginItemTooltip()) {
			ImGui.text(row.name);
			ImGui.endTooltip();
		}
		ImGui.tableNextColumn();
		ImGui.text(Std.string(Math.round(row.total)));
		ImGui.tableNextColumn();
		ImGui.text(Std.string(Math.round(row.dps)));
		ImGui.tableNextColumn();
		ImGui.text('${row.hits}/${row.crits}');
		ImGui.tableNextColumn();
		if (showProgressBar) ImGui.progressBar(row.share, ImGui.vec2(-1, ImGui.getFrameHeight() / 2), '${Math.round(row.pct)}%');
	}

	public function findRowByName(name:String):MeterRowData {
		for (row in rows) if (row.name == name) return row;
		return null;
	}

	public function findSkillRows(name:String):Array<MeterRowData> {
		var skillRows = skillRowsByName.get(name);
		return skillRows != null ? skillRows : [];
	}

	// Must run after headers are submitted; column indices match tableSetupColumn() order (0 = icon, NoSort).
	function applySortSpecs():Void {
		var specs = ImGui.tableGetSortSpecs();
		if (specs == null || !ImGui.tableSortSpecsGetSpecsDirty(specs) || ImGui.tableSortSpecsGetSpecsCount(specs) == 0) return;

		var column = ImGui.tableSortSpecsGetColumnIndex(specs, 0);
		var descending = ImGui.tableSortSpecsGetSortDirection(specs, 0) == ImGuiSortDirection.Descending;
		rows.sort((a, b) -> {
			var cmp = switch (column) {
				case 1: Reflect.compare(a.name, b.name);
				case 2: Reflect.compare(a.total, b.total);
				case 3: Reflect.compare(a.dps, b.dps);
				case 4: Reflect.compare(a.hits, b.hits);
				case 5: Reflect.compare(a.pct, b.pct);
				default: 0;
			}
			return descending ? -cmp : cmp;
		});
		ImGui.tableSortSpecsSetSpecsDirty(specs, false);
	}

	function reportState():Void {
		if (onStateChanged == null) return;

		var pos = ImGui.getWindowPos();
		var size = ImGui.getWindowSize();
		var next:MeterPanelState = {
			x: pos.x,
			y: pos.y,
			width: Std.int(size.x),
			height: Std.int(size.y),
			collapsed: ImGui.isWindowCollapsed(),
		};
		if (next.x == state.x
			&& next.y == state.y
			&& next.width == state.width
			&& next.height == state.height
			&& next.collapsed == state.collapsed)
			return;

		state = next;
		onStateChanged(next);
	}
}
