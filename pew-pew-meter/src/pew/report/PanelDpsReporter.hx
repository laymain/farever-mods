package pew.report;

import pew.tracking.Encounter;
import pew.roster.Combatant;
import pew.roster.RosterTracker;
import pew.panel.MeterPanel;

// Reads MeterPanel via closure rather than owning it - DpsTracker is the only place with GameApp/scene access to construct one.
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
