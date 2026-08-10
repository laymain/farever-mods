package pew.panel;

import imgui.ImGui;
import imgui.Structs.ImVec2;

typedef Icon = {
	var texId:hl.I64;
	var uv0:ImVec2;
	var uv1:ImVec2;
}

// Resolves a gamelib gfx:{file,x,y,width,height} spritesheet ref into an ImGui texture + UV sub-rect.
class IconCache {
	// Keyed by source file so multiple icons from one spritesheet share a single GPU upload.
	static var atlases = new Map<String, {texId:hl.I64, width:Int, height:Int}>();

	// "_<N>PX"-suffixed atlases: x/y are grid-cell indices (x/width, y/height when width/height are set), not pixel coords; unsuffixed files keep raw pixel/whole-file semantics.
	static var CELL_SIZE_SUFFIX = ~/_(\d+)PX\.[a-zA-Z0-9]+$/;

	// useGrid=false opts a caller out of grid math even on a "_<N>PX" file - some hero-icon refs reuse a skill atlas but need whole-file behavior (RosterTracker passes false).
	public static function resolve(gfx:{file:String, x:Int, y:Int, width:Null<Int>, height:Null<Int>}, useGrid = true):Icon {
		var atlas = load(gfx.file);

		if (useGrid && CELL_SIZE_SUFFIX.match(gfx.file)) {
			var cellSize = Std.parseInt(CELL_SIZE_SUFFIX.matched(1));
			var cellX = gfx.width != null && gfx.width > 0 ? Std.int(gfx.x / gfx.width) : gfx.x;
			var cellY = gfx.height != null && gfx.height > 0 ? Std.int(gfx.y / gfx.height) : gfx.y;
			var x = cellX * cellSize;
			var y = cellY * cellSize;
			return {
				texId: atlas.texId,
				uv0: ImGui.vec2(x / atlas.width, y / atlas.height),
				uv1: ImGui.vec2((x + cellSize) / atlas.width, (y + cellSize) / atlas.height),
			};
		}

		if (gfx.width == null || gfx.height == null)
			return {texId: atlas.texId, uv0: ImGui.vec2(0, 0), uv1: ImGui.vec2(1, 1)};

		return {
			texId: atlas.texId,
			uv0: ImGui.vec2(gfx.x / atlas.width, gfx.y / atlas.height),
			uv1: ImGui.vec2((gfx.x + gfx.width) / atlas.width, (gfx.y + gfx.height) / atlas.height),
		};
	}

	static function load(file:String):{texId:hl.I64, width:Int, height:Int} {
		var atlas = atlases.get(file);
		if (atlas == null) {
			var pixels = hxd.res.Loader.currentInstance.load(file).toImage().getPixels(hxd.PixelFormat.RGBA, null);
			atlas = {
				texId: ImGui.registerTexture(pixels.bytes.b, pixels.width, pixels.height, pixels.stride),
				width: pixels.width,
				height: pixels.height,
			};
			atlases.set(file, atlas);
		}
		return atlas;
	}
}
