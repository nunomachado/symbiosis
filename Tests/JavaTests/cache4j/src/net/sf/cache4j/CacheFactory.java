/* =========================================================================
 * File: $Id: CacheFactory.java,v 1.1 2010/06/18 17:01:12 smhuang Exp $CacheFactory.java,v$
 *
 * Copyright (c) 2006, Yuriy Stepovoy. All rights reserved.
 * email: stepovoy@gmail.com
 *
 * =========================================================================
 */

package net.sf.cache4j;

import net.sf.cache4j.Cache;
import net.sf.cache4j.CacheConfig;
import net.sf.cache4j.CacheException;
import net.sf.cache4j.impl.Configurator;

import java.util.Map;
import java.util.HashMap;
import java.io.InputStream;

/**
 * Класс CacheFactory управляет экземплярами кешей.
 *
 * @version $Revision: 1.1 $ $Date: 2010/06/18 17:01:12 $
 * @author Yuriy Stepovoy. <a href="mailto:stepovoy@gmail.com">stepovoy@gmail.com</a>
 **/

public class CacheFactory {
// ----------------------------------------------------------------------------- Константы
// ----------------------------------------------------------------------------- Атрибуты класса
// ----------------------------------------------------------------------------- Статические переменные

    /**
     * Карта с экземплярами кешей
     */
    private Map _cacheMap;

    /**
     * Поток занимающийся очисткой кеша
     */
    private CacheCleaner _cleaner;

    /**
     * Singleton
     */
    private static final CacheFactory _cacheFactory = new CacheFactory();

// ----------------------------------------------------------------------------- Конструкторы

    /**
     * Конструктор
     */
    public CacheFactory() {
        _cacheMap = new HashMap();
        _cleaner = new CacheCleaner(30000); //default 30sec
        _cleaner.start();
    }

// ----------------------------------------------------------------------------- Public методы

    /**
     * Возвращает экземпляр CacheFactory
     */
    public static CacheFactory getInstance(){
        return _cacheFactory;
    }

    /**
     * Загружает список кешей из xml конфигурации, без очистки CacheFactory.
     * @param in входной поток с xml конфигурацией
     * @throws CacheException
     */
    public void loadConfig(InputStream in) throws CacheException {
        Configurator.loadConfig(in);
    }

    /**
     * Добавляет кеш. Кеш кроме интерфейса Cache должен реализовывать интерфейс ManagedCache.
     * @param cache кеш
     * @throws NullPointerException если cache==null или cache.getCacheConfig()==null
     * или cache.getCacheConfig().getCacheId()==null
     * @throws CacheException если кеш уже существует или если добавляемый кеш не
     * реализует интерфейс ManagedCache
     */
    public void addCache(Cache cache) throws CacheException {
        if(cache==null){
            throw new NullPointerException("cache is null");
        }
        CacheConfig cacheConfig = cache.getCacheConfig();
        if(cacheConfig==null) {
            throw new NullPointerException("cache config is null");
        }
        if(cacheConfig.getCacheId()==null) {
            throw new NullPointerException("config.getCacheId() is null");
        }
        if(!(cache instanceof Cache)) {
            throw new CacheException("cache not instance of "+ManagedCache.class.getName());
        }

        synchronized(_cacheMap){
            if(_cacheMap.containsKey(cacheConfig.getCacheId())) {
                throw new CacheException("Cache id:"+cacheConfig.getCacheId()+" exists");
            }

            _cacheMap.put(cacheConfig.getCacheId(), cache);
        }
    }

    /**
     * Возвращает кеш
     * @param cacheId идентификатор кеша
     * @throws NullPointerException если cacheId==null
     */
    public Cache getCache(Object cacheId) throws CacheException {
        if(cacheId==null) {
            throw new NullPointerException("cacheId is null");
        }

        synchronized(_cacheMap){
            return (Cache)_cacheMap.get(cacheId);
        }
    }

    /**
     * Удаляет кеш
     * @param cacheId идентификатор кеша
     * @throws NullPointerException если cacheId==null
     */
    public void removeCache(Object cacheId) throws CacheException {
        if(cacheId==null) {
            throw new NullPointerException("cacheId is null");
        }

        synchronized(_cacheMap){
            _cacheMap.remove(cacheId);
        }
    }

    /**
     * Возвращает массив с идентификаторами кешей
     */
    public Object[] getCacheIds() {
        synchronized(_cacheMap) {
            return _cacheMap.keySet().toArray();
        }
    }

    /**
     * Устанавливает интервал очистки кеша
     * @param time количество миллисекунд
     */
    public void setCleanInterval(long time) {
        _cleaner.setCleanInterval(time);
    }

// ----------------------------------------------------------------------------- Package scope методы
// ----------------------------------------------------------------------------- Protected методы
// ----------------------------------------------------------------------------- Private методы
// ----------------------------------------------------------------------------- Inner классы

}

/*
$Log: CacheFactory.java,v $
Revision 1.1  2010/06/18 17:01:12  smhuang
*** empty log message ***

*/
