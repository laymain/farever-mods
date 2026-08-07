package imgui.ref;

// See imgui.ref.BoolRef - same persistent-pointer pattern, for Float64-valued widgets (inputDouble).
abstract DoubleRef(hl.Bytes) {
	public inline function new(value:Float = 0) {
		this = new hl.Bytes(8);
		set(value);
	}

	public inline function get():Float
		return this.getF64(0);

	public inline function set(value:Float):Void
		this.setF64(0, value);

	@:to inline function toFloat():Float
		return get();

	@:to inline function toBytes():hl.Bytes
		return this;
}
