package pew.tracking;

/**
	One combat session's timing: when it started, when the last real hit
	landed, and whether it's currently open.

	`elapsed()` is measured to the last hit, not to "now": the latter keeps
	shrinking the longer `now` drifts past the last real hit (e.g. while
	`DpsTracker` is still waiting on a trailing `onLeaveCombat`), which reads
	as the fight's DPS decaying even though no more damage happened.
	Freezing the window at the last hit gives a number that holds steady
	once damage actually stops.
**/
class Encounter {
	public var active(default, null) = false;

	var startedAt = -1.0;
	var lastHitAt = -1.0;

	public function new() {}

	public function start():Void {
		var now = Time.get_appTime();
		startedAt = now;
		lastHitAt = now;
		active = true;
	}

	public function recordHit():Void {
		lastHitAt = Time.get_appTime();
	}

	public function elapsed():Float {
		return lastHitAt - startedAt;
	}

	/**
		True once an active encounter has gone `timeout` seconds without a
		hit - a safety net for `onLeaveCombat` never firing for whichever hit
		most recently (re)started it. `onInflictDamage`'s self-healing path
		(see `pew.tracking.DpsTracker`) starts a new encounter off of *any* damage
		to a tracked hero, including a straggler tick that lands just as the
		game's own combat state is already winding down (e.g. a killing
		blow's damage-over-time tail) - if that particular unit's matching
		`onLeaveCombat` never arrives, `active` would otherwise stay stuck
		`true` forever, and the periodic report would repeat that one stale
		snapshot indefinitely (observed live: a real "0s (40 total)" fight
		that never closed, logged every report tick forever after).
	**/
	public function isStale(now:Float, timeout:Float):Bool {
		return active && (now - lastHitAt) > timeout;
	}

	/**
		Ends the encounter without clearing its timing - `elapsed()` keeps
		returning the just-completed pull's duration so a live display (the
		panel) keeps showing its final numbers instead of blanking the
		instant combat ends. Use `reset()` instead for a full clear (manual
		reset button).
	**/
	public function end():Void {
		active = false;
		// startedAt/lastHitAt intentionally left alone - elapsed() keeps
		// returning the last completed pull's duration for continued display
		// until start() is called again for the next pull.
	}

	public function reset():Void {
		active = false;
		startedAt = -1;
		lastHitAt = -1;
	}
}
