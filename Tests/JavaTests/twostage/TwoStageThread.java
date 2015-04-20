/*
 * Created on Apr 27, 2005
*/
package twostage;
/**
 * @author Xuan
 */
public class TwoStageThread extends Thread {
	TwoStage ts;
	public TwoStageThread(TwoStage ts) {
		this.ts=ts;
	}
	
	public void run() {
		ts.A();
	}

}
