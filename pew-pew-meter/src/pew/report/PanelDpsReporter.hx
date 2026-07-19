package pew.report;

import pew.tracking.Encounter;
import pew.roster.Combatant;
import pew.roster.RosterTracker;
import pew.panel.MeterPanel;

/**
	Refreshes the live `MeterPanel`, if one currently exists, with the same
	cadence `pew.tracking.DpsTracker` already gates trace-style reporters on
	(`REPORT_INTERVAL`, plus the unconditional final report on encounter end) -
	panel construction/attachment stays in `DpsTracker` (it's the only place
	with `GameApp`/scene access), so the panel is read via an accessor closure
	rather than owned here.
**/
class PanelDpsReporter implements DpsReporter {
	final getPanel:Void->MeterPanel;

	public function new(getPanel:Void->MeterPanel) {
		this.getPanel = getPanel;
	}

	public function report(encounter:Encounter, combatants:Array<Combatant>, isFinal:Bool):Void {
		var panel = getPanel();
		if (panel == null) return;

		var active = RosterTracker.sortActiveByDamage(combatants);
		panel.refresh(encounter, active, encounter.elapsed());
	}
}
