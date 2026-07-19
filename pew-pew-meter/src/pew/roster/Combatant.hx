package pew.roster;

/**
	One tracked hero: identity (the erased `Dynamic` behind its `ent.Hero`,
	used for `==` matching - see `RosterTracker`), display name, current
	combat-state flag, and its own `DamageStats`.
**/
class Combatant {
	public var heroRef:Dynamic;
	public var name:String;
	public var inCombat = false;
	public final stats = new DamageStats();

	public function new(heroRef:Dynamic, name:String) {
		this.heroRef = heroRef;
		this.name = name;
	}
}
