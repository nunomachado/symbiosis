/* =========================================================================
 * File: CacheConfigImpl.java$
 *
 * Copyright (c) 2006, Yuriy Stepovoy. All rights reserved.
 * email: stepovoy@gmail.com
 *
 * =========================================================================
 */

package net.sf.cache4j.impl;

import net.sf.cache4j.CacheConfig;

import java.util.Comparator;

/**
 * Класс CacheConfigImpl содержит конфигурацию кеша
 *
 * @version $Revision: 1.1 $ $Date: 2010/06/18 17:01:11 $
 * @author Yuriy Stepovoy. <a href="mailto:stepovoy@gmail.com">stepovoy@gmail.com</a>
 **/

public class CacheConfigImpl implements CacheConfig {
// ----------------------------------------------------------------------------- Константы
// ----------------------------------------------------------------------------- Атрибуты класса
    /** Идентификатор кеша */
    private Object _cacheId;
    /** Описание кеша */
    private String _cacheDesc;
    /** Максимальное время жизни объектов в кеше */
    private long _ttl;
    /** Максимальное время бездействия объектов в кеше */
    private long _idleTime;
    /** Максимальный объём занимаемый объектами кеша */
    private long _maxMemorySize;
    /** Максимальный количество объектов в кеше */
    private int _maxSize;
    /** Тип кеша */
    private String _type;
    /** Алгоритм вытеснения объектов */
    private String _algorithm;
    /** Тип ссылки на хранимый объект */
    private String _reference;
    /** Тип ссылки на хранимый объект */
    private int _referenceInt;
// ----------------------------------------------------------------------------- Статические переменные

    /** Алгоритм вытеснения объектов - LRU */
    static final String LRU = "lru";
    /** Алгоритм вытеснения объектов - LFU */
    static final String LFU = "lfu";
    /** Алгоритм вытеснения объектов - FIFO */
    static final String FIFO = "fifo";

    /** Тип связи с объектом - STRONG */
    static final int STRONG = 1;
    /** Тип связи с объектом - SOFT */
    static final int SOFT = 2;

// ----------------------------------------------------------------------------- Конструкторы
    /**
     * Конструктор
     * @param cacheId идентификатор кеша
     * @param cacheDesc описание кеша
     * @param ttl максимальное время жизни объектов в кеше
     * @param idleTime максимальное время бездействия объектов в кеше
     * @param maxMemorySize максимальный объём занимаемый объектами кеша
     * @param maxSize максимальный количество объектов в кеше
     * @param type тип кеша
     * @param algorithm алгоритм вытеснения объектов
     * @param reference тип ссылки на хранимый объект
     */
    public CacheConfigImpl(Object cacheId,
                           String cacheDesc,
                           long ttl,
                           long idleTime,
                           long maxMemorySize,
                           int maxSize,
                           String type,
                           String algorithm,
                           String reference) {
        _cacheId = cacheId;
        _cacheDesc = cacheDesc;
        _ttl = ttl<0 ? 0 : ttl;
        _idleTime = idleTime<0 ? 0 : idleTime;
        _maxMemorySize = maxMemorySize<0 ? 0 : maxMemorySize;
        _maxSize = maxSize<0 ? 0 : maxSize;
        _type = type;
        _algorithm = algorithm;
        _reference = reference;

        if(_reference.equalsIgnoreCase("strong")){
            _referenceInt = STRONG;
        } else if(_reference.equalsIgnoreCase("soft")){
            _referenceInt = SOFT;
        }
    }
// ----------------------------------------------------------------------------- Public методы

    public Object getCacheId() {
        return _cacheId;
    }
    public String getCacheDesc() {
        return _cacheDesc;
    }
    public long getTimeToLive() {
        return _ttl;
    }
    public long getIdleTime() {
        return _idleTime;
    }
    public long getMaxMemorySize() {
        return _maxMemorySize;
    }
    public int getMaxSize() {
        return _maxSize;
    }
    public String getType() {
        return _type;
    }
    public String getAlgorithm() {
        return _algorithm;
    }
    public String getReference(){
        return _reference;
    }
// ----------------------------------------------------------------------------- Package scope методы
    /**
     * Создаёт оболочку для хранения объектов
     * @param objId идентификатор объекта
     * @return возвращает новый объект
     */
    CacheObject newCacheObject(Object objId){
        return _referenceInt==CacheConfigImpl.STRONG ? new CacheObject(objId) : new SoftCacheObject(objId);
    }

    /**
     * Возвращает компаратор с учётом алгоритма вытеснения
     */
    Comparator getAlgorithmComparator(){
        if(_algorithm.equals(CacheConfigImpl.LRU)) {
            return new LRUComparator();
        } else if(_algorithm.equals(CacheConfigImpl.LFU)) {
            return new LFUComparator();
        } else if(_algorithm.equals(CacheConfigImpl.FIFO)) {
            return new FIFOComparator();
        } else {
            throw new RuntimeException("Unknown algorithm:"+_algorithm);
        }
    }
// ----------------------------------------------------------------------------- Protected методы
// ----------------------------------------------------------------------------- Private методы
// ----------------------------------------------------------------------------- Inner классы
}

/*
$Log: CacheConfigImpl.java,v $
Revision 1.1  2010/06/18 17:01:11  smhuang
*** empty log message ***

*/
