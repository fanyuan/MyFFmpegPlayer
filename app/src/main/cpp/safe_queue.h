//
// Created by develop on 2021/2/14.
//

#ifndef MYFFMPEGVIDEO_SAFE_QUEUE_H
#define MYFFMPEGVIDEO_SAFE_QUEUE_H
#include <queue>
#include <pthread.h>

using namespace std;

template <typename T>
class SafeQueue{
    typedef void (*ReleaseCallback)(T *);
    typedef void (*SyncHandle)(queue<T> &);

public:
    SafeQueue(){
        pthread_mutex_init(&mutex,0);//动态初始化
        pthread_cond_init(&cond,0);
    }
    ~SafeQueue(){
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }
    /**
     * 入队
     * @param value
     */
    void push(T value){
        //先加锁
        pthread_mutex_lock(&mutex);
        if(work){
            //工作状态需要push
            q.push(value);
            pthread_cond_signal(&cond);
        } else{
            //非工作状态
            if(releaseCallback){
                releaseCallback(&value);
            }
        }
        //再解锁
        pthread_mutex_unlock(&mutex);
    }
    /**
     * 出队
     * @param value
     * @return
     */
    int pop(T &value){
        int ret = 0;
        //先加锁
        pthread_mutex_lock(&mutex);

        while (work && q.empty()){
            //工作状态，说明需要pop,但是队列为空，需等待
            pthread_cond_wait(&cond,&mutex);
        }
        if(!q.empty()){
            value = q.front();
            //弹出
            q.pop();
            ret = 1;
        }

        //再解锁
        pthread_mutex_unlock(&mutex);
        return ret;
    }
    /**
     * 设置队列的工作状态
     * @param work
     */
    void setWork(int work) {
        //先加锁
        pthread_mutex_lock(&mutex);
        this->work = work;
        pthread_cond_signal(&cond);
        //后解锁
        pthread_mutex_unlock(&mutex);
    }
    /**
     * 判断队列是不是为空
     * @return
     */
    int empty(){
        return q.empty();
    }
    /**
     * 获取队列大小
     * @return
     */
    int size(){
        return q.size();
    }
    /**
     * 清空队列
     * 队列中的元素释放需要注意
     * AVPacket
     */
    void clear(){
        //先加锁
        pthread_mutex_lock(&mutex);
        unsigned  int size = q.size();
        for(int i = 0;i < size;++i){
            //取出队首元素
            T value = q.front();
            if(releaseCallback){
                releaseCallback(&value);
            }
            q.pop();
        }

        //后解锁
        pthread_mutex_unlock(&mutex);
    }

    void setReleaseCallback(ReleaseCallback releaseCallback) {
        this->releaseCallback = releaseCallback;
    }

    void setSyncHandle(SyncHandle syncHandle) {
        this->syncHandle = syncHandle;
    }
    void sync(){
        //先加锁
        pthread_mutex_lock(&mutex);
        syncHandle(q);
        //后解锁
        pthread_mutex_unlock(&mutex);
    }

private:
    queue<T> q;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    /**
     * 标记队列是否工作
     */
    int work;
    ReleaseCallback releaseCallback;
    SyncHandle syncHandle;
};
#endif //MYFFMPEGVIDEO_SAFE_QUEUE_H
