package pew.report;

import pew.tracking.Encounter;
import pew.roster.Combatant;

/**
	Output seam for an in-progress/completed encounter snapshot - swapping
	`pew.report.PanelDpsReporter` for another reporter (e.g. a trace/console
	one) is a one-line change in `pew.tracking.DpsTracker`, not a rewrite of
	anything that computes numbers.
**/
interface DpsReporter {
	function report(encounter:Encounter, combatants:Array<Combatant>, isFinal:Bool):Void;
}
