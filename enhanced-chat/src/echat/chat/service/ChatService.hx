package echat.chat.service;

import hlx.runtime.Registry;
import hlx.runtime.Bus;
import ui.hud.ChatBox;
import ui.hud.ChatBoxMessage;
import ent.Hero;
import echat.chat.domain.Chat;
import echat.chat.domain.ChatMessage;
import echat.chat.domain.ChannelOption;
import echat.chat.domain.IgnoredPlayer;

class ChatService implements Chat {
	static var active:ChatService;

	public var onMessage:ChatMessage->Void;

	var ignoredIds:Map<String, Bool> = new Map();

	public function new() {
		active = this;
	}

	public function setIgnoredPlayers(list:Array<IgnoredPlayer>):Void {
		ignoredIds = new Map();
		for (p in list)
			ignoredIds.set(p.id, true);
	}

	public function whisperTo(targetId:String, text:String):Void {
		var game = GameApp.get();
		if (game == null || game.me == null || game.layer == null)
			return;
		var target = game.layer.getPlayerById(targetId);
		if (target == null) {
			showLocalMessage("Error", 'Unknown player id "$targetId"');
			return;
		}
		game.me.chatClient.sendMessage(text, st.Channel.Player(target));
	}

	public function handleWhisperCommand(raw:String):Void {
		var space = raw.indexOf(" ");
		var token = space == -1 ? raw : raw.substr(0, space);
		var hashIdx = token.indexOf("#");
		if (hashIdx == -1) {
			showLocalMessage("Error", "Usage: !whisper Name#id message");
			return;
		}
		var message = space == -1 ? "" : raw.substr(space + 1);
		if (message.length == 0) {
			showLocalMessage("Error", "Usage: !whisper Name#id message");
			return;
		}
		whisperTo(token.substr(hashIdx + 1), message);
	}

	public function send(text:String, ?channel:st.Channel):Void {
		if (text == null || text.length == 0)
			return;

		if (text.charAt(0) == "!") {
			dispatchCommand(text);
			return;
		}

		var game = GameApp.get();
		if (game == null || game.me == null)
			return;
		game.me.chatClient.sendMessage(text, channel != null ? channel : st.Channel.Local);
	}

	// Which channels the player can currently send on - Group only shows up once
	// GameApp.get().me.group is actually set, so the panel never offers a channel
	// that would just no-op or error against the real chat client.
	public function availableChannels():Array<ChannelOption> {
		var options:Array<ChannelOption> = [
			{label: "Local", channel: st.Channel.Local},
			{label: "All", channel: st.Channel.All}
		];
		var game = GameApp.get();
		if (game != null && game.me != null && game.me.group != null)
			options.push({label: "Group", channel: st.Channel.Group(game.me.group)});
		return options;
	}

	function dispatchCommand(text:String):Void {
		var space = text.indexOf(" ");
		var name = (space == -1 ? text : text.substr(0, space)).substr(1);
		if (!isRegisteredCommand(name)) {
			showLocalMessage("Error", 'Unknown command "$name"');
			return;
		}
		Bus.publish("command.execute." + name, space == -1 ? "" : text.substr(space + 1));
	}

	function showLocalMessage(channel:String, text:String):Void {
		if (onMessage != null)
			onMessage({
				sender: null,
				senderId: null,
				channel: channel,
				channelColor: "Chat_Error",
				channelPlayerId: null,
				text: text,
				time: timeLabel()
			});
	}

	static function timeLabel():String {
		var d = Date.now();
		var h = d.getHours();
		var m = d.getMinutes();
		return (h < 10 ? "0" : "") + h + ":" + (m < 10 ? "0" : "") + m;
	}

	var registeredCommands:Map<String, Bool>;

	// Registered command names, for the panel's "!"-autocomplete - same cache isRegisteredCommand
	// builds and checks against, so a command is suggestible exactly when it would also dispatch.
	public function availableCommands():Array<String> {
		buildCommandCache();
		return [for (name in registeredCommands.keys()) name];
	}

	function isRegisteredCommand(name:String):Bool {
		buildCommandCache();
		return registeredCommands.exists(name);
	}

	function buildCommandCache():Void {
		if (registeredCommands != null) return;
		registeredCommands = new Map();
		var mods = Registry.list("mods");
		for (mod in mods) {
			// A "commands" array from another mod's Registry.register(..., {commands: [...]})
			// is a plain literal there (element type inferred, not Dynamic), so it's always
			// hl.types.ArrayObj across the module boundary - never hl.types.ArrayDyn, which is
			// what a `var:Array<Dynamic>` read here would require and fail to cast to.
			// ArrayBase is the real common ancestor for that case (see hl's own
			// types/ArrayObj.hx) - same reason the auto-generated gamelib bindings read
			// cross-module arrays this way instead of as Array<Dynamic>.
			var commands:hl.types.ArrayBase = Reflect.field(mod, "commands");
			if (commands == null) continue;
			for (i in 0...commands.length) {
				var cmdName = Reflect.field(commands.getDyn(i), "name");
				registeredCommands.set(cmdName, true);
			}
		}
	}

	@:hlx.postfix(ui.hud.ChatBox.receiveMessage)
	static function afterReceiveMessage(
		instance:ChatBox,
		a0:{args:Dynamic, channel:st.Channel, localStamp:Null<Float>, localTextId:String, notify:String, sender:ent.Unit, text:String},
		result:Void
	):Void {
		if (active == null || active.onMessage == null)
			return;

		var resolved = resolveSender(a0.sender);
		if (resolved.id != null && active.ignoredIds.exists(resolved.id))
			return;

		// A Channel.Player(p) always names "the other side of the conversation" - for an
		// incoming whisper that's the sender again, for our own outgoing echo it's the
		// only place the recipient is recorded at all, so show that name instead of the
		// generic channel label either way.
		var whisperWith = a0.channel.isPlayer() ? (a0.channel.getParams()[0] : st.Player) : null;

		active.onMessage({
			sender: resolved.name,
			senderId: resolved.id,
			channel: whisperWith != null ? whisperWith.name : ChatBoxMessage.getChannelString(a0.channel),
			channelColor: ChatBoxMessage.getChannelColor(a0.channel),
			channelPlayerId: whisperWith != null ? whisperWith.uid : null,
			text: a0.text,
			time: timeLabel()
		});
	}

	// Unit.getName() returns the class name for a Hero - the real name is Hero.player.name.
	// Only a Hero backed by a real st.Player has a stable id (uid) to whisper/ignore by.
	static function resolveSender(sender:ent.Unit):{name:String, id:String} {
		if (sender == null) return {name: null, id: null};
		var hero:Hero = sender;
		var player = hero.player;
		return player != null ? {name: player.name, id: player.uid} : {name: sender.getName(), id: null};
	}

}
