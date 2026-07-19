package pew.ui;

/**
	Generic Flow drag/resize capture, generalized from the shared plumbing
	behind `pew.panel.MeterPanel`'s original private `beginDragCapture`/
	`enableDrag`/`enableResize`. Copied back in from the shared `hlx-ui-kit`
	library as pew-pew-meter's own local copy - see `pew.ui.Gfx`'s own doc comment
	for why.

	Capture goes through `h2d.Scene.startCapture` (viewport-space events),
	not `h2d.Interactive.startCapture` (self-transformed local-space events -
	drifts/oscillates whenever the dragged object is an ancestor of the
	dragging Interactive itself, which is exactly the title-bar-drags-its-own-
	panel-root case this exists for).

	Uses the real typed `hxd.Event`/`hxd.EventKind` directly (`e.kind.
	isEMove()`, `e.relX`, `e.propagate`), not `Dynamic`/`Reflect` - this mod's
	own original version predated the generator cleanup that made this
	possible; that workaround is no longer needed.
**/
class Drag {
	/**
		Starts a scene-wide event capture, invoking `onDelta(dx, dy)` with the
		accumulated offset from the press/first-move position on every
		`EMove`/`EPush` while captured, then `onEnd()` once (if given) on
		release, before stopping the capture. `anchor` is any object with a
		real `getScene()` (typically the Flow being dragged/resized, or one of
		its descendants).
	**/
	public static function beginCapture(anchor:h2d.Object, onDelta:(dx:Float, dy:Float) -> Void, ?onEnd:Void->Void):Void {
		var scene = anchor.getScene();
		if (scene == null) return;

		var haveAnchor = false;
		var anchorX = 0.0;
		var anchorY = 0.0;

		scene.startCapture(function(e:hxd.Event) {
			var relX = e.relX;
			var relY = e.relY;
			if (e.kind.isEMove() || e.kind.isEPush()) {
				if (!haveAnchor) {
					anchorX = relX;
					anchorY = relY;
					haveAnchor = true;
				}
				onDelta(relX - anchorX, relY - anchorY);
			} else if (e.kind.isERelease() || e.kind.isEReleaseOutside()) {
				scene.stopCapture();
				if (onEnd != null) onEnd();
			}
			e.propagate = false;
		}, null, null);
	}

	/** Makes `bar` draggable: enables its Interactive, and on press starts a
		capture that moves `target.x`/`target.y` by the drag delta from
		`target`'s own position at press time. `bar` and `target` are
		typically the same Flow (a title bar dragging its own panel root) but
		don't have to be. `onChange`, if given, fires once when the drag ends
		(not per delta - see `beginCapture`'s own `onEnd`) - see `pew.ui.macro.
		PanelBuilder`'s own `onChange="..."` attribute, which is what wires
		this for `pew.panel.MeterPanel`. **/
	public static function enableDrag(bar:h2d.Flow, target:h2d.Object, ?onChange:Void->Void):Void {
		bar.enableInteractive = true;
		bar.interactive.cursor = Gfx.cursorMove();
		bar.interactive.onPush = function(e:hxd.Event) {
			var startX = target.x;
			var startY = target.y;
			beginCapture(target, (dx, dy) -> {
				target.x = startX + dx;
				target.y = startY + dy;
			}, onChange);
		};
	}

	/** Makes `handle` a resize grip: on press, starts a capture invoking
		`onDelta(dx, dy)` with the raw drag delta - callers translate that
		into whatever dimensions they're resizing (see
		`pew.ui.macro.PanelBuilder`'s own resize-handle codegen for the
		width/content-height pattern this generalizes). **/
	public static function enableResize(handle:h2d.Flow, onDelta:(dx:Float, dy:Float) -> Void):Void {
		handle.enableInteractive = true;
		handle.interactive.cursor = Gfx.cursorResize();
		handle.interactive.onPush = function(e:hxd.Event) {
			beginCapture(handle, onDelta);
		};
	}
}
