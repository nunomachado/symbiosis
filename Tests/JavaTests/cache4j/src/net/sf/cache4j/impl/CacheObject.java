/* =========================================================================
 * File: $Id: CacheObject.java,v 1.1 2010/06/18 17:01:11 smhuang Exp $CacheObject.java,v$
 *
 * Copyright (c) 2006, Yuriy Stepovoy. All rights reserved.
 * email: stepovoy@gmail.com
 *
 * =========================================================================
 */

package net.sf.cache4j.impl;

import net.sf.cache4j.CacheException;

/**
 * Класс CacheObject это оболочка для кешируемых объектов.
 *
 * @version $Revision: 1.1 $ $Date: 2010/06/18 17:01:11 $
 * @author Yuriy Stepovoy. <a href="mailto:stepovoy@gmail.com">stepovoy@gmail.com</a>
 **/

public class CacheObject {
// ----------------------------------------------------------------------------- Константы
// ----------------------------------------------------------------------------- Атрибуты класса

    /**
     * Идентификатор объекта
     */
    private Object _objId;

    /**
     * Кешируемый объект
     */
    protected Object _obj;

    /**
     * Количество обращений к объекту
     */
    private long _accessCount;

    /**
     * Время создания объекта
     */
    private long _createTime;

    /**
     * Время последнего доступа к объекту
     */
    private long _lastAccessTime;

    /**
     * Размер объекта в байтах
     */
    private int _objSize;

    /**
     * Поток блокирующий объект
     */
    private Thread _lockThread;

    /**
     * Уникальный идентификатор объекта
     */
    private long _id;

    //private long _version = 0; //версия объекта
    //private int _priority = 0; //приобритет объекта
// ----------------------------------------------------------------------------- Статические переменные
// ----------------------------------------------------------------------------- Конструкторы

    /**
     * Конструктор
     * @param objId идентификатор кешируемого объекта
     */
    CacheObject(Object objId) {
        _objId = objId;
        _obj = null;

        _createTime = System.currentTimeMillis();
        _accessCount = 1;
        _lastAccessTime = _createTime;
        _objSize = 0;
        _lockThread = null;

        _id = nextId();
    }

// ----------------------------------------------------------------------------- Public методы

    /**
     * Блокирует все потоки внутри метода, если метод предварительно был вызван
     * каким либо потоком. В пределах одного потока этот метод можно вызывать
     * произвольное количество раз, это не будет приводить к блоктровке текущего
     * потока.
     * @throws CacheException
     */
    synchronized void lock() throws CacheException {
        // в классе Mutex есть такая проверка
        //if (Thread.interrupted()) throw new InterruptedException();
        //?
        //synchronized (this) {


        //если объект заблокирован и текущий поток равен потоку который заблоктровал
        //то ничего не делаем. это cделано для того чтобы при повторном вызове lock
        //вызывающий поток не заснул в ожидании непонятно чего.
        if(_lockThread!=null && Thread.currentThread()==_lockThread){
            return;
        }

        try {
            while (_lockThread!=null) {
                //System.out.println("" + this.hashCode() + " WAIT LOCK Thread:" + Thread.currentThread().getName() + " " + (_lockThread!=null));
                wait();
                //System.out.println("" + this.hashCode() + " WAKE UP LOCK Thread:" + Thread.currentThread().getName() + " " + (_lockThread!=null));
            }
            //System.out.println(""+this.hashCode()+" GET LOCK Thread:"+Thread.currentThread().getName()+" "+(_lockThread!=null));
            _lockThread = Thread.currentThread(); //устанавливаем блокирующий поток
        } catch (InterruptedException ex) {
            notify();      //так в два раза быстрее
            //notifyAll(); //оповещаем все потоки
            throw new CacheException(ex.getMessage());
        }
    }

    /**
     * Снимает блокировку с объекта и будит один поток ожидающий блокировку на
     * текущий объект.
     */
    synchronized void unlock() {
        _lockThread = null;

        //System.out.println("" + this.hashCode() + " RELEASE LOCK Thread:" + Thread.currentThread().getName());
        notify();      //так в два раза быстрее
        //notifyAll(); //оповещаем все потоки
    }

    /**
     * Возвращает кешируемый объект
     */
    Object getObject() {
        return _obj;
    }

    /**
     * Устанавливает кешируемый объект
     */
    void setObject(Object obj) {
        _obj = obj;
    }

    /**
     * Возвращает идентификатор кешируемого объекта
     */
    Object getObjectId(){
        return _objId;
    }

    /**
     * Возвращает количество обращений к объекту
     */
    long getAccessCount() {
        return _accessCount;
    }

    /**
     * Возвращает время создания объекта в миллисекундах
     */
    long getCreateTime() {
        return _createTime;
    }

    /**
     * Возвращает время последнего доступа в миллисекундах
     */
    long getLastAccessTime() {
        return _lastAccessTime;
    }

    /**
     * Возвращает размер объекта в байтах
     */
    long getObjectSize() {
        return _objSize;
    }

    /**
     * Устанавливает размер объекта в байтах
     */
    void setObjectSize(int objSize) {
        _objSize = objSize;
    }

    /**
     * Обновляет статистику по объекту
     */
    void updateStatistics() {
        _accessCount++;
        _lastAccessTime = System.currentTimeMillis();

        _id = nextId();
    }

    /**
     * Сбрасывает статистику объекта
     */
    void reset(){
        _obj = null;
        _objSize = 0;
        _createTime = System.currentTimeMillis();
        _accessCount = 1;
        _lastAccessTime = _createTime;

        _id = nextId();
    }

    long getId() {
        return _id;
    }

    /**
     * Возвращает строковое представление объекта
     */
    public String toString(){
        return "id:"+_objId+
                " createTime:"+_createTime+
                " lastAccess:"+_lastAccessTime+
                " accessCount:"+_accessCount+
                " size:"+_objSize+
                " object:"+_obj;
    }
// ----------------------------------------------------------------------------- Package scope методы
// ----------------------------------------------------------------------------- Protected методы
// ----------------------------------------------------------------------------- Private методы

    /**
     * Счётчик
     */
    private static long ID = 0;
    /**
     * Возвращает следующий уникальный идентификатор
     */
    //** Nuno: this was synchronized, but I change it so that Soot doesn't crash
    private static long nextId(){
        return ID++;
    }
// ----------------------------------------------------------------------------- Inner классы
}

/*
$Log: CacheObject.java,v $
Revision 1.1  2010/06/18 17:01:11  smhuang
*** empty log message ***

*/
