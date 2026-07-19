package pew.ui;

/**
	A composition-only reusable button "widget". Mods can't extend `h2d.
	Object` (composition only, per `documentation/MODDING_FINAL.md`), so
	there's no hand-written `ui.Button` class to instantiate the way
	`hlx-ui-kit-poc`'s `-lib heaps` prototype does (`ui.Button extends h2d.
	Object`) - instead this builds a padded, background-filled `h2d.Flow`
	with a centered label and a real `Interactive` wired to `onClick`, and
	hands back the `Flow` itself. Copied back in from the shared `hlx-ui-kit`
	library as pew-pew-meter's own local copy - see `pew.ui.Gfx`'s own doc comment
	for why.

	Click wiring uses the real generated `h2d.Interactive.onClick` signature,
	`(hxd.Event) -> Void`, directly - no `Dynamic`/`Reflect` needed (that was
	a workaround for an older, since-lifted generator limitation).

	`captureLabel`/`minContentWidth` cover the one real variation
	`pew.panel.MeterPanel`'s XML-driven collapse toggle needs beyond the basic
	case: a caller that wants the label `h2d.Text` back (to mutate it later,
	e.g. the collapse toggle's "+"/"-" glyph) and/or a fixed content width so
	the button doesn't resize when its label text changes.
**/
class Button {
	public static function create(parent:h2d.Flow, label:String, onClick:hxd.Event->Void, padding:Int = 4,
			?captureLabel:h2d.Text->Void, ?minContentWidth:Float):h2d.Flow {
		var btn = new h2d.Flow(parent);
		btn.set_padding(padding);
		btn.backgroundTile = Gfx.solidTile(0x333333, 1, 1, 0.9);
		btn.enableInteractive = true;
		btn.interactive.onClick = onClick;

		if (minContentWidth != null) {
			var w = Math.ceil(minContentWidth) + padding * 2;
			btn.minWidth = w;
			btn.maxWidth = w;
			btn.horizontalAlign = h2d.FlowAlign.Middle; // keep the glyph
			// centered within the fixed-width box regardless of which
			// character is showing.
		}

		var text = new h2d.Text(Gfx.defaultFont(), btn);
		text.text = label;
		text.textColor = 0xFFFFFF;
		if (captureLabel != null) captureLabel(text);

		return btn;
	}
}
