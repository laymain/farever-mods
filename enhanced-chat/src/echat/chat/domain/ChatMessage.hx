package echat.chat.domain;

typedef ChatMessage = {
	var sender:String;
	var senderId:Null<String>;
	var channel:String;
	var channelColor:String;
	var channelPlayerId:Null<String>;
	var text:String;
	var time:String;
}
