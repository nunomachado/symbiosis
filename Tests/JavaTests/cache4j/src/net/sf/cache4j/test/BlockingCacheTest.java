/* =========================================================================
 * File: $Id: BlockingCacheTest.java,v 1.1 2010/06/18 17:01:16 smhuang Exp $BlockingCacheTest.java,v$
 *
 * Copyright 2006 by Yuriy Stepovoy.
 * email: stepovoy@gmail.com
 * All rights reserved.
 *
 * =========================================================================
 */

package net.sf.cache4j.test;

import net.sf.cache4j.CacheFactory;
import net.sf.cache4j.CacheConfig;
import net.sf.cache4j.CacheException;
import net.sf.cache4j.Cache;
import net.sf.cache4j.impl.BlockingCache;
import net.sf.cache4j.impl.CacheConfigImpl;

import java.util.Random;

/**
 * Класс BlockingCacheTest тестирует BlockingCache
 *
 * @version $Revision: 1.1 $ $Date: 2010/06/18 17:01:16 $
 * @author Yuriy Stepovoy. <a href="mailto:stepovoy@gmail.com">stepovoy@gmail.com</a>
 **/

public class BlockingCacheTest implements Test {
// ----------------------------------------------------------------------------- Константы
// ----------------------------------------------------------------------------- Атрибуты класса
// ----------------------------------------------------------------------------- Статические переменные
// ----------------------------------------------------------------------------- Конструкторы
// ----------------------------------------------------------------------------- Public методы

    //-------------------------------------------------------------------------- Test interface
    /**
     * Вызывается перед началом тестирования
     * @throws java.lang.Exception
     */
    public void init() throws Exception {
    }

    /**
     * Вызывается после выполнения каждого тестового метода
     * @throws java.lang.Exception
     */
    public void afterEachMethod() throws Exception {
        //очищаем CacheFactory
        CacheFactory cf = CacheFactory.getInstance();
        Object[] cacheIds = cf.getCacheIds();
        for (int i = 0, indx = cacheIds==null ? 0 : cacheIds.length; i <indx; i++) {
            cf.removeCache(cacheIds[i]);
        }
    }

    /**
     * Вызывается после тестирования
     * @throws java.lang.Exception
     */
    public void destroy() throws Exception {
    }
    //-------------------------------------------------------------------------- Test interface


    /**
     * Помещаем объект и достаём его из кеша.
     */
    public static boolean test_PUT_GET() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, 0, 0, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        Object key = "key";
        Object value = "value";
        cache.put(key, value);

