/* =========================================================================
 * File: $Id: BlockingCache.java,v 1.1 2010/06/18 17:01:09 smhuang Exp $BlockingCache.java,v$
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

import java.util.Map;
import java.util.TreeMap;
import java.util.HashMap;
import java.io.IOException;

/**
 * Класс BlockingCache это реализация интерфейса {@link net.sf.cache4j.Cache}
 * с блокированием на уровне объектов. Вызов любокого метода <code>get, put, remove</code>
 * приводит к блоктровке идентификатора объекта а потом к снятию блокировки
 * (в пределах вызваного метода). Если метод <code>get</code> вернул null то
 * блокировка с идентификатора не снимается. Поток, который заблокировал
 * идентификатор, должен загрузить объект и поместить в кеш, только после этого
 * локировака будет снята и все заблокированые потоки смогут продолжить работу.
 * Такое поведение позволяет загружать(создавать) объект, которого нет в кеше,
 * только в одном потоке.
 * <br>
 * Получение экземпляра кеша:
 * <pre>
 *     Cache _personCache = CacheFactory.getInstance().getCache("Person");
 * </pre>
 * Получение\помещение объекта:
 * <pre>
 *     Long id = ... ;
 *     Person person = null;
 *     try {
 *         person = (Person)_personCache.get(id);
 *     } catch (CacheException ce) {
 *         //throw new Exception(ce);
 *     }
 *     if (person != null) {
 *         return person;
 *     }
 *     try {
 *         person = loadPersonFromDb(id);
 *     } catch (Exception e) {
 *         //throw new Exception(e);
 *     } finally {
 *         try {
 *             _personCache.put(id, person);
 *         } catch (CacheException ce) {
 *             //throw new Exception(ce);
 *         }
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
 * @version $Revision: 1.1 $ $Date: 2010/06/18 17:01:09 $
 * @author Yuriy Stepovoy. <a href="mailto:stepovoy@gmail.com">stepovoy@gmail.com</a>
 **/

public class BlockingCache implements Cache, ManagedCache {
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

    /**
     * ThreadLocal используется для хранения CacheObject между вызовами
     * get() - put()\remove(). Если метод get() вернул null, значит CacheObject
     * с переданым идентификатором объекта залокирован текущим потоком и кеш ждёт
     * помещения объекта.
     * Если этого не делать то возможно возникновение deadlock.
     * Пример:
     *   - поток1 удаляет CacheObject1 c ключом 1, поток2 ждёт локировки на
     *     CacheObject1 c ключом 1, поток3 ждёт локтровки на CacheObject1 c ключом 1.
     *   - поток1 удалил объект CacheObject1 и снял локировку. поток2 получил
     *     локировку на объект CacheObject1 ключ1 и получил null вместо объекта
     *     (потому что ссылка на пользовательский объект удалена)
     *   - поток2 загрузил объект с ключом 1 и помещает его в кеш, но так как
     *     CacheObject1 ключ1 был удалён потоком1 создаётся новый объект
     *     CacheObject2 с ключом 1. В новый CacheObject2
     *     помещается пользовательский объект и снимается локировка.
     *   - поток3 продолжает ждать снятия локировки с первого объекта CacheObject1
     *     с ключтом 1. поток3 никогда не проснётся. deadlock !!!
     */
    private ThreadLocal _tl = new ThreadLocal();

// ----------------------------------------------------------------------------- Статические переменные
// ----------------------------------------------------------------------------- Конструкторы
// ----------------------------------------------------------------------------- Public методы

    //-------------------------------------------------------------------------- Cache interface
    /**
     * Помещает объект в кеш. Если перед вызовом put() был вызван метод get()
     * и он вернул null то в put() нужно передать такой же objId как и в метод get()
     * иначе возникнет CacheException.
     * @param objId идентификатор объекта
     * @param obj объект
     * @throws CacheException если возникли проблемы, например вычисление размера объекта
     * @throws NullPointerException если objId==null
     */
    public void put(Object objId, Object obj) throws CacheException {
        if(objId==null) {
            throw new NullPointerException("objId is null");
        }

        //достаём CacheObject из текущего потока
        CacheObject tlCO = (CacheObject)_tl.get();
        CacheObject co = null;

        int objSize = 0;
        try {
            objSize = _config.getMaxMemorySize()>0 ? Utils.size(obj) : 0;
        } catch (IOException e) {
            throw new CacheException(e.getMessage());
        }

        //проверяем не произошло ли переполнение кеша
        checkOverflow(objSize);

        if (tlCO==null) {
            //если в текущем потоке нет CacheObject значит метод get() перед этим
            //или не вызывался или вернул объект
            co = getCacheObject(objId);
        } else {
            //если в текущем потоке есть объект значит перед этим метод get() вернул null

            //идентификатор из текущего потока и идентификатор помещаемого объекта должны быть
            //одинаковыми. если это не так то при вызове метода get() был передан один ключ
            //а при вызове метода put() был передан другой ключ, а это неправильно.
            if(tlCO.getObjectId().equals(objId)){
                //чтобы не потерялся локированый в методе get() объект
                //нужно именно локиронный объект использовать для дальнейшей обработки
                co = tlCO;
                //удалять объект из текущего потока нужно только после корректного
                //вызова метода put()
                _tl.set(null);
            } else {
                tlCO.unlock();
                throw new CacheException("Cache:"+_config.getCacheId()+" wait for call put() with objId="+tlCO.getObjectId());
            }
        }

        co.lock();
        _cacheInfo.incPut();
        try {
            //при помещении у объекта сбрасывается статистика co.reset()
            //и соответственно изменяется ключ с которым объект был помещён в _tmap
            //поэтому объект нужно удалить из _tmap
            //?:для объектов которых не было в кеше это лишняя операция
            tmapRemove(co);

            //обнуляем CacheObject и корректируем атрибуты кеша
            resetCacheObject(co);

            co.setObject(obj);
            co.setObjectSize(objSize);

            synchronized(this) {
                _memorySize = _memorySize + objSize;
            }

            tmapPut(co);
        } finally {
            co.unlock();
        }
    }

