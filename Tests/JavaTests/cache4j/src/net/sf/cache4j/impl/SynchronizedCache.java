/* =========================================================================
 * File: $Id: SynchronizedCache.java,v 1.1 2010/06/18 17:01:11 smhuang Exp $SynchronizedCache.java,v$
 *
 * Copyright (c) 2006, Yuriy Stepovoy. All rights reserved.
 * email: stepovoy@gmail.com
 *
 * =========================================================================
 */

package net.sf.cache4j.impl;

import net.sf.cache4j.CacheException;
import net.sf.cache4j.Cache;
import net.sf.cache4j.CacheConfig;
import net.sf.cache4j.CacheInfo;
import net.sf.cache4j.ManagedCache;

import java.util.HashMap;
import java.util.Map;
import java.util.TreeMap;
import java.io.IOException;

/**
 * Класс SynchronizedCache это реализация интерфейса {@link Cache}
 * с синхронизироваными методами доступа к объектам кеша.
 * <br>
 * Получение экземпляра кеша:
 * <pre>
 *     Cache _personCache = CacheFactory.getInstance().getCache("Person");
 * </pre>
 * Получение\помещение объекта:
 * <pre>
 *     Long id = ... ;
 *     try {
 *         Person person = (Person)_personCache.get(id);
 *         if (person != null) {
 *             return person;
 *         }
 *         person = loadPersonFromDb(id);
 *         _personCache.put(id, person);
 *     } catch (CacheException ce) {
 *         //throw new Exception(ce);
 *     }
 * </pre>
 * Удаление объекта:
 * <pre>
 *     Person person = ... ;
 *     Long id = person.getId();
 *     removePersonFromDb(id);
 *     try {
 *         _personCache.remove(id);
 *     } catch (CacheException ce) {
 *         //throw new Exception(ce);
 *     }
 * </pre>
 *
 * @version $Revision: 1.1 $ $Date: 2010/06/18 17:01:11 $
 * @author Yuriy Stepovoy. <a href="mailto:stepovoy@gmail.com">stepovoy@gmail.com</a>
 **/
public class SynchronizedCache implements Cache, ManagedCache {
// ----------------------------------------------------------------------------- Константы
// ----------------------------------------------------------------------------- Атрибуты класса
    /**
     * Карта с кешируемыми объектами
     */
    private Map _map;

    /**
     * Дерево с отсортироваными парами ключ\значение в зависимости от алгоритма удаления
     */
    private TreeMap _tmap;

    /**
     * Конфигурация кеша
     */
    private CacheConfigImpl _config;

    /**
     * Размер объектов кеша в байтах
     */
    private long _memorySize;

    /**
     * Информация о кеше
     */
    private CacheInfoImpl _cacheInfo;

// ----------------------------------------------------------------------------- Статические переменные
// ----------------------------------------------------------------------------- Конструкторы
// ----------------------------------------------------------------------------- Public методы

    //-------------------------------------------------------------------------- Cache interface
    /**
     * Помещает объект в кеш.
     * @param objId идентификатор объекта
     * @param obj объект
     * @throws CacheException если возникли проблемы, например при вычислении размера объекта
     * @throws NullPointerException если objId==null
     */
    public synchronized void put(Object objId, Object obj) throws CacheException {
        if(objId==null) {
            throw new NullPointerException("objId is null");
        }

        //оцениваем размер объекта
        int objSize = 0;
        try {
            objSize = _config.getMaxMemorySize()>0 ? Utils.size(obj) : 0;
        } catch (IOException e) {
            throw new CacheException(e.getMessage());
        }

        //проверяем не будет ли переполнение после помещения объекта
        checkOverflow(objSize);

        CacheObject co = (CacheObject)_map.get(objId);

        if(co!=null) {
            _tmap.remove(co);
            resetCacheObject(co);
        } else {
            co = newCacheObject(objId);
        }

        _cacheInfo.incPut();

        co.setObject(obj);
        co.setObjectSize(objSize);
        _memorySize = _memorySize + objSize;

        _tmap.put(co, co);
    }

    /**
     * Возвращает объект из кеша.
     * @param objId идентификатор объекта
     * @return Объект возвращается только в том случае, если объект найден
     * и время жизни объекта не закончилось и не превышено время бездействия.
     * @throws CacheException если возникли проблемы
     * @throws NullPointerException если objId==null
     */
    public synchronized Object get(Object objId) throws CacheException {
        if(objId==null) {
            throw new NullPointerException("objId is null");
        }

        CacheObject co = (CacheObject)_map.get(objId);
        Object o = co==null ? null : co.getObject();
        if(o!=null){
            if(!valid(co)) {
                remove(co.getObjectId());

                _cacheInfo.incMisses();
                return null;
            } else {
                _tmap.remove(co);
                co.updateStatistics();
                _tmap.put(co, co);

                _cacheInfo.incHits();
                return o;
            }
        } else {
            _cacheInfo.incMisses();
            return null;
        }
    }

    /**
     * Удаляет объект из кеша.
     * @param objId идентификатор объекта
     * @throws CacheException если возникли проблемы
     * @throws NullPointerException если objId==null
     */
    public synchronized void remove(Object objId) throws CacheException {
        if(objId==null) {
            throw new NullPointerException("objId is null");
        }

        CacheObject co = (CacheObject)_map.remove(objId);

        _cacheInfo.incRemove();

        if(co!=null) {
            _tmap.remove(co);
            resetCacheObject(co);
        }
    }

    /**
     * Возвращает количество объектов в кеше
     */
    public int size() {
        return _map.size();
    }

