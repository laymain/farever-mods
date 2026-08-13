package echat;

import hlx.runtime.Registry;
import hlx.runtime.Bus;
import imgui.ImGui;
import echat.chat.domain.ChatTab;
import echat.chat.domain.IgnoredPlayer;
import echat.chat.service.ChatService;
import echat.chat.ui.ChatPanel;
import echat.chat.ui.ChatPanel.ChatPanelState;

typedef EnhancedChatConfig = {
	var chatPanelState:ChatPanelState;
	var tabs:Array<ChatTab>;
	var activeTabId:String;
	var openDMsInNewTab:Bool;
	var ignoredPlayers:Array<IgnoredPlayer>;
}

@:build(hlx.runtime.Mod.build())
class EnhancedChatMod {
	@:hlx.config
	public static var config(default, null):EnhancedChatConfig = {
		chatPanelState: {x: 16, y: 400, width: 480, height: 260, collapsed: false},
		tabs: [{id: "default", name: "All", categories: ["Chat_Local", "Chat_All", "Chat_System", "Chat_Group"]}],
		activeTabId: "default",
		openDMsInNewTab: true,
		ignoredPlayers: []
	};

	static function main():Void {
		var chatService = new ChatService();
		chatService.setIgnoredPlayers(config.ignoredPlayers);
		var panel = new ChatPanel(config.chatPanelState, config.tabs, config.activeTabId, config.openDMsInNewTab, config.ignoredPlayers);

		panel.onStateChanged = state -> {
			config.chatPanelState = state;
			config.save();
		};
		panel.onTabsChanged = (tabs, activeTabId, openDMsInNewTab) -> {
			config.tabs = tabs;
			config.activeTabId = activeTabId;
			config.openDMsInNewTab = openDMsInNewTab;
			config.save();
		};
		panel.onIgnoredChanged = ignoredPlayers -> {
			config.ignoredPlayers = ignoredPlayers;
			config.save();
			chatService.setIgnoredPlayers(ignoredPlayers);
		};
		panel.onSend = (text, channel) -> chatService.send(text, channel);
		panel.onWhisper = (targetId, text) -> chatService.whisperTo(targetId, text);
		chatService.onMessage = panel.addMessage;

		// Refreshed every frame (not once at startup) so joining/leaving a group during
		// play immediately shows up in - or drops out of - the channel selector.
		ImGui.register(HlxRuntime.moduleName(), () -> {
			panel.channelOptions = chatService.availableChannels();
			panel.commandNames = chatService.availableCommands();
			panel.draw();
		});
		Registry.register("mods", "enhanced-chat", {
			name: "Enhanced Chat",
			description: "Replacement chat UI: tabs, channel filtering, keyword filtering, ignore/banning.",
			commands: [
				{name: "whisper", description: "Whisper a player: !whisper Name#id message"}
			]
		});
		Bus.subscribe("command.execute.whisper", (raw:Dynamic) -> chatService.handleWhisperCommand(raw));
	}
}
