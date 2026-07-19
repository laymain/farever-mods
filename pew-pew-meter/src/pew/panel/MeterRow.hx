package pew.panel;

@:build(pew.ui.macro.RowBuilder.build("ui/meter_row.xml"))
class MeterRow {
	static inline var BAR_MAX_WIDTH = 160;

	public function update(name:String, total:Float, dps:Float, pct:Float, hits:Int, crits:Int, share:Float):Void {
		container.visible = true;
		nameText.text = name;
		statsText.text = '${Math.round(total)} dmg  ${Math.round(dps)} dps  ${Math.round(pct)}%  (${hits} hits, ${crits} crit)';
		var w = Std.int(BAR_MAX_WIDTH * Math.max(0, Math.min(1, share)));
		bar.width = w < 1 ? 1 : w;
	}

	public function hide():Void {
		container.visible = false;
	}
}
