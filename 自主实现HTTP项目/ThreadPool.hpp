#pragma once

#include<queue>
#include<pthread.h>
#include"Task.hpp"
#include"Log.hpp"

#define NUM 6

class ThreadPool{
    private:
        int num;//创建的线程数量
        bool stop;//用来判断线程池是否退出
        std::queue<Task> Task_queue;//任务队列
        pthread_mutex_t lock;
        pthread_cond_t cond;

    //单例模式
    private:
        ThreadPool(int _num=NUM):num(_num),stop(false){
            pthread_mutex_init(&lock,nullptr);
            pthread_cond_init(&cond,nullptr);
        }

        ThreadPool(const ThreadPool&){

        }

        static ThreadPool* single_instance;//单例对象指针
    public:
        static ThreadPool* GetInstance(){
            if(single_instance==nullptr){
                pthread_mutex_t _mutex=PTHREAD_MUTEX_INITIALIZER;
                pthread_mutex_lock(&_mutex);
                if(single_instance==nullptr){
                    single_instance=new ThreadPool();
                    if(single_instance==nullptr){
                        LOG(FATAL,"ThradPool create failed!");
                        exit(5);
                    }
                    single_instance->InitThreadPool();
                    LOG(INFO,"Threadpool create success");
                }
                pthread_mutex_unlock(&_mutex);
            }
            return single_instance;
        }

        bool IsStop(){
            return stop;
        }

        bool TaskQueueIsEmpty(){
            return Task_queue.size()==0?true:false;
        }

        void Lock(){
            pthread_mutex_lock(&lock);
        }

        void UnLock(){
            pthread_mutex_unlock(&lock);
        }

        void ThreadWait(){
            pthread_cond_wait(&cond,&lock);
        }

        void ThreadWakeUp(){
            pthread_cond_signal(&cond);
        }

        static void*ThreadRoutine(void*args){
            ThreadPool*tp=(ThreadPool*)args;
            while(true){
                Task t;
                tp->Lock();
                if(tp->stop)break;
                while(tp->TaskQueueIsEmpty()){
                    //用while循环是因为当该线程被唤醒后一定是占有锁的
                    //被唤醒后有可能条件并不满足(比如广播所有线程，但任务只有一个)，需要再次判断
                    tp->ThreadWait();
                }
                tp->PopTask(t);
                tp->UnLock();
                t.ProcessOn();
            }
        }

        bool InitThreadPool(){
            for(int i=0;i < num;++i){
                pthread_t tid;
                if(pthread_create(&tid,nullptr,ThreadRoutine,this)!=0){//成功返回0
                    LOG(FATAL,"multi pthread create fail!!!");
                    return false;
                }
                pthread_detach(tid);//线程分离
            }
            LOG(INFO,"multi pthead create success");
            return true;
        }

        void PushTask(const Task& task){
            Lock();
            Task_queue.push(task);
            UnLock();
            ThreadWakeUp();
        }

        void PopTask(Task &task){
            task=Task_queue.front();
            Task_queue.pop();
        }
        ~ThreadPool(){
            pthread_mutex_destroy(&lock);
            pthread_cond_destroy(&cond);
        }

};

ThreadPool* ThreadPool::single_instance=nullptr;
