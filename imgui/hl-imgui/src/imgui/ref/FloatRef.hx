package imgui.ref;

// See imgui.ref.BoolRef - same persistent-pointer pattern, for Single-valued widgets (sliderFloat).
abstract FloatRef(hl.Bytes) {
	public inline function new(value:Single = 0) {
		this = new hl.Bytes(4);
		set(value);
	}

	public inline function get():Single
		return this.getF32(0);

	public inline function set(value:Single):Void
		this.setF32(0, value);

	@:to inline function toSingle():Single
		return get();

	@:to inline function toBytes():hl.Bytes
		return this;
}
