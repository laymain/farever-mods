package pew.ui.macro;

import haxe.macro.Context;
import haxe.macro.Expr;
import haxe.xml.Access in XmlAccess;

/**
	@:build macro that turns an XML panel descriptor into typed fields + a
	constructor on the annotated class, targeting `farever-gamelib` types
	(via `pew.ui.Gfx`/`pew.ui.Button`/`pew.ui.Drag`) instead of real `-lib heaps`. Copied
	back in from the shared `hlx-ui-kit` library as pew-pew-meter's own local
	copy - see `pew.ui.Gfx`'s own doc comment for why, and `hlx-ui-kit-poc-mod/
	README.md` for the original record of what changed vs. the very first
	`-lib heaps` prototype (`hlx-ui-kit-poc/src/ui/macro/PanelBuilder.hx`):

	- every `h2d.*` constructor call passes its real parent explicitly -
	  farever-gamelib's constructor wrappers are recovered from one real call
	  site each and are NOT optional-arg, unlike real Heaps'
	  `new h2d.Text(font, ?parent)`.
	- `h2d.FlowLayout`/`h2d.FlowOverflow` are referenced directly
	  (`h2d.FlowLayout.Vertical`), not through `h2d.Flow.FlowLayout.Vertical`.
	- there's no gamelib/mod-writable composition widget class to instantiate
	  for `<button>` (mods can't extend `h2d.Object`) - built inline via
	  `pew.ui.Button.create`.
	- click wiring binds the real generated `h2d.Interactive.onClick`
	  signature, `(hxd.Event) -> Void`, directly.

	## Schema

	Generalized from the real, live-confirmed chrome in `pew.panel.MeterPanel`
	(the first real panel converted to this, replacing its own hand-written
	constructor/buildTitleBar/buildResizeHandle/enableResize/toggleCollapse) -
	not just the earlier `hlx-ui-kit-poc-mod` demo panel's simpler shape. See
	`pew.ui.macro.RowBuilder` for the equivalent, simpler XML-driven codegen for a
	single pooled row (`pew.panel.MeterRow`) with no panel chrome.

	```xml
	<panel title="..." width="N" [maxWidth="N"] [x="N"] [y="N"] [scale="F"]
	       [background="0xRRGGBB"] [alpha="F"] [padding="N"] [verticalSpacing="N"]
	       [draggable="true"] [resizable="true"] [collapsible="true"] [collapsed="true"]
	       [onChange="methodName"]>
	    <titlebar>                          <!-- optional -->
	        <button id="..." text="..." onClick="..."/>   <!-- 0+ -->
	    </titlebar>
	    <content id="..." [layout="vertical|horizontal"] [verticalSpacing="N"]
	             [minHeight="N"] [maxContentHeight="N"] [overflow="expand|limit|hidden|scroll"]>
	        <text id="..." text="..." [muted="true"]/>      <!-- 0+, optional -->
	        <button id="..." text="..." [onClick="..."]/>    <!-- 0+, optional -->
	    </content>
	</panel>
	```

	`<content>` is required exactly once: it's the one child Flow a mod's own
	hand-written code (merged in via `Context.getBuildFields()`) can keep
	appending dynamic children to at runtime (e.g. `MeterPanel.refresh()`
	pushing pooled `MeterRow`s) - static `<text>`/`<button>` children inside it
	are optional and are just the degenerate "nothing dynamic" case.
	`resizable="true"` requires `<content minHeight="...">` (the resize floor
	for the content area's height) - `width` (the panel's resize floor) is
	always required.

	Only `<button>` is currently supported inside `<titlebar>`.

	`onChange="methodName"` names a zero-arg `Void->Void` method (same
	XML-attribute-names-a-method convention `onClick` already uses for
	buttons) - the one hook point a consuming class needs to notice "something
	moved/resized/collapsed" and react (e.g. `MeterPanel.onStateMutated`,
	which turns it into a typed `onStateChanged:MeterPanelState->Void`
	callback). Omit it and none of that wiring is generated at all. Fires once
	when a drag or resize *gesture finishes* (on release, via `pew.ui.Drag.
	beginCapture`'s `onEnd`) - not per delta, since a single drag/resize can
	produce dozens of deltas a caller has no reason to react to individually -
	but immediately on every collapse toggle, which is already a single
	discrete click with no intermediate deltas to coalesce.

	Generated fields are un-exported (default/private access) except `root`
	and `<content>`'s own `id` - the only two a consuming mod's hand-written
	code needs to reach directly. `collapsed`, `savedMinWidth`/`savedMaxWidth`,
	and the rest of the collapse bookkeeping stay private - a consumer needing
	to read or persist them goes through `onChange`/dedicated accessor methods
	on the annotated class instead (see `pew.panel.MeterPanel.getState`/
	`loadState`), not the raw fields.
**/
class PanelBuilder {
	public static function build(xmlPath:String):Array<Field> {
		var pos = Context.currentPos();
		var path = Context.resolvePath(xmlPath);
		Context.registerModuleDependency(Context.getLocalModule(), path);

		var xmlRoot = new XmlAccess(Xml.parse(sys.io.File.getContent(path)).firstElement());

		var title = xmlRoot.att.title;
		var width = Std.parseInt(xmlRoot.att.width);
		var maxWidth = xmlRoot.has.maxWidth ? Std.parseInt(xmlRoot.att.maxWidth) : width;
		var px = xmlRoot.has.x ? Std.parseFloat(xmlRoot.att.x) : 0.0;
		var py = xmlRoot.has.y ? Std.parseFloat(xmlRoot.att.y) : 0.0;
		var scale = xmlRoot.has.scale ? Std.parseFloat(xmlRoot.att.scale) : 1.0;
		var background = xmlRoot.has.background ? Std.parseInt(xmlRoot.att.background) : 0x1a1a1a;
		var alpha = xmlRoot.has.alpha ? Std.parseFloat(xmlRoot.att.alpha) : 0.92;
		var padding = xmlRoot.has.padding ? Std.parseInt(xmlRoot.att.padding) : 8;
		var verticalSpacing = xmlRoot.has.verticalSpacing ? Std.parseInt(xmlRoot.att.verticalSpacing) : 6;
		var draggable = xmlRoot.has.draggable && xmlRoot.att.draggable == "true";
		var resizable = xmlRoot.has.resizable && xmlRoot.att.resizable == "true";
		var collapsible = xmlRoot.has.collapsible && xmlRoot.att.collapsible == "true";
		var startCollapsed = xmlRoot.has.collapsed && xmlRoot.att.collapsed == "true";
		var onChangeMethod = xmlRoot.has.onChange ? xmlRoot.att.onChange : null;
		var onChangeExpr:Expr = onChangeMethod != null ? (macro $i{onChangeMethod}) : (macro null);

		var titlebarNode:XmlAccess = null;
		var contentNode:XmlAccess = null;
		for (node in xmlRoot.elements) {
			switch (node.name) {
				case "titlebar": titlebarNode = node;
				case "content": contentNode = node;
				default: Context.error('Unknown top-level <panel> child <${node.name}> - only <titlebar> and <content> are allowed', pos);
			}
		}
		if (contentNode == null) {
			Context.error('<panel> requires exactly one <content id="..."/> child', pos);
			return Context.getBuildFields();
		}
		if (!contentNode.has.id) {
			Context.error('<content> is missing an id attribute', pos);
			return Context.getBuildFields();
		}
		var contentId = contentNode.att.id;
		var contentLayout = contentNode.has.layout && contentNode.att.layout == "horizontal"
			? (macro h2d.FlowLayout.Horizontal)
			: (macro h2d.FlowLayout.Vertical);
		var contentVSpacing = contentNode.has.verticalSpacing ? Std.parseInt(contentNode.att.verticalSpacing) : 0;
		var contentMinHeight = contentNode.has.minHeight ? Std.parseInt(contentNode.att.minHeight) : null;
		var contentMaxResizeHeight = contentNode.has.maxContentHeight ? Std.parseInt(contentNode.att.maxContentHeight) : contentMinHeight;
		var contentOverflow = contentNode.has.overflow ? parseOverflow(contentNode.att.overflow, pos) : null;
		if (resizable && contentMinHeight == null) {
			Context.error('resizable="true" requires <content minHeight="..."/> (the resize floor for the content area)', pos);
			return Context.getBuildFields();
		}
		var hasTitleBarButtons = titlebarNode != null;

		var fields = Context.getBuildFields();

		fields.push({name: "root", pos: pos, access: [APublic], kind: FVar(macro : h2d.Flow, null)});
		fields.push({name: "titleBar", pos: pos, kind: FVar(macro : h2d.Flow, null)});
		if (hasTitleBarButtons) fields.push({name: "actionsGroup", pos: pos, kind: FVar(macro : h2d.Flow, null)});
		if (resizable) fields.push({name: "resizeRow", pos: pos, kind: FVar(macro : h2d.Flow, null)});
		if (collapsible) {
			fields.push({name: "collapsed", pos: pos, kind: FVar(macro : Bool, macro false)});
			fields.push({name: "collapseLabel", pos: pos, kind: FVar(macro : h2d.Text, null)});
			fields.push({name: "savedMinWidth", pos: pos, kind: FVar(macro : Null<Int>, null)});
			fields.push({name: "savedMaxWidth", pos: pos, kind: FVar(macro : Null<Int>, null)});
		}
		fields.push({name: contentId, pos: pos, access: [APublic], kind: FVar(macro : h2d.Flow, null)});

		var ctorBody:Array<Expr> = [
			macro root = new h2d.Flow(parent),
			macro root.layout = h2d.FlowLayout.Vertical,
			macro root.set_padding($v{padding}),
			macro root.verticalSpacing = $v{verticalSpacing},
			macro root.fillWidth = false,
			macro root.fillHeight = false,
			macro if (containerIsManagedFlow) parent.getProperties(root).isAbsolute = true,
			macro root.overflow = h2d.FlowOverflow.Limit,
			macro root.minWidth = $v{width},
			macro root.maxWidth = $v{width},
			macro root.backgroundTile = pew.ui.Gfx.solidTile($v{background}, 1, 1, $v{alpha}),
			macro root.setScale($v{scale}),
			macro root.x = $v{px},
			macro root.y = $v{py},
		];

		// -- title bar --
		ctorBody.push(macro titleBar = new h2d.Flow(root));
		ctorBody.push(macro titleBar.layout = h2d.FlowLayout.Horizontal);
		ctorBody.push(macro titleBar.verticalAlign = h2d.FlowAlign.Middle);
		ctorBody.push(macro titleBar.horizontalSpacing = 8);
		ctorBody.push(macro titleBar.fillWidth = true);
		ctorBody.push(macro titleBar.backgroundTile = pew.ui.Gfx.solidTile(0x2a2a2a, 1, 1, 1.0));
		ctorBody.push(macro var __titleText = new h2d.Text(pew.ui.Gfx.defaultFont(), titleBar));
		ctorBody.push(macro __titleText.text = $v{title});
		ctorBody.push(macro __titleText.textColor = 0xFFFFFF);

		if (collapsible) {
			ctorBody.push(macro var __collapseCharWidth = Math.max(__titleText.calcTextWidth("+"), __titleText.calcTextWidth("-")));
			ctorBody.push(macro pew.ui.Button.create(titleBar, collapsed ? "+" : "-", e -> toggleCollapse(), 4, __t -> collapseLabel = __t, __collapseCharWidth));
		}

		if (hasTitleBarButtons) {
			ctorBody.push(macro actionsGroup = new h2d.Flow(titleBar));
			ctorBody.push(macro actionsGroup.layout = h2d.FlowLayout.Horizontal);
			for (btn in titlebarNode.elements) {
				if (btn.name != "button") {
					Context.error('<titlebar> only supports <button> children, got <${btn.name}>', pos);
					continue;
				}
				ctorBody.push(buttonCreateExpr(macro actionsGroup, btn));
			}
			ctorBody.push(macro titleBar.getProperties(actionsGroup).horizontalAlign = h2d.FlowAlign.Right);
		}

		if (draggable) ctorBody.push(macro pew.ui.Drag.enableDrag(titleBar, root, $onChangeExpr));

		// -- content --
		ctorBody.push(macro $i{contentId} = new h2d.Flow(root));
		ctorBody.push(macro $i{contentId}.layout = $contentLayout);
		ctorBody.push(macro $i{contentId}.verticalSpacing = $v{contentVSpacing});
		ctorBody.push(macro $i{contentId}.fillWidth = true);
		if (contentOverflow != null) ctorBody.push(macro $i{contentId}.overflow = $contentOverflow);
		if (contentMinHeight != null) {
			ctorBody.push(macro $i{contentId}.minHeight = $v{contentMinHeight});
			ctorBody.push(macro $i{contentId}.maxHeight = $v{contentMinHeight});
		}
		for (child in contentNode.elements) {
			if (!child.has.id) {
				Context.error('<${child.name}> is missing an id attribute', pos);
				continue;
			}
			switch (child.name) {
				case "text": pushText(fields, ctorBody, macro $i{contentId}, child, pos);
				case "button": pushButtonField(fields, ctorBody, macro $i{contentId}, child, pos);
				default: Context.error('Unknown UI element <${child.name}> inside <content>', pos);
			}
		}

		// -- resize handle --
		if (resizable) {
			var gripSize = 10.0;
			ctorBody.push(macro resizeRow = new h2d.Flow(root));
			ctorBody.push(macro resizeRow.layout = h2d.FlowLayout.Horizontal);
			ctorBody.push(macro resizeRow.fillWidth = true);
			ctorBody.push(macro var __resizeHandle = new h2d.Flow(resizeRow));
			ctorBody.push(macro __resizeHandle.set_padding(2));
			ctorBody.push(macro var __resizeGrip = new h2d.Graphics(__resizeHandle));
			ctorBody.push(macro var __gripColor = 0xAAAAAA);
			ctorBody.push(macro var __gripAlpha = 1.0);
			ctorBody.push(macro var __gripSegments = 0);
			ctorBody.push(macro __resizeGrip.beginFill(__gripColor, __gripAlpha));
			ctorBody.push(macro for (__gripRow in 0...3) for (__gripCol in 0...(3 - __gripRow)) __resizeGrip.drawCircle($v{gripSize} - 4 - __gripRow * 5, $v{gripSize} - 4 - __gripCol * 5, 1.3, __gripSegments));
			ctorBody.push(macro __resizeGrip.endFill());
			ctorBody.push(macro resizeRow.getProperties(__resizeHandle).horizontalAlign = h2d.FlowAlign.Right);
			ctorBody.push(macro __resizeHandle.enableInteractive = true);
			ctorBody.push(macro __resizeHandle.interactive.cursor = pew.ui.Gfx.cursorResize());

			var resizeDeltaBody:Array<Expr> = [];
			resizeDeltaBody.push(macro var w = Std.int(Math.max($v{width}, Math.min($v{maxWidth}, __startWidth + dx))));
			resizeDeltaBody.push(macro root.minWidth = w);
			resizeDeltaBody.push(macro root.maxWidth = w);
			resizeDeltaBody.push(macro var h = Std.int(Math.max($v{contentMinHeight}, Math.min($v{contentMaxResizeHeight}, __startContentHeight + dy))));
			resizeDeltaBody.push(macro $i{contentId}.minHeight = h);
			resizeDeltaBody.push(macro $i{contentId}.maxHeight = h);

			ctorBody.push(macro __resizeHandle.interactive.onPush = function(e:hxd.Event) {
				var __startWidth = root.get_outerWidth();
				var __startContentHeight = $i{contentId}.get_outerHeight();
				pew.ui.Drag.beginCapture(__resizeHandle, (dx, dy) -> $b{resizeDeltaBody}, $onChangeExpr);
			});
		}

		if (collapsible) {
			var toggleBody:Array<Expr> = [
				macro collapsed = !collapsed,
				macro $i{contentId}.visible = !collapsed,
			];
			if (resizable) toggleBody.push(macro resizeRow.visible = !collapsed);
			if (hasTitleBarButtons) toggleBody.push(macro actionsGroup.visible = !collapsed);
			toggleBody.push(macro if (collapseLabel != null) collapseLabel.text = collapsed ? "+" : "-");
			toggleBody.push(macro if (collapsed) {
				savedMinWidth = root.minWidth;
				savedMaxWidth = root.maxWidth;
				root.minWidth = null;
				root.maxWidth = null;
			} else {
				root.minWidth = savedMinWidth;
				root.maxWidth = savedMaxWidth;
			});
			if (onChangeMethod != null) toggleBody.push(macro $i{onChangeMethod}());

			fields.push({
				name: "toggleCollapse",
				pos: pos,
				access: [APublic],
				kind: FFun({args: [], ret: macro : Void, expr: macro $b{toggleBody}}),
			});

			if (startCollapsed) ctorBody.push(macro toggleCollapse());
		}

		fields.push({
			name: "new",
			pos: pos,
			access: [APublic],
			kind: FFun({
				args: [
					{name: "parent", type: macro : h2d.Flow},
					{name: "containerIsManagedFlow", type: macro : Bool, value: macro false},
				],
				ret: null,
				expr: macro $b{ctorBody},
			}),
		});

		return fields;
	}

