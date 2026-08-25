package echat.chat.ui;

import imgui.ImGui;
import imgui.Enums.ImGuiCond;
import imgui.Enums.ImGuiWindowFlags;
import imgui.Enums.ImGuiInputTextFlags;
import imgui.Enums.ImGuiHoveredFlags;
import imgui.Enums.ImGuiStyleVar;
import imgui.Enums.ImGuiCol;
import imgui.Enums.ImGuiTableColumnFlags;
import imgui.Enums.ImGuiTabBarFlags;
import imgui.Enums.ImGuiTabItemFlags;
import imgui.Enums.ImGuiMouseButton;
import imgui.Enums.ImGuiChildFlags;
import imgui.Enums.ImGuiFocusedFlags;
import imgui.Enums.ImGuiKey;
import imgui.ref.IntRef;
import imgui.ref.BoolRef;
import imgui.Structs.ImVec4;
import hlx.runtime.HlxPrefixResult;
import hlx.runtime.HlxPrefixControl;
import ui.hud.ChatBox;
import echat.chat.domain.ChatMessage;
import echat.chat.domain.ChannelOption;
import echat.chat.domain.ChatTab;
import echat.chat.domain.IgnoredPlayer;

typedef ChatPanelState = {
	var x:Float;
	var y:Float;
	var width:Int;
	var height:Int;
	var collapsed:Bool;
}

private typedef TabView = {
	var id:String;
	var name:String;
	var categories:Null<Array<String>>;
	var dmSender:Null<String>;
	var playerId:Null<String>;
	var persisted:Bool;
}

private typedef DmTab = {
	var id:String;
	var sender:String;
	var playerId:String;
}

class ChatPanel {
	static inline final MAX_HISTORY = 200;
	static inline final INPUT_BUF_SIZE = 256;
	static inline final RENAME_BUF_SIZE = 64;
	static inline final MAX_SUGGESTIONS = 5;

	static var inputFocused = false;
	static var mouseCaptured = false;
	static var focusRequested = false;
	static var panelActive = false;
	static var chatJustFocused = false;
	static var timeColWidth = -1.0;
	static var channelColWidth = -1.0;
	static var senderColWidth = -1.0;

	static final CATEGORIES:Array<{key:String, label:String}> = [
		{key: "Chat_Local", label: "Local"},
		{key: "Chat_All", label: "All"},
		{key: "Chat_System", label: "System"},
		{key: "Chat_Group", label: "Group"},
		{key: "Chat_Player", label: "Direct Messages"},
	];

	public var onStateChanged:ChatPanelState->Void;
	public var onTabsChanged:(tabs:Array<ChatTab>, activeTabId:String, openDMsInNewTab:Bool)->Void;
	public var onIgnoredChanged:Array<IgnoredPlayer>->Void;
	public var onSend:(text:String, channel:st.Channel)->Void;
	public var onWhisper:(targetId:String, text:String)->Void;
	public var channelOptions:Array<ChannelOption> = [];
	public var commandNames:Array<String> = [];

	var state:ChatPanelState;
	var history:Array<ChatMessage> = [];
	var inputBuf = new hl.Bytes(INPUT_BUF_SIZE);
	var channelIndex = new IntRef();

	var tabs:Array<ChatTab>;
	var activeTabId:String;
	var openDMsInNewTab:Bool;
	var dmTabs:Array<DmTab> = [];
	var unreadTabIds:Map<String, Bool> = new Map();
	var pendingSelectId:String;
	var nextTabSeq = 1;

	var renamingTabId:String;
	var renameBuf = new hl.Bytes(RENAME_BUF_SIZE);
	var configuringTabId:String;
	var categoryWorking:Map<String, BoolRef> = new Map();
	var openDMsRef = new BoolRef();
	var ignoredPlayers:Array<IgnoredPlayer>;

	var pendingRenameTabId:String;
	var pendingConfigureTabId:String;
	var wantsOpenOptions = false;

	public function new(state:ChatPanelState, tabs:Array<ChatTab>, activeTabId:String, openDMsInNewTab:Bool, ignoredPlayers:Array<IgnoredPlayer>) {
		this.state = state;
		this.tabs = tabs;
		this.activeTabId = activeTabId;
		this.openDMsInNewTab = openDMsInNewTab;
		this.pendingSelectId = activeTabId;
		this.ignoredPlayers = ignoredPlayers;
	}

