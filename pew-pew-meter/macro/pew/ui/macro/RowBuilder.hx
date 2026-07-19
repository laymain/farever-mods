package pew.ui.macro;

import haxe.macro.Context;
import haxe.macro.Expr;
import haxe.xml.Access in XmlAccess;

/**
	@:build macro that turns a flat row XML descriptor into typed fields + a
	constructor - the same XML-driven approach as `pew.ui.macro.PanelBuilder`, but
	for a single pooled `h2d.Flow` row with no panel chrome (no title bar,
	drag, resize, or collapse), generalized from `pew.panel.MeterRow`'s original
	hand-written constructor.

	## Schema

	```xml
	<row [layout="vertical|horizontal"] [horizontalSpacing="N"] [verticalSpacing="N"]
	     [verticalAlign="top|middle|bottom"]>
	    <bar id="..." color="0xRRGGBB" height="N"/>   <!-- 0+ -->
	    <text id="..." [color="0xRRGGBB"]/>            <!-- 0+ -->
	</row>
	```

	A `<bar>` becomes an `h2d.Bitmap` field backed by `pew.ui.Gfx.solidTile` at
	the given color, 1px wide and `height`px tall, with `width`/`height` both
	pinned explicitly on the object itself (not just the tile) - see
	`pew.panel.MeterRow`'s own doc comment for why leaving `height` null infers it
	from the tile's aspect ratio instead. A `<text>` becomes an `h2d.Text`
	field using `pew.ui.Gfx.defaultFont()`; neither element sets initial text/
	width, since both are mutated every frame by the consuming class's own
	hand-written `update()` (merged in via `Context.getBuildFields()` the same
	way `PanelBuilder` merges `MeterPanel`'s `refresh()`).

	Generated fields are all default access except `container` (the row's own
	root Flow, `public`, matching `PanelBuilder`'s own `root`).
**/
class RowBuilder {
	public static function build(xmlPath:String):Array<Field> {
		var pos = Context.currentPos();
		var path = Context.resolvePath(xmlPath);
		Context.registerModuleDependency(Context.getLocalModule(), path);

		var xmlRoot = new XmlAccess(Xml.parse(sys.io.File.getContent(path)).firstElement());

		var layout = xmlRoot.has.layout && xmlRoot.att.layout == "vertical"
			? (macro h2d.FlowLayout.Vertical)
			: (macro h2d.FlowLayout.Horizontal);
		var horizontalSpacing = xmlRoot.has.horizontalSpacing ? Std.parseInt(xmlRoot.att.horizontalSpacing) : 0;
		var verticalSpacing = xmlRoot.has.verticalSpacing ? Std.parseInt(xmlRoot.att.verticalSpacing) : 0;
		var verticalAlign = xmlRoot.has.verticalAlign ? parseAlign(xmlRoot.att.verticalAlign, pos) : null;

		var fields = Context.getBuildFields();
		fields.push({name: "container", pos: pos, access: [APublic], kind: FVar(macro : h2d.Flow, null)});

		var ctorBody:Array<Expr> = [
			macro container = new h2d.Flow(parent),
			macro container.layout = $layout,
			macro container.horizontalSpacing = $v{horizontalSpacing},
			macro container.verticalSpacing = $v{verticalSpacing},
		];
		if (verticalAlign != null) ctorBody.push(macro container.verticalAlign = $verticalAlign);

		for (child in xmlRoot.elements) {
			if (!child.has.id) {
				Context.error('<${child.name}> is missing an id attribute', pos);
				continue;
			}
			var id = child.att.id;
			switch (child.name) {
				case "bar":
					var color = Std.parseInt(child.att.color);
					var height = Std.parseInt(child.att.height);
					fields.push({name: id, pos: pos, kind: FVar(macro : h2d.Bitmap, null)});
					ctorBody.push(macro $i{id} = new h2d.Bitmap(pew.ui.Gfx.solidTile($v{color}, 1, $v{height}), container));
					ctorBody.push(macro $i{id}.width = 1);
					ctorBody.push(macro $i{id}.height = $v{height});
				case "text":
					var color = child.has.color ? Std.parseInt(child.att.color) : 0xFFFFFF;
					fields.push({name: id, pos: pos, kind: FVar(macro : h2d.Text, null)});
					ctorBody.push(macro $i{id} = new h2d.Text(pew.ui.Gfx.defaultFont(), container));
					ctorBody.push(macro $i{id}.textColor = $v{color});
				default:
					Context.error('Unknown UI element <${child.name}> inside <row> - only <bar> and <text> are allowed', pos);
			}
		}

		fields.push({
			name: "new",
			pos: pos,
			access: [APublic],
			kind: FFun({
				args: [{name: "parent", type: macro : h2d.Flow}],
				ret: null,
				expr: macro $b{ctorBody},
			}),
		});

		return fields;
	}

	static function parseAlign(value:String, pos:Position):Expr {
		return switch (value) {
			case "top": macro h2d.FlowAlign.Top;
			case "middle": macro h2d.FlowAlign.Middle;
			case "bottom": macro h2d.FlowAlign.Bottom;
			default:
				Context.error('Unknown verticalAlign "$value"', pos);
				macro h2d.FlowAlign.Top;
		}
	}
}