    /**
     * Удаляет все объекты из кеша
     * @throws CacheException если возникли проблемы
     */
    public synchronized void clear() throws CacheException {
        _map.clear();
        _tmap.clear();
        _memorySize = 0;
    }

    /**
     * Возвращает информацию о кеше
     */
    public CacheInfo getCacheInfo() {
        return _cacheInfo;
    }

    /**
     * Возвращает конфигруцию кеша
     */
    public CacheConfig getCacheConfig() {
        return _config;
    }
    //-------------------------------------------------------------------------- Cache interface

    //-------------------------------------------------------------------------- ManagedCache interface

    /**
     * Устанавливает конфигурацию кеша. При установке конфигурации все объекты
     * кеша теряются.
     * @param config конфигурация
     * @throws CacheException если возникли проблемы
     * @throws NullPointerException если config==null
     */
    public synchronized void setCacheConfig(CacheConfig config) throws CacheException {
        if(config==null) {
            throw new NullPointerException("config is null");
        }

        _config = (CacheConfigImpl)config;

        _map = _config.getMaxSize()>1000 ? new HashMap(1024) : new HashMap();
        _memorySize = 0;
        _tmap = new TreeMap(_config.getAlgorithmComparator());
        _cacheInfo = new CacheInfoImpl();
    }

    /**
     * Выполняет очистку кеша. Удаляются объекты у которых закончилось время
     * жизни или превышен период ожидания или если объект равен null.
     * @throws CacheException если возникли проблемы
     */
    public void clean() throws CacheException {
        //объекты из кеша нужно удалять по времени ?
        if(_config.getTimeToLive()==0 && _config.getIdleTime()==0){
            return;
        }

        Object[] objArr = null;
        synchronized(this) {
            objArr = _map.values().toArray();
        }

        for (int i = 0, indx = objArr==null ? 0 : objArr.length; i<indx; i++) {
            CacheObject co = (CacheObject)objArr[i];
            if ( !valid(co) ) {
                remove(co.getObjectId());
            }
        }
    }

    //-------------------------------------------------------------------------- ManagedCache interface

// ----------------------------------------------------------------------------- Package scope методы
// ----------------------------------------------------------------------------- Protected методы
// ----------------------------------------------------------------------------- Private методы

    /**
     * Если кеш переполнен, по количеству объектов или по размеру, то
     * удаляется первый объект в соответсвии с алгоритмом LFU, LRU, FIFO, ...
     */
    private void checkOverflow(int objSize) {
        //произошло переполнение по количеству или размеру ?
        //если помещаем большой объект то, возможно, из кеша нужно удалить несколько
        //объектов поменьше поэтому while
        while ( (_config.getMaxSize() > 0 && _map.size()+1   > _config.getMaxSize()) ||
                (_config.getMaxMemorySize()  > 0 && _memorySize+objSize > _config.getMaxMemorySize()) ) {

            //если в tmap что то есть удаляем первый элемент
            //при прямой сортировке это будет минимальный элемент
            //для LRU это будет самый первый использованый объект
            CacheObject co = _tmap.size()==0 ? null : (CacheObject)_tmap.remove(_tmap.firstKey());

            if(co!=null) {
                _map.remove(co.getObjectId());
                resetCacheObject(co);
            }
        }
    }


    /**
     * Создаёт CacheObject с идентификатором objId и помещает его в _map.
     * @param objId идентификатор объекта
     */
    private CacheObject newCacheObject(Object objId) {
        CacheObject co = _config.newCacheObject(objId);
        _map.put(objId, co);
        return co;
    }
    /**
     * Возвращает true если объект валидный.
     * @param co CacheObject
     */
    private boolean valid(CacheObject co) {
        long curTime = System.currentTimeMillis();
        return  (_config.getTimeToLive()==0 || (co.getCreateTime()  + _config.getTimeToLive()) >= curTime) &&
                (_config.getIdleTime()==0 || (co.getLastAccessTime() + _config.getIdleTime()) >= curTime) &&
                //если используются soft ссылки то возможна ситуация когда объекта
                //внутри CacheObject может уже не быть
                co.getObject()!=null;
    }
    /**
     *  Корреектирует размер объектов в кеше, обнуляет CacheObject
     * @param co CacheObject
     */
    private void resetCacheObject(CacheObject co){
        _memorySize = _memorySize - co.getObjectSize();
        co.reset();
    }

// ----------------------------------------------------------------------------- Inner классы
    private class CacheInfoImpl implements CacheInfo {
        private long _hit;
        private long _miss;
        private long _put;
        private long _remove;

        void incHits(){
            _hit++;
        }
        void incMisses(){
            _miss++;
        }
        void incPut(){
            _put++;
        }
        void incRemove(){
            _remove++;
        }
        public long getCacheHits(){
            return _hit;
        }
        public long getCacheMisses(){
            return _miss;
        }
        public long getTotalPuts() {
            return _put;
        }
        public long getTotalRemoves() {
            return _remove;
        }
        public synchronized void reset() {
            _hit = 0;
            _miss = 0;
            _put = 0;
            _remove = 0;
        }
        public long getMemorySize() {
            return _memorySize;
        }
        public String toString(){
            return "hit:"+_hit+" miss:"+_miss+" memorySize:"+_memorySize;
            //DEBUG return "hit:"+_hit+" miss:"+_miss+" memorySize:"+_memorySize+" size:"+_map.size()+" tsize:"+_tmap.size();
        }
    }
}

/*
$Log: SynchronizedCache.java,v $
Revision 1.1  2010/06/18 17:01:11  smhuang
*** empty log message ***

*/
