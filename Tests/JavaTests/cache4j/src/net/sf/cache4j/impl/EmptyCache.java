/* =========================================================================
 * File: $Id: EmptyCache.java,v 1.1 2010/06/18 17:01:12 smhuang Exp $EmptyCache.java,v$
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

/**
 * Класс EmptyCache это реализация интерфейса {@link net.sf.cache4j.Cache}
 * без какой либо полезной функциональности. Метод <code>get()</code> всегда
 * возвращает <code>null</code>. Эту реализацию кеша можно использовать при
 * необходимости отключить кеширование.
 * @version $Revision: 1.1 $ $Date: 2010/06/18 17:01:12 $
 * @author Yuriy Stepovoy. <a href="mailto:stepovoy@gmail.com">stepovoy@gmail.com</a>
 **/
public class EmptyCache implements Cache, ManagedCache {
// ----------------------------------------------------------------------------- Константы
// ----------------------------------------------------------------------------- Атрибуты класса

    /**
     * Конфигурация кеша
     */
    private CacheConfigImpl _config;

    /**
     * Информация о кеше
     */
    private CacheInfo _cacheInfo;

// ----------------------------------------------------------------------------- Статические переменные
// ----------------------------------------------------------------------------- Конструкторы
// ----------------------------------------------------------------------------- Public методы

    //-------------------------------------------------------------------------- Cache interface
    /**
     * Помещает объект в кеш.
     * @param objId идентификатор объекта
     * @param obj объект
     * @throws NullPointerException если objId==null
     */
    public void put(Object objId, Object obj) throws CacheException {
        if(objId==null) {
            throw new NullPointerException("objId is null");
        }
    }

    /**
     * Возвращает объект из кеша. Всегда возвращает null.
     * @param objId идентификатор объекта
     * @return всегда возвращает null
     * @throws NullPointerException если objId==null
     */
    public Object get(Object objId) throws CacheException {
        if(objId==null) {
            throw new NullPointerException("objId is null");
        }
        return null;
    }

    /**
     * Удаляет объект из кеша.
     * @param objId идентификатор объекта
     * @throws NullPointerException если objId==null
     */
    public void remove(Object objId) throws CacheException {
        if(objId==null) {
            throw new NullPointerException("objId is null");
        }
    }

    /**
     * Возвращает количество объектов в кеше
     * @return всегда возвращает 0.
     */
    public int size() {
        return 0;
    }

    /**
     * Удаляет все объекты из кеша
     */
    public void clear() throws CacheException {
    }

    /**
     * Возвращает информацию о кеше
     * @return всегда возвращает null
     */
    public CacheInfo getCacheInfo() {
        return _cacheInfo;
    }

    /**
     * Возвращает конфигруцию кеша
     * @return возвращает конфигурацию кеша
     */
    public CacheConfig getCacheConfig() {
        return _config;
    }

    //-------------------------------------------------------------------------- Cache interface

    //-------------------------------------------------------------------------- ManagedCache interface

    /**
     * Устанавливает конфигурацию кеша.
     * @param config конфигурация
     * @throws NullPointerException если config==null
     */
    public void setCacheConfig(CacheConfig config) {
        if(config==null) {
            throw new NullPointerException("config is null");
        }
        _config = (CacheConfigImpl)config;
        _cacheInfo = new CacheInfoImpl();
    }

    /**
     * Выполняет очистку кеша от устаревших объектов.
     */
    public void clean() throws CacheException {
    }

    //-------------------------------------------------------------------------- ManagedCache interface

// ----------------------------------------------------------------------------- Package scope методы
// ----------------------------------------------------------------------------- Protected методы
// ----------------------------------------------------------------------------- Private методы
// ----------------------------------------------------------------------------- Inner классы
    private class CacheInfoImpl implements CacheInfo {
        public long getCacheHits(){
            return 0;
        }
        public long getCacheMisses(){
            return 0;
        }
        public long getTotalPuts() {
            return 0;
        }
        public long getTotalRemoves() {
            return 0;
        }
        public void reset() {

        }
        public long getMemorySize() {
            return 0;
        }
        public String toString(){
            return "hit:0 miss:0 memorySize:0";
        }
    }
}

/*
$Log: EmptyCache.java,v $
Revision 1.1  2010/06/18 17:01:12  smhuang
*** empty log message ***

*/
