package pew;

import ent.Unit;
import st.skill.DamageResult;
import pew.tracking.DpsTracker;
import pew.panel.MeterPanel.MeterPanelState;

typedef PewPewMeterConfig = {
	var meterPanelState:MeterPanelState;
}

@:build(hlx.runtime.Mod.build())
class PewPewMeterMod {
	@:hlx.config
	public static var config(default, null):PewPewMeterConfig = {
		meterPanelState: {x: 16, y: 16, width: 400, height: 45, collapsed: true}
	};

	static function main():Void {
		trace("loaded");
	}

	@:hlx.postfix(GameApp.update)
	static function afterGameAppUpdate(instance:GameApp, dt:Float, result:Void):Void {
		DpsTracker.instance.onGameAppUpdate(instance);
	}

	@:hlx.postfix(ent.Unit.onEnterCombat)
	static function afterOnEnterCombat(instance:Unit, result:Void):Void {
		DpsTracker.instance.onEnterCombat(instance);
	}

	@:hlx.postfix(ent.Unit.onLeaveCombat)
	static function afterOnLeaveCombat(instance:Unit, result:Void):Void {
		DpsTracker.instance.onLeaveCombat(instance);
	}

	@:hlx.postfix(ent.Unit.onInflictDamage)
	static function afterOnInflictDamage(instance:Unit, dmg:DamageResult, result:Void):Void {
		DpsTracker.instance.onInflictDamage(instance, dmg);
	}
}
