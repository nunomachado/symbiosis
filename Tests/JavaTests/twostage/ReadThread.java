package twostage;
/**
 * @author Xuan
 * Created on Apr 27, 2005
 */
public class ReadThread extends Thread {
	TwoStage ts;
	public ReadThread(TwoStage ts) {
		this.ts=ts;
	}
	
	public void run() {
		ts.B();
	}
}