	public function addMessage(entry:ChatMessage):Void {
		history.push(entry);
		if (history.length > MAX_HISTORY)
			history.shift();

		if (openDMsInNewTab && entry.channelColor == "Chat_Player" && entry.sender != null && entry.senderId != null)
			ensureDmTab(entry.sender, entry.senderId);

		for (view in buildTabViews())
			if (view.id != activeTabId && matchesTab(entry, view))
				unreadTabIds.set(view.id, true);
	}

	function ensureDmTab(sender:String, playerId:String):Void {
		for (d in dmTabs)
			if (d.playerId == playerId)
				return;
		dmTabs.push({id: "dm:" + playerId, sender: sender, playerId: playerId});
	}

	function buildTabViews():Array<TabView> {
		var views:Array<TabView> = [for (t in tabs) {id: t.id, name: t.name, categories: t.categories, dmSender: null, playerId: null, persisted: true}];
		for (d in dmTabs)
			views.push({id: d.id, name: d.sender, categories: null, dmSender: d.sender, playerId: d.playerId, persisted: false});
		return views;
	}

	function isIgnored(id:String):Bool {
		for (p in ignoredPlayers)
			if (p.id == id)
				return true;
		return false;
	}

	// Chat_Error is a local command-result message, not a real channel - it has no
	// category to opt into, so every tab always shows it rather than silently eating it.
	function matchesTab(entry:ChatMessage, view:TabView):Bool {
		if (entry.senderId != null && isIgnored(entry.senderId))
			return false;
		if (entry.channelColor == "Chat_Error")
			return true;
		if (view.dmSender != null)
			return entry.channelColor == "Chat_Player" && entry.sender == view.dmSender;
		if (view.categories == null)
			return true;
		return view.categories.indexOf(entry.channelColor) != -1;
	}

	function notifyTabsChanged():Void {
		if (onTabsChanged != null)
			onTabsChanged(tabs, activeTabId, openDMsInNewTab);
	}

	function addIgnoredPlayer(name:String, id:String):Void {
		if (!isIgnored(id)) {
			ignoredPlayers = ignoredPlayers.concat([{id: id, name: name}]);
			notifyIgnoredChanged();
		}
		closeDmTabFor(id);
	}

	function closeDmTabFor(playerId:String):Void {
		var tabId = "dm:" + playerId;
		if (!Lambda.exists(dmTabs, d -> d.playerId == playerId))
			return;
		dmTabs = [for (d in dmTabs) if (d.playerId != playerId) d];
		unreadTabIds.remove(tabId);
		if (activeTabId == tabId) {
			activeTabId = tabs[0].id;
			pendingSelectId = activeTabId;
		}
	}

	function removeIgnoredPlayer(id:String):Void {
		ignoredPlayers = [for (p in ignoredPlayers) if (p.id != id) p];
		notifyIgnoredChanged();
	}

	function notifyIgnoredChanged():Void {
		if (onIgnoredChanged != null)
			onIgnoredChanged(ignoredPlayers);
	}

	public function draw():Void {
		mouseCaptured = false;
		var game = GameApp.get();
		if (game == null) {
			panelActive = false;
			return;
		}

		ImGui.setNextWindowPos(ImGui.vec2(state.x, state.y), ImGuiCond.FirstUseEver);
		ImGui.setNextWindowSize(ImGui.vec2(state.width, state.height), ImGuiCond.FirstUseEver);
		ImGui.setNextWindowCollapsed(state.collapsed, ImGuiCond.FirstUseEver);

		var flags = game.isCursorFree() ? 0 : ImGuiWindowFlags.NoInputs;
		ImGui.pushStyleVar(ImGuiStyleVar.WindowPadding, ImGui.vec2(4, 4));
		var open = ImGui.begin("Chat", null, flags);
		ImGui.popStyleVar();
		// A stray click/hover while the cursor is genuinely locked (isCursorFree() false)
		// must never be able to set the panel active on its own - only two things may:
		// the cursor was already free and the window is legitimately focused, or the
		// native "open chat" action just fired (chatJustFocused), which is the one case
		// meant to free the cursor starting from a locked state.
		var windowFocused = open && ImGui.isWindowFocused(ImGuiFocusedFlags.ChildWindows);
		panelActive = chatJustFocused || (game.isCursorFree() && windowFocused);
		chatJustFocused = false;
		if (open) {
			var views = buildTabViews();
			drawTabBar(views);
			var active = activeView(views);
			unreadTabIds.remove(active.id);
			drawContent(active);
			drawModals();
			mouseCaptured = ImGui.isWindowHovered(ImGuiHoveredFlags.ChildWindows);
		}
		reportState();
		ImGui.end();
	}

