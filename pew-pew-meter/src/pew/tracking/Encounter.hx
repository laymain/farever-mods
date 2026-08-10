package pew.tracking;

// elapsed() is measured to the last hit, not "now" - otherwise DPS keeps decaying while DpsTracker waits on a trailing onLeaveCombat.
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

	// Safety net for onLeaveCombat never firing - without this a stuck encounter reports its stale snapshot forever (observed live).
	public function isStale(now:Float, timeout:Float):Bool {
		return active && (now - lastHitAt) > timeout;
	}

	// Leaves startedAt/lastHitAt alone so elapsed() keeps showing the just-completed pull's duration; use reset() for a full clear.
	public function end():Void {
		active = false;
	}

	public function reset():Void {
		active = false;
		startedAt = -1;
		lastHitAt = -1;
	}
}