        return cache.get(key)!=null ? true : false;
    }

    /**
     * Помещаем объект. Помещаем null. Результат должен быть null.
     */
    public static boolean test_PUT_OBJ_PUT_NULL_GET() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, 0, 0, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        Object key = "key";
        Object value = "value";

        cache.put(key, value);
        cache.put(key, null);

        return cache.get(key)==null ? true : false;
    }

    /**
     * Помещаем объект. Удаляем объект. Результат должен быть null.
     */
    public static boolean test_PUT_REMOVE_GET() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, 0, 0, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        Object key = "key";
        Object value = "value";

        cache.put(key, value);
        cache.remove(key);

        return cache.get(key)==null ? true : false;
    }

    /**
     * Помещаем объект. Очищаем кеш. Результат должен быть null.
     */
    public static boolean test_PUT_CLEAR_GET() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, 0, 0, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        Object key = "key";
        Object value = "value";

        cache.put(key, value);
        cache.clear();

        return cache.get(key)==null ? true : false;
    }

    /**
     * Устанавливаем максимальное количество элементов в кеше. Помещаем больше
     * чем установили. Результатом должно быть максимальное количество объектов.
     */
    public static boolean test_MAXSIZE() throws Exception {
        BlockingCache cache = new BlockingCache();
        int maxSize = 100;
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, 0, maxSize, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        for (int i = 0, maxi = maxSize*2; i <maxi; i++) {
            cache.put(new Long(i), new Long(i));
        }

        return cache.size()==maxSize ? true : false;
    }

    /**
     * Устанавливаем максимальный размер памяти занимаемый объектами кеша.
     * Помещаем столько столько объектов чтобы они превысили максимальный размер.
     * В результате объекты в кеше должны занимать объекм не превышающий
     * максимальный.
     */
    public static boolean test_MAXMEMORYSIZE() throws Exception {
        BlockingCache cache = new BlockingCache();
        int maxMemorySize = 1000;
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, maxMemorySize, 0, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        for (int i = 0; i <1000; i++) {
            cache.put(new Long(i), new Long(i));
        }

        return cache.getCacheInfo().getMemorySize()<=maxMemorySize ? true : false;
    }

    /**
     * Устанавливаем максимальное количество и максимальный объём объектов в кеше.
     * Помещаем столько объектов чтобы они превысили максимальное количество и объем.
     * В результате количество объектов и объем не должны превышать максимальные
     * значения.
     */
    public static boolean test_MAXSIZE_AND_MAXMEMORYSIZE() throws Exception {
        BlockingCache cache = new BlockingCache();
        int maxMemorySize = 1000;
        int maxSize = 1000;
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, maxMemorySize, maxSize, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        for (int i = 0; i <maxSize*2; i++) {
            cache.put(new Long(i), new Long(i));
        }

        return cache.getCacheInfo().getMemorySize()<=maxMemorySize && cache.size()<=maxSize ? true : false;
    }

    /**
     * Устанавливаем максимальное время жизни объекта. Помещаем объект.
     * Останавливаем поток на время превышающее максимальное время.
     * В результате кеш должен вернуть null.
     */
    public static boolean test_TTL() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 1, 0, 0, 0, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        Object key = "key";
        Object value = "value";

        cache.put(key, value);
        Thread.sleep(30);

        return cache.get(key)==null ? true : false;
    }

    /**
     * Устанавливаем максимальное время бездействия объекта. Помещаем объект.
     * Останавливаем поток на время превышающее максимальное время.
     * В результате кеш должен вернуть null.
     */
    public static boolean test_IDLE() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 1, 0, 0, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        Object key = "key";
        Object value = "value";

        cache.put(key, value);
        Thread.sleep(30);

        return cache.get(key)==null ? true : false;
    }

    /**
     * Проверяем как работает атоматическая очистка объектов у которых закончилось
     * время жизни. Добавляем объекты с максимальным временем жизни Х,
     * устанавливаем интервал очистки X*2, усыпляем текущий поток на Х*3.
     * В результате кеш должен быть полность пустой.
     */
    public static boolean test_CACHE_CLEANER_TTL() throws Exception {
        CacheFactory cf = CacheFactory.getInstance();
        long ttl = 100;
        long cleanInterval = ttl*2;
        long sleep = ttl*3;

        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, ttl, 0, 0, 0, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        for (int i = 0; i <1000; i++) {
            cache.put(new Long(i), new Long(i));
        }

        cf.addCache(cache);
        cf.setCleanInterval(cleanInterval);
        Thread.sleep(sleep);

        return cache.size()==0 ? true : false;
    }

    /**
     * Проверяем как работает атоматическая очистка объектов у которых закончилось
     * время бездействия. Добавляем объекты с максимальным временем бездействия Х,
     * устанавливаем интервал очистки X*2, усыпляем текущий поток на Х*3.
     * В результате кеш должен быть полность пустой.
     */
    public static boolean test_CACHE_CLEANER_IDLE() throws Exception {
        CacheFactory cf = CacheFactory.getInstance();
        long idle = 100;
        long cleanInterval = idle*2;
        long sleep = idle*3;

        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, idle, 0, 0, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        for (int i = 0; i <1000; i++) {
            cache.put(new Long(i), new Long(i));
        }

        cf.addCache(cache);
        cf.setCleanInterval(cleanInterval);
        Thread.sleep(sleep);

        return cache.size()==0 ? true : false;
    }

    /**
     * Проверяем алгоритм вытеснения LRU. При переполнении кеша должен удалиться
     * объект с наименьшим временем использования.
     */
    public static boolean test_EVICTION_ALGORITHM_LRU() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, 0, 2, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        Object o1 = "o1";
        Object o2 = "o2";
        Object o3 = "o3";

        cache.put(o1, o1);
        Thread.sleep(50);
        cache.put(o2, o2);
        Thread.sleep(50);
        cache.get(o1); //update access time

        cache.put(o3, o3);

        //первый объект o1 access time : 12
        //второй объект is null access time:2
        //третий объект o3 time:13
        return cache.get(o1)!=null &&
               cache.get(o3)!=null &&
               cache.get(o2)==null ? true : false;
    }

    /**
     * Проверяем алгоритм вытеснения LFU. При переполнении кеша должен удалиться
     * объект который использовался наименьшее количество раз.
     */
    public static boolean test_EVICTION_ALGORITHM_LFU() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, 0, 2, null, "lfu", "strong");
        cache.setCacheConfig(cacheConfig);

        Object o1 = "o1";
        Object o2 = "o2";
        Object o3 = "o3";

        cache.put(o1, o1);
        cache.get(o1); //update frequency count
        Thread.sleep(50);
        cache.put(o2, o2);

        cache.put(o3, o3);

        //первый объект o1 frequency count:2
        //второй объект is null frequency count:1
        //третий объект o3 frequency count:1
        return cache.get(o1)!=null &&
               cache.get(o3)!=null &&
               cache.get(o2)==null ? true : false;
    }

    /**
     * Проверяем алгоритм вытеснения FIFO. При переполнении кеша должен удалиться
     * объект с наименьшим временем создания.
     */
    public static boolean test_EVICTION_ALGORITHM_FIFO() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, 0, 2, null, "fifo", "strong");
        cache.setCacheConfig(cacheConfig);

        Object o1 = "o1";
        Object o2 = "o2";
        Object o3 = "o3";

        cache.put(o1, o1); //create time 1
        Thread.sleep(50);
        cache.put(o2, o2); //create time 2
        Thread.sleep(50);
        cache.put(o3, o3); //create time 3

        return cache.get(o2)!=null &&
               cache.get(o3)!=null &&
               cache.get(o1)==null ? true : false;
    }

    /**
     * Проверяем тип связи с объектом strong. При помещении большого количества
     * объектов должно вылететь исключение OutOfMemoryError.
     */
    public static boolean test_REFERENCE_STRONG() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, 0, 0, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        int i = 0;
        try {
            for (; i <10; i++) {
                cache.put(new Integer(i), new Long[2048*2048]);
            }
        } catch (OutOfMemoryError o){
            //int size = cache.size();
            cache.clear();
            return true; //return size==i;
        }

        return false;
    }

    /**
     * Проверяем тип связи с объектом soft. При помещении большого количества
     * объектов исключение OutOfMemoryError не должно вылетать.
     */
    public static boolean test_REFERENCE_SOFT() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, 0, 0, null, "lru", "soft");
        cache.setCacheConfig(cacheConfig);

        int i = 0;
        try {
            for (; i <10; i++) {
                cache.put(new Integer(i), new Long[2048*2048]);
            }
        } catch (OutOfMemoryError o){
            cache.clear();
            return false;
        }

        return cache.size()==i;
    }

    /**
     * Проверяем способность кеша ругаться на неправильные действия пользователя.
     * Если запрошен объект и кеш вернул null то в кеш нужно поместить объект с
     * ключом переданным в методе get(). Если помещать объект с другим ключом,
     * доставать объект, удалять объект то кеш должен кидать исключение.
     */
    public static boolean test_BLOCKING() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, 0, 0, null, "lru", "soft");
        cache.setCacheConfig(cacheConfig);

        cache.get("1");
        int exceptCount = 0;

        try {
            cache.get("2");
        } catch (CacheException o){
            exceptCount++; //+
        }

        try {
            cache.put("3", "3");
        } catch (CacheException o){
            exceptCount++; //+
        }

        try {
            cache.remove("4");
        } catch (CacheException o){
            exceptCount++; //+
        }

        try {
            cache.put("1", "1"); //помещаем объект
        } catch (CacheException o){
            exceptCount++;
        }

        return exceptCount==3;
    }

    /**
     * Проверяем работу кеша с потоками
     */
    public static boolean test_THREAD1() throws Exception {
        BlockingCache cache = new BlockingCache();
        CacheConfig cacheConfig = new CacheConfigImpl("cacheId", null, 0, 0, 0, 1000, null, "lru", "strong");
        cache.setCacheConfig(cacheConfig);

        int tcount = 2;
        int count = 2;
        for (int i = 0; i <tcount; i++) {
            new TThread(cache, count).start();
        }

        while(_threadCount!=0){
            Thread.currentThread().yield();
            Thread.currentThread().sleep(1);
        }
        return true;
    }

// ----------------------------------------------------------------------------- Package scope методы
// ----------------------------------------------------------------------------- Protected методы
// ----------------------------------------------------------------------------- Private методы
// ----------------------------------------------------------------------------- Inner классы

    private static int _threadCount;
    private static long _id;
    synchronized static void incThreadCount(){
        _threadCount++;
    }
    synchronized static void decThreadCount(){
        _threadCount--;
    }
    synchronized static long nextId(){
        return _id++;
    }
    static class TThread extends Thread {
        private Cache _cache;
        private long _count;
        private Random _rnd = new Random(nextId());
        public TThread(Cache cache, long count) {
            incThreadCount();
            _cache = cache;
            _count = count;
        }

        public void run() {
            long count = 0;
            try {
                while(count<_count){
                    count++;
                    Object key = new Long(_rnd.nextInt(1500));
                    _cache.get(key);
                    _cache.put(key, key);
                    //_cache.remove(key);
                }
            } catch (Exception e){
                throw new RuntimeException(e.getMessage());
            } finally {
                decThreadCount();
            }
        }
    }

}

/*
$Log: BlockingCacheTest.java,v $
Revision 1.1  2010/06/18 17:01:16  smhuang
*** empty log message ***

*/
