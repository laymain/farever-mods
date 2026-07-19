package pew.ui;

/**
	Solid-color-tile / default-font / cursor / window-size helpers for
	building `h2d` UI against `farever-gamelib`. Originally factored out into
	the shared `hlx-ui-kit` library once `hlx-ui-kit-poc-mod` needed the same
	helpers as this mod's own (since-removed) `panel/GameGfx.hx`; copied back
	in here as pew-pew-meter's own local copy - a fully generic shared UI
	library turned out to be more indirection than payoff for what this mod
	actually needs (see `../hlx-ui-kit/README.md`). `hlx-ui-kit-poc-mod` still
	depends on the shared library; this copy only serves this mod and
	`hlx-ui-mod` (which reuses `pew.panel.*` unmodified via a shared `-cp`, see
	that project's own README).

	**`solidTile`**: `h2d.Tile.fromColor` (real Heaps' own convenience static
	helper) does not exist anywhere in `farever-gamelib` - nothing in the
	currently-installed game build calls it directly, so Haxe's dead-code
	elimination stripped it out of the source the generator reads. `h3d.mat.
	Texture.fromColor(color, alpha)`, the lower-level helper it wraps
	internally, IS generated and is what this calls instead.
**/
class Gfx {
	/** A real, solid-color `h2d.Tile` of the given size. **/
	public static function solidTile(color:Int, width:Int, height:Int, alpha:Float = 1.0):h2d.Tile {
		var tex = h3d.mat.Texture.fromColor(color, alpha);
		var dx = 0.0;
		var dy = 0.0;
		return new h2d.Tile(tex, 0, 0, width, height, dx, dy);
	}

	/** The game's own default UI font. **/
	public static function defaultFont():h2d.Font
		return hxd.res.DefaultFont.get();

	public static function cursorMove():hxd.Cursor
		return hxd.Cursor.Move;

	public static function cursorResize():hxd.Cursor
		return hxd.Cursor.ResizeNWSE;

	/** Real window pixel dimensions, read off the given `h2d.Scene`'s own
		`window:hxd.Window` reference rather than the Scene's own `width`/
		`height` fields directly - whatever `Scene` a HUD-style panel sits
		under commonly reads those back as `0` (confirmed live by this mod's
		own screen-edge clamp, `pew.panel.MeterPanel.clampToScreen`). Callers
		should still check the returned size for `<= 0` themselves. **/
	public static function windowSize(scene:h2d.Scene):{width:Int, height:Int} {
		var window = scene.window;
		if (window == null) return {width: 0, height: 0};
		return {width: window.windowWidth, height: window.windowHeight};
	}
}
