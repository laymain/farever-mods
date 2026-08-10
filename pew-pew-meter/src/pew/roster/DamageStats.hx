package pew.roster;

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

	// Floors the divisor - a fight's first hit lands microseconds after start(), and dividing by that tiny duration spikes to a multi-million "dps".
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
