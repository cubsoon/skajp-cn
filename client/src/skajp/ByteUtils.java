package skajp;

public class ByteUtils {
	
	public static int strncmp(byte[] byteArray, int byteArrayStart, byte[] otherByteArray, int n) {
		for (int i = 0; i < n; i++) {
			int d = byteArray[i + byteArrayStart] - otherByteArray[i];
			if (d != 0) {
				return d;
			}
		}
		return 0;
	}
	
	public static int strncpy(byte[] destination, int destinationStart, byte[] source, int n) {
		for (int i = 0; i < n; i++) {
			destination[i + destinationStart] = source[i];
		}
		return n;
	}
	
	public static int strncpy(byte[] destination, byte[] source, int sourceStart, int n) {
		for (int i = 0; i < n; i++) {
			destination[i] = source[i + sourceStart];
		}
		return n;
	}
}
