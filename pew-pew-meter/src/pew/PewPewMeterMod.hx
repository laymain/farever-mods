package pew;

import ent.Unit;
import st.skill.DamageResult;
import imgui.ImGui;
import pew.tracking.DpsTracker;
import pew.panel.PanelTheme;
import pew.panel.PanelTheme.PanelThemeConfig;
import pew.panel.MeterPanel;
import pew.panel.MeterPanel.MeterPanelState;
import pew.panel.CombatantDetailPanel;
import pew.panel.CombatantDetailPanel.DetailPanelState;

typedef PewPewMeterConfig = {
	var meterPanelState:MeterPanelState;
	var detailPanelState:DetailPanelState;
	var theme:PanelThemeConfig;
}

@:build(hlx.runtime.Mod.build())
class PewPewMeterMod {
	@:hlx.config
	public static var config(default, null):PewPewMeterConfig = {
		meterPanelState: {x: 16, y: 16, width: 400, height: 45, collapsed: false},
		detailPanelState: {x: 440, y: 16, opened: false, collapsed: false},
		theme: PanelTheme.defaultConfig()
	};

	static function main():Void {
		var panel = new MeterPanel(config.meterPanelState);
		panel.onStateChanged = state -> {
			config.meterPanelState = state;
			config.save();
		};

		var detailPanel = new CombatantDetailPanel(panel, config.detailPanelState, PanelTheme.color(config.theme.rollupRowBg));
		detailPanel.onStateChanged = state -> {
			config.detailPanelState = state;
			config.save();
		};
		panel.onOpenDetail = detailPanel.show;

		var theme = PanelTheme.build(config.theme);
		DpsTracker.instance.panel = panel;
		ImGui.register(HlxRuntime.moduleName(), () -> theme.wrap(() -> {
			panel.draw();
			detailPanel.draw();
		}));
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