	function activeView(views:Array<TabView>):TabView {
		for (view in views)
			if (view.id == activeTabId)
				return view;
		activeTabId = views[0].id;
		pendingSelectId = activeTabId;
		notifyTabsChanged();
		return views[0];
	}

	function drawTabBar(views:Array<TabView>):Void {
		if (!ImGui.beginTabBar("##ChatTabs", ImGuiTabBarFlags.Reorderable))
			return;

		if (ImGui.tabItemButton("+", ImGuiTabItemFlags.Trailing | ImGuiTabItemFlags.NoTooltip))
			addTab();
		if (ImGui.tabItemButton("Options", ImGuiTabItemFlags.Trailing | ImGuiTabItemFlags.NoTooltip))
			wantsOpenOptions = true;

		for (view in views)
			drawTab(view);

		pendingSelectId = null;
		ImGui.endTabBar();

		if (wantsOpenOptions) {
			wantsOpenOptions = false;
			ImGui.openPopup("Chat Options");
		}
		if (pendingRenameTabId != null) {
			renamingTabId = pendingRenameTabId;
			pendingRenameTabId = null;
			for (t in tabs)
				if (t.id == renamingTabId)
					fillBuf(renameBuf, RENAME_BUF_SIZE, t.name);
			ImGui.openPopup("Rename Tab");
		}
		if (pendingConfigureTabId != null) {
			openConfigureModal(pendingConfigureTabId);
			pendingConfigureTabId = null;
		}
	}

	function drawTab(view:TabView):Void {
		var flags = pendingSelectId == view.id ? ImGuiTabItemFlags.SetSelected : 0;
		var unread = view.id != activeTabId && unreadTabIds.exists(view.id);
		var label = (unread ? "* " : "") + view.name + "###" + view.id;
		var selected = ImGui.beginTabItem(label, null, flags);
		if (!view.persisted && ImGui.isItemClicked(ImGuiMouseButton.Middle))
			deleteTab(view);
		if (selected) {
			if (activeTabId != view.id) {
				activeTabId = view.id;
				notifyTabsChanged();
			}
			ImGui.endTabItem();
		}
		drawTabContextMenu(view);
	}

	function drawTabContextMenu(view:TabView):Void {
		if (!ImGui.beginPopupContextItem())
			return;

		if (view.persisted) {
			if (ImGui.menuItem("Rename"))
				pendingRenameTabId = view.id;
			if (ImGui.menuItem("Configure channels..."))
				pendingConfigureTabId = view.id;
			if (ImGui.menuItem("Clear"))
				clearTabHistory(view);
			if (ImGui.menuItem("Delete", null, false, tabs.length > 1))
				deleteTab(view);
		} else {
			if (ImGui.menuItem("Ignore"))
				addIgnoredPlayer(view.name, view.playerId);
			if (ImGui.menuItem("Clear"))
				clearTabHistory(view);
			if (ImGui.menuItem("Close"))
				deleteTab(view);
		}
		ImGui.endPopup();
	}

	// Clears just what this tab shows, not the whole shared history - a message dropped from
	// one tab's view (say the default "All" tab) must stay visible to any other tab whose
	// filter also matches it (e.g. a per-sender DM tab for that same message).
	function clearTabHistory(view:TabView):Void {
		history = [for (m in history) if (!matchesTab(m, view)) m];
	}

	function applyRename():Void {
		var name = @:privateAccess String.fromUTF8(renameBuf);
		if (name.length > 0)
			for (t in tabs)
				if (t.id == renamingTabId)
					t.name = name;
		renamingTabId = null;
		notifyTabsChanged();
	}

