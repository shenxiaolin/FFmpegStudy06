/**
 * 这个对列主要用于存放AVpacket 的指针
 * 使用生产者消费者来使用对列，音频Avpacket 的Queue ,视频的Queue
 * 3个线程：
 * 生产者线程：readStream 不断的读然后分别放入视频和音频的Queue
 * 消费者线程：1.不断重视频Queue 获取AVpacket ，解码，绘制
 *              2.不断重音频Queue 获取AVpacket ，解码，播放
 * */
#include <malloc.h>

#include "vaqueue.h"
struct _AVQueue{
    int size;

    //要保存多个AVpacket，所以要用到数组，因为想通用。所以使用void** 代表任意类型
    void** tab;
    //用于记录下一次要读写的位置
    int next_to_write;
    int next_to_read;

};
/**
 * 使用回调函数来开辟元素的内存
 * init queue
 * @param size
 * @return
 */
AVQueue* queue_init(int size,queue_fill_fun fill_fun){
    AVQueue *avQueue = (AVQueue *) malloc(sizeof(AVQueue));
    avQueue->size =size;
    avQueue->next_to_read=0;
    avQueue->next_to_write =0;
    avQueue->tab = (void **) malloc(sizeof(*avQueue->tab) * size);
    for (int i = 0; i < size; ++i) {
        avQueue->tab[i] =fill_fun();
    }
    return avQueue;
}

/**
 * free queue
 * @param queue
 */
void queue_free(AVQueue *queue,queue_free_fun free_fun){
    int size = queue->size;
    for (int i = 0; i < size; ++i) {
        free_fun((void *) queue->tab[i]);
    }
    free(queue->tab);
    free(queue);
}
int get_next(AVQueue *queue,int current){
    return (current +1)%queue->size;
}
//压入对列
void *queue_push(AVQueue *queue, pthread_mutex_t *mutex, pthread_cond_t *cond) {
    int current = queue->next_to_write;
    int next_to_write;
    for(;;){
        next_to_write =get_next(queue,current);
        if (next_to_write !=queue->next_to_read){
            break;
        }
        //阻塞
        pthread_cond_wait(cond,mutex);
    }
    queue->next_to_write=next_to_write;
    //通知
    pthread_cond_broadcast(cond);
    return queue->tab[current];
}
//弹出对列
void *queue_pop(AVQueue *queue, pthread_mutex_t *mutex, pthread_cond_t *cond) {
    int current = queue->next_to_read;
    for(;;){
        if (queue->next_to_read !=queue->next_to_write){
            break;
        }
        //阻塞
        pthread_cond_wait(cond,mutex);
    }
    queue->next_to_read=get_next(queue,current);
    pthread_cond_broadcast(cond);
    return queue->tab[current];
}


