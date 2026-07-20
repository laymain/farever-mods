package pew.panel;

import pew.tracking.Encounter;
import pew.roster.Combatant;

typedef MeterPanelState = {
	var x:Float;
	var y:Float;
	var width:Int;
	var height:Int;
	var collapsed:Bool;
}

@:build(pew.ui.macro.PanelBuilder.build("ui/meter_panel.xml"))
class MeterPanel {
	var rows:Array<MeterRow> = [];

	public var onStateChanged:MeterPanelState->Void;

	function onResetClicked(e:hxd.Event):Void {
		pew.tracking.DpsTracker.instance.manualReset();
		clear();
	}

	public function clear():Void {
		for (row in rows) row.hide();
		rowsFlow.needReflow = true;
	}

	function onStateMutated():Void {
		if (onStateChanged != null) onStateChanged(getState());
	}

	public function getState():MeterPanelState {
		var width = collapsed ? savedMaxWidth : root.maxWidth;
		return {
			x: root.x,
			y: root.y,
			width: width == null ? 0 : width,
			height: rowsFlow.maxHeight == null ? 0 : rowsFlow.maxHeight,
			collapsed: collapsed,
		};
	}

	public function loadState(state:MeterPanelState):Void {
		if (collapsed) toggleCollapse();

		root.x = state.x;
		root.y = state.y;
		root.minWidth = state.width;
		root.maxWidth = state.width;
		rowsFlow.minHeight = state.height;
		rowsFlow.maxHeight = state.height;

		if (state.collapsed) toggleCollapse();
	}

	public function refresh(encounter:Encounter, active:Array<Combatant>, elapsed:Float):Void {
		while (rows.length < active.length) rows.push(new MeterRow(rowsFlow));

		var maxTotal = active.length > 0 ? active[0].stats.total : 0.0;
		var totalDamage = 0.0;
		for (c in active) totalDamage += c.stats.total;

		for (i in 0...rows.length) {
			if (i < active.length) {
				var c = active[i];
				var pct = totalDamage > 0 ? c.stats.total / totalDamage * 100 : 0;
				var share = maxTotal > 0 ? c.stats.total / maxTotal : 0;
				rows[i].update(c.name, c.stats.total, c.stats.dps(elapsed), pct, c.stats.hits, c.stats.crits, share);
			} else {
				rows[i].hide();
			}
		}

		rowsFlow.needReflow = true;
		clampToScreen();
	}

	function clampToScreen():Void {
		var scene = root.getScene();
		if (scene == null) return;

		var size = pew.ui.Gfx.windowSize(scene);
		if (size.width <= 0 || size.height <= 0) return;

		var scale = root.scaleX;
		var panelHeight = root.get_outerHeight() * scale;
		var maxY = Math.max(0, size.height - panelHeight);
		if (root.y > maxY) root.y = maxY;

		var panelWidth = root.get_outerWidth() * scale;
		var maxX = Math.max(0, size.width - panelWidth);
		if (root.x > maxX) root.x = maxX;
	}
}
