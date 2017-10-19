/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"


/**
  Initializes the priqueue_t data structure.

  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{
	q->comp = comparer;
	q->length = 0;
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{

	if( 0 == q->length ) {
		q->length = 1;
		q->arr[0] = ptr;
		return 0;
	}

	int newLength = q->length+1;
	int next = 0;
	int ret = -1;
	char done = 0;
	void **tempArr = malloc(sizeof(void *[newLength]));

	for(int i = 0; i < q->length; i++){
		if(!done){
			if(q->comp(ptr, q->arr[i]) > 0){
				tempArr[next] = ptr;
				ret = next;
				next++;

				done = 1;
			}
		}

		tempArr[next] = q->arr[i];
		next++;
	}

	if(!done){
		tempArr[next] = ptr;
		ret = next;
	}

	q->length = newLength;
	free(q->arr);
	q->arr = tempArr;
	return ret;
}


/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
	if(0 == q->length){
		return NULL;
	}

	return q->arr[0];
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
	void *tempPtr;

	if (0 == q->length) {
		return NULL;

	} else if(1 == q->length){
		tempPtr = q->arr[0];
		free(q->arr);
		q->length = 0;
		return tempPtr;
	}

	void **tempArr = malloc(sizeof(void *[q->length -1]));
	for(int i = 0; i < q->length -1; i++){
		tempArr[i] = q->arr[i+1];
	}

	tempPtr = q->arr[0];
	free(q->arr);
	q->length = q->length -1;
	q->arr = tempArr;
	return tempPtr;
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
	if(index <= q->length){
		return NULL;
	}

	return q->arr[index];
}


/**
  Removes all instances of ptr from the queue.

  This function should not use the comparer function, but check if the data contained in each element of the queue is equal (==) to ptr.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{
	int ret = 0;
	int next = 0;

	for(int i = 0 ; i < q->length; i++){
		if(q->arr[i] == ptr){
			q->arr[i] = NULL;
			ret++;
		}
	}

	if(ret > 0){
		if(q->length - ret != 0){
			void **tempArr = malloc(sizeof(void *[q->length - ret]));

			for(int i = 0; i < q->length; i++){ 		//Not the most efficient way to do this
				if(q->arr[i] != NULL){
					tempArr[next] = q->arr[i];
					next++;
				}
			}

			q->length = next;
			free(q->arr);
			q->arr = tempArr;
		}else {
			q->length = 0;
			free(q->arr);
		}
	}

	return ret;
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{
	void *tempPtr;

	if(index >= q->length){
		return NULL;
	} else if(1 == q->length){
		tempPtr = q->arr[0];
		free(q->arr);
		q->length = 0;
		return tempPtr;
	}

	void **tempArr = malloc(sizeof(void *[q->length -1]));
	int next = 0;

	for(int i = 0; i < q->length; i++){
		if(i != index){
			tempArr[next] = q->arr[i];
			next++;
		} else {
			tempPtr = q->arr[i];
		}
	}

	q->length = next;
	free(q->arr);
	q->arr = tempArr;
	return tempPtr;
}


/**
  Returns the number of elements in the queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
	return q->length;
}


/**
  Destroys and frees all the memory associated with q.

  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
	free(q->arr);
	free(q);
}
