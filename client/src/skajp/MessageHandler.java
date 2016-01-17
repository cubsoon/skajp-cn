package skajp;

import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.SourceDataLine;
import javax.sound.sampled.AudioFormat.Encoding;
import javax.sound.sampled.AudioSystem;
import javax.swing.JOptionPane;

public class MessageHandler implements Runnable {

	private final AtomicBoolean running ;

	private byte[] buffer;

	private InputStream inputStream;

	private SourceDataLine dataLine;

	private SwingApplication application;
	
	private Play playRunnable;
	
	private boolean playing;
	
	private Thread playThread;

	public MessageHandler(SwingApplication application, InputStream inputStream) throws LineUnavailableException {
		this.application = application;
		this.inputStream = inputStream;
		this.running = new AtomicBoolean(true);
		this.buffer = new byte[SwingApplication.BUFFER_SIZE];

		DataLine.Info info = new DataLine.Info(
				SourceDataLine.class, 
				new AudioFormat(Encoding.PCM_SIGNED, 8000, 8, 1, 1, 8000, false)
				);

		this.dataLine = (SourceDataLine) AudioSystem.getLine(info);
	}

	@Override
	public void run() {
		this.running.set(true);
		Message inputMessage = new Message();

		try {			
			dataLine.open();
			dataLine.start();
			playing = false;
			playRunnable = new Play(dataLine);
			playThread = new Thread(playRunnable);
			
			while(this.running.get()) {
				inputStream.read(buffer, 0, SwingApplication.BUFFER_SIZE);
				inputMessage.fromBuffer(buffer);
				ApplicationState state = application.getApplicationState();

				switch (inputMessage.getMessageType()) {
				case MESSAGE_CONN:
					if (ApplicationState.STATE_CONNECTED.equals(state)) {
						String sender = new String(inputMessage.getSender(), StandardCharsets.US_ASCII);
						int choice = JOptionPane.showConfirmDialog(
								null, 
								String.format("%s is calling! Receive?", sender),
								"INCOMING CALL",
								JOptionPane.YES_NO_OPTION
								);
						if (choice == JOptionPane.YES_OPTION) {
							application.setOutputHandler(sender);
							application.setApplicationState(ApplicationState.STATE_TRANSMISSION);
						}
					}
					break;
				case MESSAGE_TRAN:
					if (ApplicationState.STATE_TRANSMISSION.equals(state)) {
						handleTransmission(inputMessage);
					}
					break;
				case MESSAGE_BYES:
					this.running.set(false);
					break;
				default:
					continue;
				}
			}
		}
		catch (Exception ex) {
			SwingApplication.outputException(ex);
			this.running.set(false);
		}
		finally {
			dataLine.close();
		}

		application.setApplicationState(ApplicationState.STATE_IDLE);
	}

	public AtomicBoolean getRunning() {
		return running;
	}

	public void setInputStream(InputStream inputStream) {
		this.inputStream = inputStream;
	}

	private void handleTransmission(Message inputMessage) {
		dataLine.write(inputMessage.getTransmissionData(), 0, 512);
		if (!playing && dataLine.available() >= 2048) {
			playThread.start();
			playing = true; // !!! :D :D :D
		}
	}

	private static class Play implements Runnable {

		private SourceDataLine source;
		
		public Play(SourceDataLine source) {
			this.source = source;
		}
		
		@Override
		public void run() {
			source.drain();
		}
		
	}
	
}
