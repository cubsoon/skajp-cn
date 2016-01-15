package skajp;

import java.io.OutputStream;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFormat.Encoding;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.TargetDataLine;

public class OutputMessageHandler implements Runnable {

	private TargetDataLine dataLine;

	private OutputStream outputStream;

	private final AtomicBoolean running;

	private String sender;

	private String receiver;

	private byte[] buffer;

	public OutputMessageHandler(OutputStream outputStream, String sender, String receiver) throws LineUnavailableException {
		this.outputStream = outputStream;
		this.sender = sender;
		this.receiver = receiver;
		
		this.running = new AtomicBoolean(true);
		this.buffer = new byte[SwingApplication.BUFFER_SIZE];

		DataLine.Info info = new DataLine.Info(
				TargetDataLine.class, 
				new AudioFormat(Encoding.PCM_SIGNED, 8000, 8, 1, 1, 8000, false)
				);

		this.dataLine = (TargetDataLine) AudioSystem.getLine(info);

	}

	@Override
	public void run() {
		this.running.set(true);

		Message outputMessage = new Message();
		outputMessage.setMessageType(MessageType.MESSAGE_TRAN);
		outputMessage.setSender(sender);
		outputMessage.setReceiver(receiver);
		try {
			dataLine.open();
			dataLine.start();

			while (this.running.get()) {
				this.dataLine.read(outputMessage.getTransmissionData(), 0, 512);
				int i = outputMessage.toBuffer(buffer);
				outputStream.write(buffer, 0, i);
			} 
		}
		catch (Exception ex) {
			SwingApplication.outputException(ex);
		}
		finally {
			dataLine.close();
		}

	}

	public AtomicBoolean getRunning() {
		return running;
	}

}
