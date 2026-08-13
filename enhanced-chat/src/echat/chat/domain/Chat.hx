package echat.chat.domain;

interface Chat {
	function send(text:String, ?channel:st.Channel):Void;
	function availableChannels():Array<ChannelOption>;
	function availableCommands():Array<String>;
	var onMessage:ChatMessage->Void;
}
