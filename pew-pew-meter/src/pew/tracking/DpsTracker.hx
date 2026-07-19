package pew.tracking;

import ent.Unit;
import st.skill.DamageResult;
import pew.roster.RosterTracker;
import pew.report.DpsReporter;
import pew.report.PanelDpsReporter;

//	Orchestrates one running DPS session: owns the roster, the current encounter, the reporter, and the two tick intervals (roster refresh, periodic report).
class DpsTracker {
	public static final instance = new DpsTracker();

	static inline var ROSTER_REFRESH_INTERVAL = 2.0;
	static inline var REPORT_INTERVAL = 1.0;
	static inline var IDLE_TIMEOUT = 10.0;

	var rosterTracker = new RosterTracker();
	var encounter = new Encounter();
	var reporter:DpsReporter;
	var nextRosterRefresh = 0.0;
	var nextReport = 0.0;

	var panel:pew.panel.MeterPanel;

	function new() {
		reporter = new PanelDpsReporter(() -> panel);
	}

	public function onGameAppUpdate(app:GameApp):Void {
		var now = Time.get_appTime();

		if (now >= nextRosterRefresh) {
			rosterTracker.refresh(app);
			nextRosterRefresh = now + ROSTER_REFRESH_INTERVAL;
		}

		if (encounter.active) {
			if (encounter.isStale(now, IDLE_TIMEOUT)) {
				reporter.report(encounter, rosterTracker.all(), true);
				rosterTracker.clearInCombat();
				encounter.end();
			} else if (now >= nextReport) {
				reporter.report(encounter, rosterTracker.all(), false);
				nextReport = now + REPORT_INTERVAL;
			}
		}

		if (app.gui != null) {
			var container = app.gui.getDragContainer();
			var containerScene = container != null ? container.getScene() : null;
			if (containerScene != null && (panel == null || panel.root.getScene() != containerScene)) {
				panel = new pew.panel.MeterPanel(container, true);
				panel.loadState(PewPewMeterMod.config.meterPanelState);
				panel.onStateChanged = state -> {
					PewPewMeterMod.config.meterPanelState = state;
					PewPewMeterMod.config.save();
				};
			}
		}
	}

	public function onEnterCombat(unit:Unit):Void {
		var combatant = rosterTracker.findBySourceUnit(unit);
		if (combatant == null) {
			return;
		}
		combatant.inCombat = true;

		if (!encounter.active) {
			startEncounter();
		}
	}

	public function onLeaveCombat(unit:Unit):Void {
		var combatant = rosterTracker.findBySourceUnit(unit);
		if (combatant == null) {
			return;
		}
		combatant.inCombat = false;

		if (!encounter.active || rosterTracker.anyInCombat()) {
			return;
		}
		reporter.report(encounter, rosterTracker.all(), true);
		encounter.end();
	}

	public function onInflictDamage(unit:Unit, dmg:DamageResult):Void {
		var combatant = rosterTracker.findBySourceUnit(unit);
		if (combatant == null) {
			return;
		}
		var amount = dmg.get_amount();
		if (amount <= 0) {
			return;
		}
		combatant.inCombat = true;
		if (encounter.active) {
			encounter.recordHit();
			combatant.stats.record(amount, dmg.get_critical());
		}
	}

	function startEncounter():Void {
		rosterTracker.resetStats();
		encounter.start();
		nextReport = Time.get_appTime() + REPORT_INTERVAL;
	}

	public function manualReset():Void {
		encounter.reset();
		rosterTracker.resetStats();
	}
}