	static function parseOverflow(value:String, pos:Position):Expr {
		return switch (value) {
			case "expand": macro h2d.FlowOverflow.Expand;
			case "limit": macro h2d.FlowOverflow.Limit;
			case "hidden": macro h2d.FlowOverflow.Hidden;
			case "scroll": macro h2d.FlowOverflow.Scroll;
			default:
				Context.error('Unknown overflow "$value"', pos);
				macro h2d.FlowOverflow.Expand;
		}
	}

	static function pushText(fields:Array<Field>, ctorBody:Array<Expr>, parent:Expr, node:XmlAccess, pos:Position):Void {
		var id = node.att.id;
		var text = node.att.text;
		var muted = node.has.muted && node.att.muted == "true";
		var color = muted ? 0xAAAAAA : 0xFFFFFF;
		fields.push({name: id, pos: pos, access: [APublic], kind: FVar(macro : h2d.Text, null)});
		ctorBody.push(macro $i{id} = new h2d.Text(pew.ui.Gfx.defaultFont(), $parent));
		ctorBody.push(macro $i{id}.text = $v{text});
		ctorBody.push(macro $i{id}.textColor = $v{color});
	}

	static function pushButtonField(fields:Array<Field>, ctorBody:Array<Expr>, parent:Expr, node:XmlAccess, pos:Position):Void {
		var id = node.att.id;
		fields.push({name: id, pos: pos, access: [APublic], kind: FVar(macro : h2d.Flow, null)});
		var call = buttonCreateExpr(parent, node);
		ctorBody.push(macro $i{id} = $call);
	}

	static function buttonCreateExpr(parent:Expr, node:XmlAccess):Expr {
		var text = node.att.text;
		var onClickExpr = node.has.onClick ? (macro $i{node.att.onClick}) : (macro function(e:hxd.Event) {});
		return macro pew.ui.Button.create($parent, $v{text}, $onClickExpr);
	}
}
