/*
 * lru_queue.cpp
 *
 *  Created on: Feb 19, 2017
 *      Author: alex
 */
#include "replacer.h"


LRU_Queue::LRU_Queue(int bufSize,  Frame **frames, unordered_map<PageID, Frame*>* map ):
Replacer(bufSize, frames, map){

	back = front = NULL;

}

void LRU_Queue::constructFrames(){
    /*
    
    
    cout << "in constructFrames\n";

	LinkedFrame* linkedFrames = new LinkedFrame[numFrames];
	for(int f = 0; f < numFrames; ++f){
		frames[f] = linkedFrames + f;
	}


	back = front = linkedFrames;
	back->prev   = NULL;

	for(int f = 1; f < numFrames; ++f){
		front->next = linkedFrames + f;
		front       = front ->next; //Move up front pointer
		front->prev = linkedFrames + f - 1;
	}
	front->next = NULL;

	freeFrames = numFrames;

*/
}
inline void LRU_Queue::RemoveFront(){

	front  = front->prev;
	if(--freeFrames == 0) back =  NULL;

	else front->next = NULL; //We would get segmentation fault if we try do this when front == NULL

}

inline void LRU_Queue::RemoveBack(){
	back   = back->next;
	if(--freeFrames == 0) front = NULL;

	else back->prev = NULL;

}

inline void LRU_Queue::RemoveMiddle(LinkedFrame* frame){
	(frame->prev)->next = frame->next;
	--freeFrames;
}

/*Puts a frame in the front of the queue (as if it were a stack instead) */
inline void LRU_Queue::PushFront(LinkedFrame* frame){
         frame->prev = front;  frame->next = NULL;

         front = frame;

         if(++freeFrames == 1) back = frame; //If queue has just one element; the back = the front
         else (front->prev)->next = front;

}

	/*Enqueues a frame on the linked queue*/
inline void LRU_Queue::PushBack(LinkedFrame* toAdd){

	toAdd->next = back; toAdd->prev = NULL;
	back = toAdd;


	if(freeFrames == 0) front  = toAdd; //If queue is empty, the back is also the front

	else (toAdd->next)->prev = toAdd; //Set the next element's prev pointer to the new back of the queue

	++freeFrames;

}

Frame* LRU_Queue::NextFrame(){

	if(freeFrames == 0) return NULL; //This means we are out of frames

	LinkedFrame* nextFrame = front; //We will give the frame on top of queue to the buffer manager so it can allocate it for a use

	front  = front->prev;
	if(--freeFrames == 0) back =  NULL;

	else front->next = NULL;

	return nextFrame;
}



/*When a frame is freed we place it back in the queue*/
void LRU_Queue::FreeFrame(Frame* frame){
	LinkedFrame * toAdd = static_cast<LinkedFrame*>(frame);

	/*An optimization, if a frame is empty we add it to the front of queue instead so we can keep the frames with actual data longer */
	if(!frame->hasValidPID()){PushFront(toAdd); return;}



	toAdd->next = back; toAdd->prev = NULL;
	back = toAdd;


	if(freeFrames == 0) front  = toAdd; //If queue is empty, the back is also the front

	else (toAdd->next)->prev = toAdd; //Set the next element's prev pointer to the new back of the queue

	++freeFrames;

}


void LRU_Queue::ResurrectFrame(Frame* frame){
	LinkedFrame* f = static_cast<LinkedFrame*>(frame);

	if(f == front)	RemoveFront();

	else if(f == back) RemoveBack();

	else RemoveMiddle(f);

}

/*Moves the frame to the front of the queue so it can be chosen next as its no longer needed */
void LRU_Queue::ClearFrame(Frame * frame){
LinkedFrame* linkedf = static_cast<LinkedFrame*>(frame);

	if(linkedf == front)	RemoveFront();

	else if(linkedf == back) RemoveBack();

	else RemoveMiddle(linkedf);

	PushFront(linkedf); //Push it to the front of the queue

}
