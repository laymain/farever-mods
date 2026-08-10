package pew.roster;

import pew.panel.IconCache;
import pew.panel.IconCache.Icon;

typedef SkillStats = {
	var name:String;
	var icon:Icon;
	var stats:DamageStats;
}

class Combatant {
	public var heroRef:Dynamic;
	public var name:String;
	public var className:String;
	public var icon:Icon;
	public var inCombat = false;
	public final stats = new DamageStats();
	public final skills = new Map<String, SkillStats>();

	public function new(heroRef:Dynamic, name:String, className:String, icon:Icon) {
		this.heroRef = heroRef;
		this.name = name;
		this.className = className;
		this.icon = icon;
	}

	// gfx is only consulted the first time `key` is seen (to resolve the icon); pass null for damage with no resolvable skill.
	public function recordSkillDamage(key:String, name:String, gfx:Null<{file:String, x:Int, y:Int, width:Null<Int>, height:Null<Int>}>, amount:Float,
			isCritical:Bool):Void {
		var bucket = skills.get(key);
		if (bucket == null) {
			bucket = {name: name, icon: gfx != null ? IconCache.resolve(gfx) : null, stats: new DamageStats()};
			skills.set(key, bucket);
		}
		bucket.stats.record(amount, isCritical);
	}

	// Clears skills outright (not just each bucket's stats) so an unused skill has no row instead of a stale zero-damage entry.
	public function resetStats():Void {
		stats.reset();
		skills.clear();
	}
}