	function deleteTab(view:TabView):Void {
		if (view.persisted) {
			if (tabs.length <= 1)
				return;
			tabs = [for (t in tabs) if (t.id != view.id) t];
		} else {
			dmTabs = [for (d in dmTabs) if (d.id != view.id) d];
		}
		unreadTabIds.remove(view.id);
		if (activeTabId == view.id) {
			activeTabId = tabs[0].id;
			pendingSelectId = activeTabId;
		}
		if (view.persisted)
			notifyTabsChanged();
	}

	function addTab():Void {
		var id = "tab-" + (nextTabSeq++);
		while (tabIdTaken(id))
			id = "tab-" + (nextTabSeq++);
		tabs = tabs.concat([{id: id, name: "New Tab", categories: null}]);
		activeTabId = id;
		pendingSelectId = id;
		notifyTabsChanged();
	}

	function tabIdTaken(id:String):Bool {
		for (t in tabs)
			if (t.id == id)
				return true;
		return false;
	}

	function openConfigureModal(tabId:String):Void {
		var target:ChatTab = null;
		for (t in tabs)
			if (t.id == tabId)
				target = t;
		if (target == null)
			return;

		configuringTabId = target.id;
		categoryWorking = new Map();
		for (cat in CATEGORIES) {
			var ref = new BoolRef();
			ref.set(target.categories == null || target.categories.indexOf(cat.key) != -1);
			categoryWorking.set(cat.key, ref);
		}
		ImGui.openPopup("Configure Channels");
	}

	function applyConfigure():Void {
		if (configuringTabId == null)
			return;
		var selected = [for (cat in CATEGORIES) if (categoryWorking.get(cat.key).get()) cat.key];
		var categories = selected.length == CATEGORIES.length ? null : selected;
		for (t in tabs)
			if (t.id == configuringTabId)
				t.categories = categories;
		configuringTabId = null;
		notifyTabsChanged();
	}

	function resetToDefault():Void {
		tabs = [{id: "default", name: "All", categories: ["Chat_Local", "Chat_All", "Chat_System", "Chat_Group"]}];
		activeTabId = "default";
		pendingSelectId = "default";
		openDMsInNewTab = true;
		dmTabs = [];
		unreadTabIds = new Map();
		notifyTabsChanged();
	}

	function drawModals():Void {
		if (ImGui.beginPopupModal("Rename Tab")) {
			ImGui.setNextItemWidth(200);
			var submitted = ImGui.inputText("##RenameTab", renameBuf, RENAME_BUF_SIZE, ImGuiInputTextFlags.EnterReturnsTrue | ImGuiInputTextFlags.AutoSelectAll);
			if (submitted || ImGui.button("Apply")) {
				applyRename();
				ImGui.closeCurrentPopup();
			}
			ImGui.sameLine();
			if (ImGui.button("Cancel")) {
				renamingTabId = null;
				ImGui.closeCurrentPopup();
			}
			ImGui.endPopup();
		}

		if (ImGui.beginPopupModal("Configure Channels")) {
			for (cat in CATEGORIES)
				ImGui.checkbox(cat.label, categoryWorking.get(cat.key));
			if (ImGui.button("Apply")) {
				applyConfigure();
				ImGui.closeCurrentPopup();
			}
			ImGui.sameLine();
			if (ImGui.button("Cancel")) {
				configuringTabId = null;
				ImGui.closeCurrentPopup();
			}
			ImGui.endPopup();
		}

		if (ImGui.beginPopupModal("Chat Options")) {
			openDMsRef.set(openDMsInNewTab);
			if (ImGui.checkbox("Open direct messages in new tab", openDMsRef)) {
				openDMsInNewTab = openDMsRef.get();
				notifyTabsChanged();
			}
			ImGui.separator();
			ImGui.text("Ignored players");
			if (ignoredPlayers.length == 0)
				ImGui.textDisabled("(none)");
			var toRemove:String = null;
			if (ignoredPlayers.length > 0) {
				var visibleRows = Std.int(Math.min(ignoredPlayers.length, 8));
				var listHeight = visibleRows * ImGui.getFrameHeightWithSpacing();
				if (ImGui.beginChild("IgnoredPlayersList", ImGui.vec2(0, listHeight), ImGuiChildFlags.Borders)) {
					for (p in ignoredPlayers) {
						ImGui.text('${p.name}#${p.id}');
						ImGui.sameLine();
						if (ImGui.button('Remove##ignore-${p.id}'))
							toRemove = p.id;
					}
				}
				ImGui.endChild();
			}
			if (toRemove != null)
				removeIgnoredPlayer(toRemove);
			ImGui.separator();
			if (ImGui.button("Reset to default"))
				resetToDefault();
			ImGui.sameLine();
			if (ImGui.button("Close"))
				ImGui.closeCurrentPopup();
			ImGui.endPopup();
		}
	}

