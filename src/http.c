//
// Created by Lincong Li on 1/12/18.
//

#include "http.h"
#include "message.h"


///*
//  _     _____ _____ ____    _____  _    ____  _  __
// | | | |_   _|_   _|  _ \  |_   _|/ \  / ___|| |/ /
// | |_| | | |   | | | |_) |   | | / _ \ \___ \| ' /
// |  _  | | |   | | |  __/    | |/ ___ \ ___) | . \
// |_| |_| |_|   |_| |_|       |_/_/   \_\____/|_|\_\
//
//*/
//
//http_task_t* create_http_task()
//{
//    return (http_task_t*) malloc(1*sizeof(http_task_t));
//}
//
//void destroy_http_task(http_task_t* http_task)
//{
//    if(http_task != NULL)
//        free(http_task);
//}

/*
 _                _    _
| |              | |  | |
| |__   __ _  ___| | _| | ___   __ _    __ _ _   _  ___ _   _  ___
| '_ \ / _` |/ __| |/ / |/ _ \ / _` |  / _` | | | |/ _ \ | | |/ _ \
| |_) | (_| | (__|   <| | (_) | (_| | | (_| | |_| |  __/ |_| |  __/
|_.__/ \__,_|\___|_|\_\_|\___/ \__, |  \__, |\__,_|\___|\__,_|\___|
                                __/ |     | |
                               |___/      |_|
*/
//
//// Create an empty queue
//void create_http_backlog(http_backlog_t* backlog)
//{
//    backlog->front = backlog->rear = NULL;
//    backlog->size = 0;
//}
//
//void destroy_http_backlog(http_backlog_t* backlog)
//{
//    while(!is_http_backlog_empty(backlog))
//        deq_http_backlog(backlog);
//    assert(backlog->size == 0);
//}
//
//// Returns queue size
//size_t http_backlog_size(http_backlog_t* backlog)
//{
//    return backlog->size;
//}
//
//// Enqueing the queue
//void enq_http_backlog(http_task_t* new_http_task, http_backlog_t* backlog)
//{
//    if (backlog->rear == NULL)
//    {
//        backlog->rear = (struct backlog_request *)malloc(1*sizeof(struct backlog_request));
//        backlog->rear->next = NULL;
//        backlog->rear->http_task = new_http_task;
//        backlog->front = backlog->rear;
//
//    } else {
//        backlog->temp = (struct backlog_request *)malloc(1*sizeof(struct backlog_request));
//        backlog->rear->next = backlog->temp;
//        backlog->temp->http_task = new_http_task;
//        backlog->temp->next = NULL;
//        backlog->rear = backlog->temp;
//    }
//    backlog->size++;
//}
//
//// Dequeing the queue
//int deq_http_backlog(http_backlog_t* backlog)
//{
//    backlog->front1 = backlog->front;
//
//    if (backlog->front1 == NULL)
//    {
//        printf("\n Error: Trying to display elements from empty queue");
//        assert(backlog->size == 0);
//        assert(backlog->rear == NULL);
//        return EXIT_FAILURE;
//
//    } else if (backlog->front1->next != NULL) {
//        assert(backlog->size > 1);
//        backlog->front1 = backlog->front1->next;
//
//        // destroy the http task first
//        destroy_http_task(backlog->front->http_task);
//        // destroy the current node storing the http task
//        free(backlog->front);
//        backlog->front = backlog->front1;
//
//    } else { // only one node
//        assert(backlog->size == 1);
//
//        // destroy the http task first
//        destroy_http_task(backlog->front->http_task);
//        // destroy the current node storing the http task
//        free(backlog->front);
//        backlog->front = NULL;
//        backlog->rear = NULL;
//
//    }
//    backlog->size--;
//    return EXIT_SUCCESS;
//}
//
//// Returns the front element of queue
//http_task_t* peek_http_backlog(http_backlog_t* backlog)
//{
//    if ((backlog->front != NULL) && (backlog->rear != NULL)) {
//        return (backlog->front->http_task);
//    } else {
//        assert(backlog->size == 0);
//        return NULL;
//    }
//}
//
///* Display if queue is empty or not */
//bool is_http_backlog_empty(http_backlog_t* backlog)
//{
//    return ((backlog->front == NULL) && (backlog->rear == NULL));
//}


/*
  _   _ _____ _____ ____    ____                            _     _                     _ _
 | | | |_   _|_   _|  _ \  |  _ \ ___  __ _ _   _  ___  ___| |_  | |__   __ _ _ __   __| | | ___  ___
 | |_| | | |   | | | |_) | | |_) / _ \/ _` | | | |/ _ \/ __| __| | '_ \ / _` | '_ \ / _` | |/ _ \/ __|
 |  _  | | |   | | |  __/  |  _ <  __/ (_| | |_| |  __/\__ \ |_  | | | | (_| | | | | (_| | |  __/\__ \
 |_| |_| |_|   |_| |_|     |_| \_\___|\__, |\__,_|\___||___/\__| |_| |_|\__,_|_| |_|\__,_|_|\___||___/
                                         |_|
*/

int handle_http(peer_t *peer)
{
    /*
       // process input part
  1. read all data from the recv buffer and process it
  2. generate Request
  3. Use Request object to generate Response object
  4. Put the response object in the back_log (queue)

  // generate output part
  5. check the state of the current "http_task"
  6. do corresponding actions
  7. if the current "http_task" is finished. Get another one from the queue and repeat from step 5 until the send buffer is filled up
  8. return
     */
    http_task_t* curr_task = peer->http_task;
    if(curr_task == NULL){
        fprintf(stderr, "When handle_http() is called, http_task should have been allocated");
        exit(EXIT_FAILURE);
    }

    // RECV_HEADER_STATE -> RECV_BODY_STATE -> GENERATE_HEADER_STATE ->
    // SEND_HEADER_STATE -> SEND_BODY_STATE -> FINISHED_STATE
    while(curr_task->state != FINISHED_STATE){
        if(curr_task->state == RECV_HEADER_STATE){


            curr_task->state = RECV_BODY_STATE;

        }else if(curr_task->state == RECV_BODY_STATE){

            curr_task->state = GENERATE_HEADER_STATE;

        }else if(curr_task->state == GENERATE_HEADER_STATE){

            curr_task->state = SEND_HEADER_STATE;

        }else if(curr_task->state == SEND_HEADER_STATE){

            curr_task->state = SEND_BODY_STATE;

        }else if(curr_task->state == SEND_BODY_STATE){

            curr_task->state = FINISHED_STATE;
        }else{
            fprintf(stderr, "Shouldn't get to this unknown state");
            exit(EXIT_FAILURE);
        }
    }

    return CLOSE_CONN;
}