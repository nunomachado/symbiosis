/* =========================================================================
 * File: CacheCleaner.java$
 *
 * Copyright (c) 2006, Yuriy Stepovoy. All rights reserved.
 * email: stepovoy@gmail.com
 *
 * =========================================================================
 */

package net.sf.cache4j;

/**
 * ����� CacheCleaner ��������� ������� ���������� ��������
 */

public class CacheCleaner extends Thread {
	// ----------------------------------------------------------------------------- ���������
	// ----------------------------------------------------------------------------- �������� ������

    
    //** Nuno: variables for Symbiosis
	boolean inTryBlock = false;	//indicates if a thread is inside a try/catch block
	boolean sleeping = false;	//variables that mimics the sleeping behavior of a thread

    /**
	 * �������� �������
	 */
	public long _cleanInterval;		//** CHANGE it was private

	/**
	 * true ���� ����� ��������� � ������
	 */
	public boolean _sleep = false;	//** CHANGE it was private

	// ----------------------------------------------------------------------------- ����������� ����������
	// ----------------------------------------------------------------------------- ������������

	/**
	 * �����������
	 * @param cleanInterval ��������(� �������������) � ������� ����� ��������� �������
	 */
	public CacheCleaner(long cleanInterval) {
		_cleanInterval = cleanInterval;

		setName(this.getClass().getName());
		setDaemon(true);
		System.out.println("Thread "+Thread.currentThread().getName()+" started");
		//������������� ����������� ��������� �� ����� ������ ��� �������� ����������
		//�������� �� ����� ������ ������
		//setPriority(Thread.MIN_PRIORITY);
	}

	// ----------------------------------------------------------------------------- Public ������

	/**
	 * ������������� �������� �������
	 * @param cleanInterval ��������(� �������������) � ������� ����� ��������� �������
	 */
	public void setCleanInterval(long cleanInterval) {
		_cleanInterval = cleanInterval;
		synchronized(this){
			if(_sleep){
				assert(inTryBlock == true);//Nuno: added this (Symbiosis)
				sleeping = false;//*/
                //interrupt();  //Nuno: commented this
			}
		}	
	}

	/**
	 * �������� �����. ��� ���� ����� ���������� ����� <code>clean</code>
	 */
	public void run() {
        
		while(true)  {
			try {
				CacheFactory cacheFactory = CacheFactory.getInstance();
				Object[] objIdArr = cacheFactory.getCacheIds();
				for (int i = 0, indx = objIdArr==null ? 0 : objIdArr.length; i<indx; i++) {
					ManagedCache cache = (ManagedCache)cacheFactory.getCache(objIdArr[i]);
					if(cache!=null){
						cache.clean();
					}
					yield();
				}
			} catch (Throwable t){
				t.printStackTrace();
			}

			_sleep = true; 
			yield();
			try {
				//Symbiosis
				inTryBlock = true;	//Nuno: added this
				//System.out.println(Thread.currentThread().getName()+" in");
				sleeping = true;
				while(!sleeping){}
				//sleep(_cleanInterval); //Nuno: commented this
			} catch (Throwable t){
			} finally {
				inTryBlock = false; //Nuno: added this (Symbiosis)
                //_sleep = false;
			}
		}
	}

	// ----------------------------------------------------------------------------- Package scope ������
	// ----------------------------------------------------------------------------- Protected ������
	// ----------------------------------------------------------------------------- Private ������
	// ----------------------------------------------------------------------------- Inner ������

}

/*
$Log: CacheCleaner.java,v $
Revision 1.1  2010/06/18 17:01:12  smhuang
 *** empty log message ***

 */
