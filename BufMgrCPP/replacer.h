#include "frame.h"
#include "linkedframe.h"
#include "unordered_map"
/**
 * Class to implement the buffer replacement policy.
 * There is only one main function to be implemented, PickVictim() which returns an integer corresponding to the frame to be
 * replaced. Remember, when a page in a frame is replaced, you need to write the page back to the disk if the page has been
 * modified. It is up to you to decide whether this is a replacer's responsibility or this has to be left to some other classes.
 *
 * Here we have defined a Clock replacer which should implement the clock replacement policy. Feel free to modify this interface,
 * or add any other replacement policy as you like
 *
 */
class Replacer
{
	protected:
		int numFrames;
	    Frame** frames;
		unordered_map<PageID, Frame*>* map;
		int  freeFrames;


	public :

		Replacer(int bufSize, Frame **frames,  unordered_map<PageID, Frame*>* map ){
			this->numFrames = bufSize; this->frames = frames; this->map = map; this->freeFrames = 0;

		}
		virtual ~Replacer() {;}
		virtual Frame* NextFrame() = 0;       //Buffer manager uses this function to ask the replacer for an available frame
		virtual void FreeFrame(Frame* frame) = 0;   //Buffer manager uses this to let the Replacer know the frame is now available
		virtual bool isFull()  {return freeFrames == 0;};
		//Some replacers may use custom frames, hence the replacer and  not the buffer manager is responsible for creating them
		virtual void constructFrames(){
			for(int f = 0; f < numFrames; ++numFrames) frames[f] = new Frame();

		}
		virtual void ResurrectFrame(Frame* frame){;} //This lets the replacer know that a page with no pins has been revived and will be repinned
		inline virtual int getUnpinnedPages(){return freeFrames;}

		/*The buffer manager may let the replacer know it will not need the frame again. Subclasses are free to optimize on this info*/
		virtual void ClearFrame(Frame* frame){;}

};

class Clock : public Replacer
{
	private :
		
		int current;
		int numFrames;
		Frame **frames;
		unordered_map<PageID, Frame*> *map;

	public :
		
		Clock( int bufSize, Frame **frames, unordered_map<PageID, Frame*> *map );
		~Clock();
		Frame* NextFrame();
};

/**We represent the collection of the buffer's frames as:
 * A queue (Implemented with a doubly linked list) of available (pin count = 0) frames where the least recently used frame is on the top of the queue.
 * In the beginning of the buffer manager's life, all available frames are put on this queue in arbitrary order.
 * When a frame is unpinned, it is moved to the back of the queue so we can represent an LRU
 * collection of frames
 *
 *
 * When an available frame is pinned, it is removed from the linked queue of available frames
 * When a used frame is unpinned, if its pin count is now 0, it is moved to the back of the queue of available frames
 *
 *In the beginning, we all the frames/pages we are allowed to use as an array so that we can take advantage of cache speed ups
 *when memory is contiguous. The other option would be to allocate new frames up till a maximum as you need them, that would have an
 *when advantage of not taking up unnecessary space but would not have the cache advantage we just described
 *
 */
class LRU_Queue : public Replacer{

private:
	LinkedFrame* front, * back; //Top and bottom of the linked Queue of available frames

public:
	LRU_Queue(int ttlFrames, Frame **frames,  unordered_map<PageID, Frame*>* map );
	Frame* NextFrame();
	void   FreeFrame(Frame* frame);
	void constructFrames();
	void ResurrectFrame(Frame* frame);
	void ClearFrame(Frame* frame);

private:
	inline void RemoveFront();
	inline void RemoveBack();
	inline void RemoveMiddle(LinkedFrame* frame);
	inline void PushFront(LinkedFrame* frame);
	inline void PushBack(LinkedFrame* frame);

};
