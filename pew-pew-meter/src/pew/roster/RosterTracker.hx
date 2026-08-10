package pew.roster;

import ent.Unit;
import st.Player;
import pew.panel.IconCache;

// Combatants are never pruned - a departed/respawned member just stops accumulating and drops out of reports once its total is 0.
class RosterTracker {
	var combatants:Array<Combatant> = [];

	public function new() {}

	public function refresh(app:GameApp):Void {
		var me = app.me;
		if (me == null) return;
		var group = me.group;
		if (group == null) return;
		var players = group.players;
		if (players == null) return;

		for (raw in players.array) {
			var p:Player = cast raw;
			var hero = p.hero;
			if (hero == null) continue;

			var heroRef:Dynamic = cast hero;
			var combatant = findByRef(heroRef);
			if (combatant == null) {
				combatants.push(new Combatant(heroRef, hero.name, hero.inf.texts.name, IconCache.resolve(hero.inf.gfx, false)));
			} else {
				combatant.name = hero.name;
				combatant.className = hero.inf.texts.name;
			}
		}
	}

	public function findBySourceUnit(unit:Unit):Combatant {
		return findByRef(cast unit);
	}

	public function anyInCombat():Bool {
		for (c in combatants) if (c.inCombat) return true;
		return false;
	}

	public function all():Array<Combatant> {
		return combatants;
	}

	public function resetStats():Void {
		for (c in combatants) c.resetStats();
	}

	// Used when Encounter's idle-timeout force-closes a stuck encounter - otherwise the stuck combatant's inCombat stays true forever, blocking anyInCombat() for the next real fight too.
	public function clearInCombat():Void {
		for (c in combatants) c.inCombat = false;
	}

	function findByRef(ref:Dynamic):Combatant {
		for (c in combatants) if (c.heroRef == ref) return c;
		return null;
	}

	public static function sortActiveByDamage(combatants:Array<Combatant>):Array<Combatant> {
		var active = combatants.filter(c -> c.stats.total > 0);
		active.sort((a, b) -> {
			if (a.stats.total == b.stats.total) return 0;
			return a.stats.total > b.stats.total ? -1 : 1;
		});
		return active;
	}
}
