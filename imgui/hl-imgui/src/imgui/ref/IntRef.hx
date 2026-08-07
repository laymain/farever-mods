package imgui.ref;

// See imgui.ref.BoolRef - same persistent-pointer pattern, for Int-valued widgets (sliderInt).
abstract IntRef(hl.Bytes) {
	public inline function new(value:Int = 0) {
		this = new hl.Bytes(4);
		set(value);
	}

	public inline function get():Int
		return this.getI32(0);

	public inline function set(value:Int):Void
		this.setI32(0, value);

	@:to inline function toInt():Int
		return get();

	@:to inline function toBytes():hl.Bytes
		return this;
}