	function fillBuf(buf:hl.Bytes, size:Int, s:String):Void {
		var utf8 = @:privateAccess s.toUtf8();
		var len = 0;
		while (len < size - 1 && utf8.getUI8(len) != 0)
			len++;
		buf.blit(0, utf8, 0, len);
		buf.setUI8(len, 0);
	}

	function prefillWhisper(name:String, id:String):Void {
		var existing = @:privateAccess String.fromUTF8(inputBuf);
		fillBuf(inputBuf, INPUT_BUF_SIZE, "!whisper " + name + "#" + id + " " + existing);
		focusRequested = true;
	}

	function attachPlayerInteractions(label:String, columnWidth:Float, playerName:String, playerId:String, popupId:String):Void {
		if (playerId == null)
			return;
		var hovered = ImGui.isItemHovered(ImGuiHoveredFlags.AllowWhenBlockedByActiveItem);
		if (ImGui.calcTextSize(label).x > columnWidth && hovered) {
			ImGui.beginTooltip();
			ImGui.text(playerName);
			ImGui.endTooltip();
		}
		if (hovered && ImGui.isMouseClicked(ImGuiMouseButton.Left))
			prefillWhisper(playerName, playerId);
		if (ImGui.beginPopupContextItem(popupId)) {
			if (ImGui.menuItem("Whisper"))
				prefillWhisper(playerName, playerId);
			if (ImGui.menuItem("Ignore"))
				addIgnoredPlayer(playerName, playerId);
			ImGui.endPopup();
		}
	}

	function drawSenderCell(entry:ChatMessage, row:Int):Void {
		var label = '[${entry.sender}]';
		ImGui.textDisabled(label);
		attachPlayerInteractions(label, senderColWidth, entry.sender, entry.senderId, 'SenderMenu##${row}');
	}

	function drawChannelCell(entry:ChatMessage, row:Int):Void {
		var label = '(${entry.channel})';
		ImGui.textColored(channelColor(entry.channelColor), label);
		attachPlayerInteractions(label, channelColWidth, entry.channel, entry.channelPlayerId, 'ChannelMenu##${row}');
	}

