package pew.roster;

import ent.Unit;
import st.Player;

/**
	Owns "who are we tracking": the group roster (rebuilt periodically,
	since party composition and who's-playing-which-hero can change
	mid-session - see `pew.tracking.DpsTracker`) and identity lookup from a
	damage-inflicting `ent.Unit` back to the `Combatant` it belongs to.

	Roster source: `GameApp.me:st.Player` -> `me.group:st.Group` ->
	`group.players.array:Array<Dynamic>`, each element cast back to
	`st.Player` for its `.hero:ent.Hero` (`Hero` carries its own display
	`.name`, so there's no need to also resolve `Player.name`).

	Identity matching compares the erased `Dynamic` reference behind both
	`ent.Hero`/`ent.Unit` abstracts with `==` (both are `abstract
	X(Dynamic)`, so the underlying object *is* the real identity). A plain
	`Array` scan does the lookup instead of `Map<Dynamic,_>` (whose
	behavior for arbitrary object keys isn't something worth relying on
	here) - fine since a party is always a handful of members.

	Note: combatants are never pruned (a member who leaves the group, or
	whose hero instance is replaced on respawn, just stops accumulating
	damage and drops out of reports once its total is 0) - acceptable for a
	first cut, and harmless since reporting only shows entries with damage.
**/
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
				combatants.push(new Combatant(heroRef, hero.name));
			} else {
				combatant.name = hero.name;
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
		for (c in combatants) c.stats.reset();
	}

	/**
		Forces every combatant's `inCombat` false - used when `pew.tracking.
		Encounter`'s idle-timeout safety net force-closes a stuck encounter
		(see `Encounter.isStale`). Without this, whichever combatant's stray
		hit started that stuck encounter would stay `inCombat = true` forever
		too (no `onLeaveCombat` ever arrived for it, by definition of why the
		encounter got stuck) - which would then permanently block
		`anyInCombat()` from ever returning `false` again, breaking the
		*next* legitimate fight's `onLeaveCombat` close-out as well.
	**/
	public function clearInCombat():Void {
		for (c in combatants) c.inCombat = false;
	}

	function findByRef(ref:Dynamic):Combatant {
		for (c in combatants) if (c.heroRef == ref) return c;
		return null;
	}

	/**
		Combatants with damage this encounter, highest total first. Shared by
		`pew.report.PanelDpsReporter` and `pew.tracking.DpsTracker`/`pew.panel.MeterPanel`
		so there's exactly one definition of "who's shown and in what order."
		Static (not instance) since it just operates on whatever
		`Array<Combatant>` it's given.
	**/
	public static function sortActiveByDamage(combatants:Array<Combatant>):Array<Combatant> {
		var active = combatants.filter(c -> c.stats.total > 0);
		active.sort((a, b) -> {
			if (a.stats.total == b.stats.total) return 0;
			return a.stats.total > b.stats.total ? -1 : 1;
		});
		return active;
	}
}
