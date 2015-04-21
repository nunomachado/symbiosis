package net.sf.cache4j.test;

import net.sf.cache4j.CacheCleaner;


//** CHANGE - created by me
public class WorkerThread extends Thread{

	public CacheCleaner cache;

	public WorkerThread(CacheCleaner c)
	{
		cache = c;
	}

	public void run()
	{
		for(int i = 0; i < 10; i++)
		{
			cache.setCleanInterval(i);
			System.out.println(this.getName()+" iteration "+(i+1));
		}
	}
}