	function drawContent(active:TabView):Void {
		var filtered = [for (m in history) if (matchesTab(m, active)) m];
		var text = @:privateAccess String.fromUTF8(inputBuf);
		var matches = matchingCommands(text);
		if (matches.length > MAX_SUGGESTIONS)
			matches = matches.slice(0, MAX_SUGGESTIONS);

		ImGui.pushStyleVar(ImGuiStyleVar.WindowPadding, ImGui.vec2(8, 6));
		ImGui.pushStyleColor(ImGuiCol.ChildBg, 0);
		if (ImGui.beginChild("ChatHistory", ImGui.vec2(0, -ImGui.getFrameHeightWithSpacing()))) {
			var atBottom = ImGui.getScrollY() >= ImGui.getScrollMaxY() - 1;
			if (timeColWidth < 0) {
				timeColWidth = ImGui.calcTextSize("00:00").x;
				senderColWidth = ImGui.calcTextSize("[XXXXXXXX]").x;
				channelColWidth = senderColWidth;
			}
			ImGui.pushStyleVar(ImGuiStyleVar.CellPadding, ImGui.vec2(4, 1));
			if (ImGui.beginTable("ChatHistoryTable", 4)) {
				ImGui.tableSetupColumn("time", ImGuiTableColumnFlags.WidthFixed, timeColWidth);
				ImGui.tableSetupColumn("channel", ImGuiTableColumnFlags.WidthFixed, channelColWidth);
				ImGui.tableSetupColumn("sender", ImGuiTableColumnFlags.WidthFixed, senderColWidth);
				ImGui.tableSetupColumn("message", ImGuiTableColumnFlags.WidthStretch);

				for (i in 0...filtered.length) {
					var entry = filtered[i];
					ImGui.tableNextRow();
					ImGui.tableNextColumn();
					ImGui.text(entry.time);
					ImGui.tableNextColumn();
					drawChannelCell(entry, i);
					ImGui.tableNextColumn();
					if (entry.sender != null) {
						drawSenderCell(entry, i);
						ImGui.tableNextColumn();
						ImGui.pushTextWrapPos(0.0);
						ImGui.text(entry.text);
						ImGui.popTextWrapPos();
					} else {
						var pos = ImGui.getCursorScreenPos();
						var winPos = ImGui.getWindowPos();
						var winSize = ImGui.getWindowSize();
						ImGui.pushClipRect(pos, ImGui.vec2(winPos.x + winSize.x, pos.y + ImGui.getFrameHeightWithSpacing()), false);
						ImGui.text(entry.text);
						ImGui.popClipRect();
					}
				}
				ImGui.endTable();
			}
			ImGui.popStyleVar();
			if (atBottom)
				ImGui.setScrollHereY(1);
		}
		ImGui.endChild();
		ImGui.popStyleColor();
		ImGui.popStyleVar();

		if (active.playerId != null) {
			ImGui.textDisabled('Whisper to ${active.name}');
			ImGui.sameLine();
		} else if (channelOptions.length > 0) {
			if (channelIndex.get() >= channelOptions.length)
				channelIndex.set(0);
			var labelWidth = 0.0;
			for (opt in channelOptions) labelWidth = Math.max(labelWidth, ImGui.calcTextSize(opt.label).x);
			ImGui.setNextItemWidth(labelWidth + 45);
			if (ImGui.beginCombo("##ChatChannel", channelOptions[channelIndex.get()].label)) {
				for (i in 0...channelOptions.length) {
					var isSelected = i == channelIndex.get();
					if (ImGui.selectable(channelOptions[i].label, isSelected)) {
						channelIndex.set(i);
						focusRequested = true;
					}
					if (isSelected)
						ImGui.setItemDefaultFocus();
				}
				ImGui.endCombo();
			}
			ImGui.sameLine();
		}

		ImGui.setNextItemWidth(-1);
		if (focusRequested) {
			focusRequested = false;
			ImGui.setKeyboardFocusHere();
		}
		var wasTyping = inputFocused;
		var submitted = ImGui.inputTextWithCompletion("##ChatInput", inputBuf, INPUT_BUF_SIZE, ImGuiInputTextFlags.EnterReturnsTrue, () -> {
			if (matches.length > 0)
				ImGui.setCompletionText("!" + matches[0] + " ");
			else if (channelOptions.length > 0)
				channelIndex.set((channelIndex.get() + 1) % channelOptions.length);
		});
		inputFocused = ImGui.isItemActive();

		if (submitted) {
			inputBuf.setUI8(0, 0);
			if (text.length > 0) {
				if (active.playerId != null && text.charAt(0) != "!") {
					if (onWhisper != null)
						onWhisper(active.playerId, text);
				} else if (onSend != null && channelOptions.length > 0) {
					onSend(text, channelOptions[channelIndex.get()].channel);
				}
			}
			ImGui.setWindowFocus(null);
			inputFocused = false;
			return;
		}

		if (wasTyping && ImGui.isKeyPressed(ImGuiKey.Escape, false))
			ImGui.setWindowFocus(null);

		if (matches.length > 0)
			drawSuggestionsAbove(matches);
	}

	static final CHANNEL_COLORS:Map<String, Int> = [
		"Chat_Local" => 0x00D564,
		"Chat_All" => 0xD62D00,
		"Chat_System" => 0xFF950C,
		"Chat_Player" => 0x9600FF,
		"Chat_Group" => 0x005DDC,
		"Chat_Error" => 0xF7352E,
	];

