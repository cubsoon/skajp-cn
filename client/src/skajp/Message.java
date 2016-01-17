package skajp;

import java.nio.charset.StandardCharsets;

public class Message {

	public static final int TRANSMISSION_SIZE = 512;
	
	public static final int NAME_SIZE = 8;
	
	private MessageType messageType;
	
	private byte[] sender;
	
	private byte[] receiver;
	
	private byte[] transmissionData;
	
	public Message() {
		this.sender = new byte[NAME_SIZE];
		this.receiver = new byte[NAME_SIZE];
		this.transmissionData = new byte[TRANSMISSION_SIZE];
	}

	public MessageType getMessageType() {
		return messageType;
	}

	public void setMessageType(MessageType messageType) {
		this.messageType = messageType;
	}

	public boolean isSenderEqual(String s) {
		return ByteUtils.strncmp(sender, 0, s.substring(0, 8).getBytes(StandardCharsets.US_ASCII), 8) == 0;
	}
	
	public byte[] getSender() {
		return sender;
	}

	public void setSender(String sender) {
		this.sender = sender.substring(0, 8).getBytes(StandardCharsets.US_ASCII);
	}

	public boolean isReceiverEqual(String r) {
		return ByteUtils.strncmp(receiver, 0, r.substring(0, 8).getBytes(StandardCharsets.US_ASCII), 8) == 0;
	}
	
	public byte[] getReceiver() {
		return receiver;
	}

	public void setReceiver(String receiver) {
		this.receiver = receiver.substring(0, 8).getBytes(StandardCharsets.US_ASCII);
	}

	public byte[] getTransmissionData() {
		return transmissionData;
	}
	
	public int toBuffer(byte[] buffer) {
		ByteUtils.strncpy(buffer, 0, messageType.getBytes(), 8);
		switch (messageType) {
			case MESSAGE_HELL:
			case MESSAGE_BYES:
				ByteUtils.strncpy(buffer, 8, sender, 8);
				return 16;
			case MESSAGE_CONN:
				ByteUtils.strncpy(buffer, 8, sender, 8);
				ByteUtils.strncpy(buffer, 16, receiver, 8);
				return 24;
			case MESSAGE_TRAN:
				ByteUtils.strncpy(buffer, 8, sender, 8);
				ByteUtils.strncpy(buffer, 16, receiver, 8);
				ByteUtils.strncpy(buffer, 24, transmissionData, TRANSMISSION_SIZE);
				return 24 + TRANSMISSION_SIZE;
		}
		return 0;
	}
	
	public int fromBuffer(byte[] buffer) {
		messageType = MessageType.getMessageType(buffer);
		switch (messageType) {
			case MESSAGE_HELL:
			case MESSAGE_BYES:
				ByteUtils.strncpy(sender, buffer, 8, 8);
				return 16;
			case MESSAGE_CONN:
				ByteUtils.strncpy(sender, buffer, 8, 8);
				ByteUtils.strncpy(receiver, buffer, 16,  8);
				return 24;
			case MESSAGE_TRAN:
				ByteUtils.strncpy(sender, buffer, 8, 8);
				ByteUtils.strncpy(receiver, buffer, 16, 8);
				ByteUtils.strncpy(transmissionData, buffer, 24, TRANSMISSION_SIZE);
				return 24 + TRANSMISSION_SIZE;
		}
		return 0;	
	}
}
