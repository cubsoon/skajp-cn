package skajp;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;

import javax.sound.sampled.LineUnavailableException;
import javax.swing.*;        

public class SwingApplication implements ActionListener {
    
	private ApplicationState state;
	
	private Socket socket;
	
	private InputStream input;
	private OutputStream output;
	
	public static final int BUFFER_SIZE = 2048;
	byte[] buffer;
	
	private MessageHandler messageHandler;
	private Thread inputThread;
	
	private OutputMessageHandler outputMessageHandler;
	private Thread outputThread;
	
	private static JFrame frame;
	private JTextField serverIpField;
	private JTextField serverPortField;
	private JTextField senderNameField;
	private JButton connectButton;
	private JTextField receiverNameField;
	private JButton callButton;
	private JButton endButton;
	
	private SwingApplication() {
		buffer = new byte[BUFFER_SIZE];
	}
	
	/**
     * Create the GUI and show it.  For thread safety,
     * this method should be invoked from the
     * event-dispatching thread.
     */	
    private void createAndShowGUI() {
        //Create components
        serverIpField = new JTextField("127.0.0.1");
        serverPortField = new JTextField("1234");
        senderNameField = new JTextField("Your name");
        connectButton = new JButton("CONNECT");
        receiverNameField = new JTextField("Receiver name");
        callButton = new JButton("CALL");
        endButton = new JButton("END CALL");
        
        connectButton.addActionListener(this);
        callButton.addActionListener(this);
        endButton.addActionListener(this);
        
        //Create layout
        GridLayout layout = new GridLayout(2, 4);
        layout.setHgap(10);
        layout.setVgap(10);
        
        //Create frame
        frame = new JFrame("Skajp");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setLayout(layout);
        
        frame.add(serverIpField);
        frame.add(serverPortField);
        frame.add(senderNameField);
        frame.add(connectButton);
        frame.add(receiverNameField);
        frame.add(callButton);
        frame.add(endButton);
        
        setApplicationState(ApplicationState.STATE_IDLE);
        
        //Display the window.
        frame.setResizable(false);
        frame.pack();
        frame.setVisible(true);
    }

    public static void main(String[] args) throws LineUnavailableException {
        //Schedule a job for the event-dispatching thread:
        //creating and showing this application's GUI.
        final SwingApplication application = new SwingApplication();
    	        
    	javax.swing.SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                application.createAndShowGUI();
            }
        });
    }
    
    public ApplicationState getApplicationState() {
    	return this.state;
    }
    
    public void setApplicationState(ApplicationState state) {
    	if (state.equals(this.state)) {
    		return;
    	}
    	if (state.equals(ApplicationState.STATE_IDLE)) {
    		try {
    			if (messageHandler != null) {
    				messageHandler.getRunning().set(false);
    			}
        		if (outputMessageHandler != null) {
        			outputMessageHandler.getRunning().set(false);
        		}
    			if (socket != null) {
	    			socket.close();
	    		}
    			if (input != null) {
	    			input.close();
	    		}
	    		if (output != null) {
	    			output.close();
	    		}
    		}
    		catch (Exception ex) {
    			outputException(ex);
    		}
    		
    		serverIpField.setEnabled(true);
    		serverPortField.setEnabled(true);
    		senderNameField.setEnabled(true);
    		connectButton.setEnabled(true);
    		receiverNameField.setEnabled(false);
    		callButton.setEnabled(false);
    		endButton.setEnabled(false);
    	}
    	else if (state.equals(ApplicationState.STATE_CONNECTED)) {
    		serverIpField.setEnabled(false);
    		serverPortField.setEnabled(false);
    		senderNameField.setEnabled(false);
    		connectButton.setEnabled(false);
    		receiverNameField.setEnabled(true);
    		callButton.setEnabled(true);
    		endButton.setEnabled(false);
    	}
    	else if (state.equals(ApplicationState.STATE_TRANSMISSION)) {
    		serverIpField.setEnabled(false);
    		serverPortField.setEnabled(false);
    		senderNameField.setEnabled(false);
    		connectButton.setEnabled(false);
    		receiverNameField.setEnabled(false);
    		callButton.setEnabled(false);
    		endButton.setEnabled(true);
    	}
    	this.state = state;
    }

	@Override
	public void actionPerformed(ActionEvent e) {
		if (connectButton.equals(e.getSource())) {
			try {
				socket = new Socket(
						serverIpField.getText(),
						Integer.parseInt(serverPortField.getText())
				);
				
				input = socket.getInputStream();
				output = socket.getOutputStream();
			
				Message helloMessage = new Message();
				
				helloMessage.setMessageType(MessageType.MESSAGE_HELL);
				helloMessage.setSender(senderNameField.getText());
				int n = helloMessage.toBuffer(buffer);
				output.write(buffer, 0, n);
				
				input.read(buffer, 0, BUFFER_SIZE);
				helloMessage.fromBuffer(buffer);
				
				if (!helloMessage.getMessageType().equals(MessageType.MESSAGE_HELL)) {
					throw new InvalidResponseException("Wrong response - expected HELL");
				}
				if (!helloMessage.isSenderEqual(senderNameField.getText())) {
					throw new InvalidResponseException("Wrong response - wrong name");
				}
				
				messageHandler = new MessageHandler(this, input);
		    	inputThread = new Thread(messageHandler);
				inputThread.start();
				
				setApplicationState(ApplicationState.STATE_CONNECTED);
			} 
			catch (Exception ex) {
				outputException(ex);
				setApplicationState(ApplicationState.STATE_IDLE);
			}
		}
		else if (callButton.equals(e.getSource())) {
			try {
				Message callMessage = new Message();
				callMessage.setMessageType(MessageType.MESSAGE_CONN);
				callMessage.setSender(senderNameField.getText());
				callMessage.setReceiver(receiverNameField.getText());
				int n = callMessage.toBuffer(buffer);
				output.write(buffer, 0, n);
				
				outputMessageHandler = new OutputMessageHandler(output, senderNameField.getText(), receiverNameField.getText());
		    	outputThread = new Thread(outputMessageHandler);
				outputThread.start();
				
				setApplicationState(ApplicationState.STATE_TRANSMISSION);
			}
			catch (Exception ex) {
				outputException(ex);
				setApplicationState(ApplicationState.STATE_IDLE);
			}
		}
		else if (endButton.equals(e.getSource())) {
			try {
				Message byeMessage = new Message();
				byeMessage.setMessageType(MessageType.MESSAGE_BYES);
				byeMessage.setSender(senderNameField.getText());
				int n = byeMessage.toBuffer(buffer);
				output.write(buffer, 0, n);
			}
			catch (Exception ex) {
				outputException(ex);
			}
			finally {
				setApplicationState(ApplicationState.STATE_IDLE);
			}
		}
	}
	
	public static void outputException(Exception ex) {

		JOptionPane.showMessageDialog(frame, ex.getMessage());
		ex.printStackTrace();
	}
	
	public void setOutputHandler(String senderName) throws LineUnavailableException {
		Message callMessage = new Message();
		callMessage.setMessageType(MessageType.MESSAGE_CONN);
		callMessage.setSender(this.senderNameField.getText());
		callMessage.setReceiver(senderName);
		int n = callMessage.toBuffer(buffer);
		try {
			output.write(buffer, 0, n);
		} catch (IOException e) {
			outputException(e);
		}
		
		this.outputMessageHandler = new OutputMessageHandler(output, this.senderNameField.getText(), senderName);
		this.outputThread = new Thread(this.outputMessageHandler);
		this.outputThread.start();
	}
	
}
