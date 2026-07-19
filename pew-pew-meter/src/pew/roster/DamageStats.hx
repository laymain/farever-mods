package pew.roster;

/**
	Accumulated damage for one combatant over the current encounter. Owns
	its own mutation (`record`/`reset`) instead of being a plain bag that
	external code pokes fields on directly.
**/
class DamageStats {
	public var total(default, null) = 0.0;
	public var hits(default, null) = 0;
	public var crits(default, null) = 0;

	public function new() {}

	public function record(amount:Float, isCritical:Bool):Void {
		total += amount;
		hits++;
		if (isCritical) crits++;
	}

	// A minimum floor on the divisor, not on whether a dps is reported at
	// all: a fight's very first hit sets `startedAt`/`lastHitAt` (see
	// pew.tracking.Encounter) only microseconds apart, since `startEncounter()`
	// calls `encounter.start()` immediately followed by `encounter.
	// recordHit()` in the same call - `duration` is then a genuine but
	// absurdly tiny positive number, and dividing by it produces a
	// multi-million "dps" spike from a single hit. A rate isn't actually
	// meaningful from one instant anyway; clamping the divisor to a sane
	// floor (rather than suppressing the row/report entirely) gives a
	// conservative, sane-looking estimate that converges to the true value
	// as more real time passes - the same approach real DPS meters use.
	static inline var MIN_DURATION = 0.5;

	public function dps(duration:Float):Float {
		return total > 0 ? total / (duration > MIN_DURATION ? duration : MIN_DURATION) : 0;
	}

	public function reset():Void {
		total = 0;
		hits = 0;
		crits = 0;
	}
}