    /**
     * Возвращает объект из кеша. Если метод вернул null то происходит блокирование
     * объект с переданным objId. После этого в кеш нужно поместить объект с
     * укзаным ранее objId иначе при вызове любого метода будет возникать
     * CacheException.
     * @param objId идентификатор объекта
     * @return Объект возвращается только в том случае, если объект найден
     * и время жизни объекта не закончилось и не превышено время бездействия.
     * @throws CacheException если возникли проблемы
     * @throws NullPointerException если objId==null
     */
    public Object get(Object objId) throws CacheException {
        if(objId==null) {
            throw new NullPointerException("objId is null");
        }

        CacheObject tlCO = (CacheObject)_tl.get();
        if (tlCO!=null) {
            throw new CacheException("Cache:"+_config.getCacheId()+" wait for call put() with objId="+tlCO.getObjectId());
        }

        CacheObject co = getCacheObject(objId);
        co.lock();
        Object o = co.getObject();
        if(o!=null){
            tmapRemove(co);

            if(!valid(co)) {
                resetCacheObject(co);
                //помещаем объект в текущий поток
                _tl.set(co);

                _cacheInfo.incMisses();
                //объект не валидный поэтому локировку не снимаем
                return null;
            } else {
                co.updateStatistics();
                tmapPut(co);

                //объект существует и он валидный поэтому снимаем блокировку с объекта
                co.unlock();
                _cacheInfo.incHits();

                //?:return copy(o);
                return o;
            }
        } else {
            //помещаем объект в текущий поток чтобы потом достать в методе put()
            _tl.set(co);
            _cacheInfo.incMisses();
            //если объекта нет то локировку не снимаем
            return null;
        }
    }

