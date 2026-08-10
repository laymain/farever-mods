package pew.report;

import pew.tracking.Encounter;
import pew.roster.Combatant;

// Output seam - swaps in another reporter (e.g. a trace one) without touching anything that computes numbers.
interface DpsReporter {
	function report(encounter:Encounter, combatants:Array<Combatant>, isFinal:Bool):Void;
}
