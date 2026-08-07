package imgui.ref;

// A persistent, native-backed Bool - what ImGui.checkbox needs to store its checked state across
// frames. hl.Ref<Bool> can't do this (HL's ORef only ever addresses the current call's own stack
// frame, never a field - see ImGui.checkbox's doc comment); a 1-byte hl.Bytes buffer is a genuine
// heap pointer instead, freely storable and stable across calls since HL's GC never moves live
// allocations.
abstract BoolRef(hl.Bytes) {
	public inline function new(value:Bool = false) {
		this = new hl.Bytes(1);
		set(value);
	}

	public inline function get():Bool
		return this.getUI8(0) != 0;

	public inline function set(value:Bool):Void
		this.setUI8(0, value ? 1 : 0);

	@:to inline function toBool():Bool
		return get();

	@:to inline function toBytes():hl.Bytes
		return this;
}