    /**
     * Удаляет объект из кеша.
     * @param objId идентификатор объекта
     * @throws CacheException Если перед вызовом remove() был вызван метод get()
     * и он вернул null то при вызове remove() возникнет CacheException.
     * @throws NullPointerException если objId==null
     */
    public void remove(Object objId) throws CacheException {
        if(objId==null) {
            throw new NullPointerException("objId is null");
        }

        //если за текущим потоком числится объект значит метод put() не был вызван
        //после того как get() вернул null
        CacheObject tlCO = (CacheObject)_tl.get();
        if (tlCO!=null) {
            throw new CacheException("Cache:"+_config.getCacheId()+" wait for call put() with objId="+tlCO.getObjectId());
        }

        CacheObject co = null;//getCacheObject(objId);
        synchronized (this) {
            co = (CacheObject)_map.get(objId);
        }

        if (co==null) {
            return;
        }

        co.lock();
        _cacheInfo.incRemove();
        try {
            synchronized (this) {
                tmapRemove(co);

                //после получения локировки объекта уже межет не быть в кеше
                //или в кеше может быть другой объект с таким же objId
                CacheObject co2 = (CacheObject)_map.get(co.getObjectId());
                if(co2!=null && co2==co){
                    _map.remove(co.getObjectId());
                    resetCacheObject(co);
                }
            }
        } finally {
            co.unlock();
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
     * @throws CacheException если перед вызовом clear() был вызван метод get()
     * и он вернул null то при вызове clear() может возникнуть CacheException.
     */
    public void clear() throws CacheException {
        Object[] objArr = null;
        synchronized (this) {
            objArr = _map.values().toArray();
        }

        //чтобы не возник deadlock нужно удалять не напрямую из _map а через метод remove()
        for (int i = 0, indx = objArr==null ? 0 : objArr.length; i<indx; i++) {
            remove(((CacheObject)objArr[i]).getObjectId());
        }
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
        _tl.set(null);
    }

    /**
     * Выполняет очистку кеша. Удаляются объекты у которых закончилось время
     * жизни или превышен период ожидания или если объект равен null.
     * @throws CacheException если перед вызовом clean() был вызван метод get()
     * и он вернул null то при вызове clean() может возникнуть CacheException.
     */
    public void clean() throws CacheException {
        //объекты из кеша нужно удалять по времени ?
        if(_config.getTimeToLive()==0 && _config.getIdleTime()==0){
            return;
        }

        Object[] objArr = null;
        synchronized (this) {
            objArr = _map.values().toArray();
        }

        for (int i = 0, indx = objArr==null ? 0 : objArr.length; i<indx; i++) {
            CacheObject co = (CacheObject)objArr[i];
            //если объект не валидный то объект нужно удалить
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
     * удаляется один объект в соответсвии с алгоритмом LFU, LRU, FIFO, ...
     */
    private void checkOverflow(int objSize) {
        synchronized(this) {
            //произошло переполнение по количеству или размеру ?
            //если поместили большой объект то, возможно,  из кеша нужно удалить несколько
            //объектов поменьше поэтому while
            while( (_config.getMaxSize() > 0 &&  _map.size()+1>_config.getMaxSize() ) ||
                    (_config.getMaxMemorySize()  > 0 && _memorySize+objSize > _config.getMaxMemorySize())  ) {

                //если в tmap что то есть удаляем первый элемент
                //при прямой сортировке это будет минимальный элемент
                //для LRU это будет самый первый использованый объект
                Object firstKey = _tmap.size()==0 ? null : _tmap.firstKey();
                CacheObject fco = firstKey==null ? null : (CacheObject)_tmap.remove(firstKey);

                if(fco!=null) {
                    //после получения локировки объекта уже межет не быть в кеше
                    //или в кеше может быть другой объект с таким же objId
                    CacheObject co = (CacheObject)_map.get(fco.getObjectId());
                    if(co!=null && co==fco){
                        _map.remove(fco.getObjectId());
                        resetCacheObject(fco);
                    }
                }
            } //while
        }
    }
    /**
     * Удаляет объект из _tmap
     */
    private void tmapRemove(CacheObject co){
        synchronized(this) {
            _tmap.remove(co);
        }
    }
    /**
     * Помещает кешируемый объект в _tmap
     * @param co кешируемый объект
     */
    private void tmapPut(CacheObject co){
        synchronized(this) {
            //ключ и значение, которые содержит _tmap, должны быть идентичны
            //_tmap может содержать только те значения которые есть в _map
            Object mapO = _map.get(co.getObjectId());
            if(mapO!=null && mapO==co){
                _tmap.put(co, co);
            }
        }
    }


    /**
     * Возвращает объект CacheObject для идентификатора objId.
     * Если объекта CacheObject нет то он создаётся.
     * @param objId идентификатор объекта
     */
    private CacheObject getCacheObject(Object objId) {
        synchronized (this) {
            CacheObject co = (CacheObject)_map.get(objId);
            if (co == null) {
                co = _config.newCacheObject(objId);
                _map.put(objId, co);
            }
            return co;
        }
    }
    /**
     * Возвращает true если объект валидный.
     * Проверяется время жизни объекта и время бездействия объекта
     * @param co кешируемый объект
     */
    private boolean valid(CacheObject co){
        long curTime = System.currentTimeMillis();
        return  (_config.getTimeToLive()==0 || (co.getCreateTime()  + _config.getTimeToLive()) >= curTime) &&
                (_config.getIdleTime()==0 || (co.getLastAccessTime() + _config.getIdleTime()) >= curTime) &&
                //если используются soft ссылки то возможна ситуация когда объекта
                //внутри CacheObject может уже не быть
                co.getObject()!=null;
    }
    /**
     *  Если объект был в кеше то:
     *   - корреектируем размер кеша
     *   - удаляем ссылку на объект, обнуляем размер объекта, сбрасываем статистику
     * @param co кешируемый объект
     */
    private void resetCacheObject(CacheObject co){
        synchronized(this) {
            _memorySize = _memorySize - co.getObjectSize();
        }
        co.reset();
    }
// ----------------------------------------------------------------------------- Inner классы
    private class CacheInfoImpl implements CacheInfo {
        private long _hit;
        private long _miss;
        private long _put;
        private long _remove;

        synchronized void incHits(){
            _hit++;
        }
        synchronized void incMisses(){
            _miss++;
        }
        synchronized void incPut(){
            _put++;
        }
        synchronized void incRemove(){
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
            //return "hit:"+_hit+" miss:"+_miss+" memorySize:"+_memorySize+" size:"+_map.size()+" tsize:"+_tmap.size();
        }
    }
}

/*
$Log: BlockingCache.java,v $
Revision 1.1  2010/06/18 17:01:09  smhuang
*** empty log message ***

*/
