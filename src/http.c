//
// Created by Lincong Li on 1/12/18.
//

#include "http.h"

/* Create an empty queue */
void create(backlog_t* backlog)
{
    backlog->front = backlog->rear = NULL;
}

/* Returns queue size */
size_t backlog_size(backlog_t* backlog)
{
    return backlog->size;
}

/* Enqueing the queue */
void enq(http_task_t* new_http_task, backlog_t* backlog)
{
    if (rear == NULL)
    {
        backlog->rear = (requests_backlog_t *)malloc(1*sizeof(backlog_request_t));
        backlog->rear->next = NULL;
        backlog->rear->http_task = new_http_task;
        front = rear;

    } else {
        backlog->temp=(requests_backlog_t *)malloc(1*sizeof(backlog_request_t));
        backlog->rear->next = temp;
        backlog->temp->http_task = new_http_task;
        backlog->temp->next = NULL;
        rear = temp;
    }
    backlog->size++;
}

/* Dequeing the queue */
int deq(backlog_t* backlog)
{
    backlog->front1 = backlog->front;

    if (backlog->front1 == NULL)
    {
        printf("\n Error: Trying to display elements from empty queue");
        return EXIT_FAILURE;
    }
    else
    if (backlog->front1->next != NULL)
    {
        backlog->front1 = backlog->front1->next;
        printf("\n Dequed value : %d", front->info);
        free(backlog->front);
        backlog->front = backlog->front1;
    }
    else
    {
        printf("\n Dequed value : %d", backlog->front->http_task);
        free(backlog->front);
        backlog->front = NULL;
        backlog->rear = NULL;
    }
    backlog->szie--;
}

// Returns the front element of queue
http_task_t* peek(backlog_t* backlog)
{
    if ((front != NULL) && (rear != NULL))
        return(backlog->front->http_task);
    else
        return NULL;
}

/* Display if queue is empty or not */
bool is_empty(backlog_t* backlog)
{
    return ((backlog->front == NULL) && (backlog->rear == NULL));
}

int handle_http(peer_t *peer)
{
    return CLOSE_CONN;
}