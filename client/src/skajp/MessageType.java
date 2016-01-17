package skajp;

import java.nio.charset.StandardCharsets;

public enum MessageType {
	MESSAGE_HELL("HELLOOOO"),
	MESSAGE_CONN("CONNECTT"),
	MESSAGE_TRAN("TRANSMIT"),
	MESSAGE_BYES("BYESSSSS"),
	MESSAGE_OTHER("XXXXXXXX");
	
	private static final int HEADER_LENGTH = 8;
	
	private byte[] bytes;
	
	MessageType(String header) {
		bytes = header.getBytes(StandardCharsets.US_ASCII);
	}
	
	public byte[] getBytes() {
		return bytes;
	}
	
	public static MessageType getMessageType(byte[] buffer) {
		for (MessageType value : MessageType.values()) {
			if (ByteUtils.strncmp(buffer, 0, value.bytes, HEADER_LENGTH) == 0) {
				return value;
			}
		}
		return MessageType.MESSAGE_OTHER;
	}
	
	public static boolean isMessageType(byte[] buffer, MessageType type) {
		return ByteUtils.strncmp(buffer, 0, type.bytes, HEADER_LENGTH) == 0;
	}
}