	static function channelColor(className:String):ImVec4 {
		var value = className != null ? CHANNEL_COLORS.get(className) : null;
		if (value == null) return ImGui.vec4(1, 1, 1, 1);
		return ImGui.vec4(((value >> 16) & 0xFF) / 255, ((value >> 8) & 0xFF) / 255, (value & 0xFF) / 255, 1);
	}

	// Only while still typing the command name itself (no space yet) - once there's a space the player has moved on to the command's arguments, and suggestions would just be noise.
	function matchingCommands(text:String):Array<String> {
		if (text.length == 0 || text.charAt(0) != "!" || text.indexOf(" ") != -1)
			return [];
		var prefix = text.substr(1).toLowerCase();
		return [for (name in commandNames) if (name.toLowerCase().indexOf(prefix) == 0) name];
	}

	function drawSuggestionsAbove(matches:Array<String>):Void {
		var min = ImGui.getItemRectMin();
		ImGui.setNextWindowPos(ImGui.vec2(min.x, min.y), ImGuiCond.Always, ImGui.vec2(0, 1));
		var flags = ImGuiWindowFlags.NoTitleBar | ImGuiWindowFlags.NoResize | ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoFocusOnAppearing | ImGuiWindowFlags.NoSavedSettings | ImGuiWindowFlags.NoInputs | ImGuiWindowFlags.AlwaysAutoResize;
		if (ImGui.begin("##ChatSuggestions", null, flags)) {
			for (name in matches) ImGui.text("!" + name);
		}
		ImGui.end();
	}

	function reportState():Void {
		if (onStateChanged == null) return;

		var pos = ImGui.getWindowPos();
		var size = ImGui.getWindowSize();
		var next:ChatPanelState = {
			x: pos.x,
			y: pos.y,
			width: Std.int(size.x),
			height: Std.int(size.y),
			collapsed: ImGui.isWindowCollapsed(),
		};
		if (next.x == state.x && next.y == state.y && next.width == state.width && next.height == state.height && next.collapsed == state.collapsed)
			return;

		state = next;
		onStateChanged(next);
	}

	@:hlx.prefix(client.UnitController.isInputBlocked)
	static function beforeIsInputBlocked(instance:client.UnitController):HlxPrefixResult<Bool> {
		return ChatPanel.inputFocused ? SkipWith(true) : Continue;
	}

	@:hlx.prefix(h2d.Scene.handleEvent)
	static function beforeSceneHandleEvent(instance:h2d.Scene, event:hxd.Event, prev:Dynamic):HlxPrefixResult<Dynamic> {
		return ChatPanel.mouseCaptured ? SkipWith(null) : Continue;
	}

	@:hlx.prefix(client.BaseCamera.onEvent)
	static function beforeBaseCameraOnEvent(instance:client.BaseCamera, event:hxd.Event):HlxPrefixControl {
		return ChatPanel.mouseCaptured ? Skip : Continue;
	}

	@:hlx.prefix(ui.Hud.shouldFreeCursor)
	static function beforeHudShouldFreeCursor(instance:ui.Hud):HlxPrefixResult<Bool> {
		return ChatPanel.panelActive ? SkipWith(true) : Continue;
	}

	@:hlx.prefix(lib.Input.allBlocked)
	static function beforeInputAllBlocked():HlxPrefixResult<Bool> {
		return ChatPanel.inputFocused ? SkipWith(true) : Continue;
	}

	@:hlx.prefix(ui.BaseUI.isBlockingAllInputs)
	static function beforeUIBlockingAllInputs(instance:ui.BaseUI):HlxPrefixResult<Bool> {
		return ChatPanel.inputFocused ? SkipWith(true) : Continue;
	}

	@:hlx.prefix(ui.hud.ChatBox.focus)
	static function beforeChatBoxFocus(instance:ChatBox):HlxPrefixControl {
		focusRequested = true;
		chatJustFocused = true;
		return Skip;
	}

	static var chatBox:ChatBox;

	@:hlx.postfix(ui.hud.ChatBox.init)
	static function afterChatBoxInit(instance:ChatBox, result:Void):Void {
		chatBox = instance;
		instance.set_visible(false);
	}

	@:hlx.postfix(GameApp.update)
	static function afterGameAppUpdate(instance:GameApp, dt:Float, result:Void):Void {
		if (chatBox != null) chatBox.set_visible(false);
	}
}
