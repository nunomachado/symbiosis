package pt.tecnico.symbiosis.runtime;

public class MonitorThread extends Thread{
	
	public boolean isCrashed;
	
	MonitorThread()
	{
		super("MonitorThread");
	}
	
	public void run()
	{
		SymbiosisRuntime.saveTrace();
	}

}
