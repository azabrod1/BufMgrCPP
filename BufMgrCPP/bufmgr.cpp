
#include "bufmgr.h"
#include "frame.h"

//--------------------------------------------------------------------
// Constructor for BufMgr
//
// Input   : bufSize  - number of pages in the this buffer manager
// Output  : None
// PostCond: All frames are empty.
//--------------------------------------------------------------------

BufMgr::BufMgr( int bufSize )
{
    totalCall = totalHit = numDirtyPageWrites = 5;

    numFrames  = bufSize;
	frames     = new Frame*[bufSize];
	//policyMgr  = new LRU_Queue(bufSize, frames, &map);
	//policyMgr->constructFrames();
    
   // LinkedFrame* linkedFrames = new Frame[numFrames];
    for(int f = 0; f < numFrames; ++f)
        frames[f] = new Frame;
    
    
    
    
    cout << endl;
}


//--------------------------------------------------------------------
// Destructor for BufMgr
//
// Input   : None
// Output  : None
//--------------------------------------------------------------------

BufMgr::~BufMgr()
{   

	for(int f = 0; f < numFrames; ++f){
		if(frames[f]->IsDirty())
			frames[f]->Write();
		delete frames[f];
	}

	delete[] frames;
	delete policyMgr;

}

//--------------------------------------------------------------------
// BufMgr::PinPage
//
// Input    : pid     - page id of a particular page 
//            isEmpty - (optional, default to false) if true indicate
//                      that the page to be pinned is an empty page.
// Output   : page - a pointer to a page in the buffer pool. (NULL
//            if fail)
// Purpose  : Pin the page with page id = pid to the buffer.  
//            Read the page from disk unless isEmpty is true or unless
//            the page is already in the buffer.
// Condition: Either the page is already in the buffer, or there is at
//            least one frame available in the buffer pool for the 
//            page.
// PostCond : The page with page id = pid resides in the buffer and 
//            is pinned. The number of pin on the page increase by
//            one.
// Return   : OK if operation is successful.  FAIL otherwise.
//--------------------------------------------------------------------


Status BufMgr::PinPage(PageID pid, Page*& page, bool isEmpty)
{
	++totalCall;

	if(map.count(pid)){ //We have a record of this page already in memory
		++totalHit;
		Frame* frame = map[pid];
		if(frame->NotPinned())
			policyMgr->ResurrectFrame(frame); //Lets the replacer know the frame is back into use so it wont be recycled
		frame->Pin();
		page = frame->GetPage();
		return OK;
	}

	/*We need to ask the replacer for a free page to work with*/
    
    //cout <<policyMgr;

	Frame* newFrame = policyMgr->NextFrame();

	if(newFrame == NULL) return FAIL;

	if(newFrame->hasValidPID()){ //if the page previously had a valid pid, we need remove it from the record map

		map.erase(newFrame->GetPageID()); //Erase the old pageID from the map of pages
		if(newFrame->IsDirty()){           //Flush the contents of the page to disk if it is dirty
			if(newFrame->Write() == FAIL) std::cerr << "Warning: A page write failed\n";
			++numDirtyPageWrites;
		}
	}

	newFrame->SetPageID(pid);

	if(!isEmpty){

		Status status = newFrame->Read();

		if(status != OK){ //If read fails, give page back to buffer

			newFrame->ResetFrame(); policyMgr->FreeFrame(newFrame);
			return status;
		}

	}
	page  = newFrame->GetPage();
	map[pid] = newFrame;

	return OK;
} 

//--------------------------------------------------------------------
// BufMgr::UnpinPage
//
// Input    : pid     - page id of a particular page 
//            dirty   - indicate whether the page with page id = pid
//                      is dirty or not. (Optional, default to false)
// Output   : None
// Purpose  : Unpin the page with page id = pid in the buffer. Mark 
//            the page dirty if dirty is true.  
// Condition: The page is already in the buffer and is pinned.
// PostCond : The page is unpinned and the number of pin on the
//            page decrease by one. 
// Return   : OK if operation is successful.  FAIL otherwise.
//--------------------------------------------------------------------


Status BufMgr::UnpinPage(PageID pid, bool dirty)
{
	Frame * frame;
	try{
		frame = map.at(pid);
	}
	catch (const std::out_of_range& except) { //The pid does not exist in the map
		return FAIL;
	}

	if(frame->NotPinned()) return FAIL;      //Can't unpin if already pinCount is 0

	frame->Unpin();

	if(dirty) frame->DirtyIt();

	if(frame->NotPinned()) policyMgr->FreeFrame(frame); //Note that we do not flush it to disk until we can't fit it in main memory

	return OK;
}


//--------------------------------------------------------------------
// BufMgr::NewPage
//
// Input    : howMany - (optional, default to 1) how many pages to 
//                      allocate.
// Output   : firstPid  - the page id of the first page (as output by
//                   DB::AllocatePage) allocated.
//            firstPage - a pointer to the page in memory.
// Purpose  : Allocate howMany number of pages, and pin the first page
//            into the buffer. 
// Condition: howMany > 0 and there is at least one free buffer space
//            to hold a page.
// PostCond : The page with page id = pid is pinned into the buffer.
// Return   : OK if operation is successful.  FAIL otherwise.
// Note     : You can call DB::AllocatePage() to allocate a page.  
//            You should call DB:DeallocatePage() to deallocate the
//            pages you allocate if you failed to pin the page in the
//            buffer.
//--------------------------------------------------------------------


Status BufMgr::NewPage (PageID& firstPid, Page*& firstPage, int howMany)
{
	if(policyMgr->isFull()) return FAIL; //We have no space left to pin the first page so we must ABORT

	Status status = MINIBASE_DB->AllocatePage(firstPid, howMany);
	if(status != OK) return FAIL;

	this->PinPage(firstPid, firstPage, true); //Guaranteed to work because we checked available space before

	return OK;
}

//--------------------------------------------------------------------
// BufMgr::FreePage
//
// Input    : pid     - page id of a particular page 
// Output   : None
// Purpose  : Free the memory allocated for the page with 
//            page id = pid  
// Condition: Either the page is already in the buffer and is pinned
//            no more than once, or the page is not in the buffer.
// PostCond : The page is unpinned, and the frame where it resides in
//            the buffer pool is freed.  Also the page is deallocated
//            from the database. 
// Return   : OK if operation is successful.  FAIL otherwise.
// Note     : You can call MINIBASE_DB->DeallocatePage(pid) to
//            deallocate a page.
//--------------------------------------------------------------------


Status BufMgr::FreePage(PageID pid)
{
	if(!map.count(pid)) //if the page is not in main memory, we try to deallocate it
		return MINIBASE_DB->DeallocatePage(pid, 1);

	Frame* toFree = map[pid];

	int pins = toFree->NumPins();

	if(pins > 1) return FAIL;

	/*Next we erase all records of the page from main memory*/

	toFree->ResetFrame(); map.erase(pid);

	if(pins == 1){
		policyMgr->FreeFrame(toFree);     //Tell the policy manager that the frame should now be available
	}
	else //Pin Count 0 :  let the Policy manager know the frame will no longer be needed so it can perform optimizations
		policyMgr->ClearFrame(toFree);


	return MINIBASE_DB->DeallocatePage(pid, 1);
}


//--------------------------------------------------------------------
// BufMgr::FlushPage
//
// Input    : pid  - page id of a particular page 
// Output   : None
// Purpose  : Flush the page with the given pid to disk.
// Condition: The page with page id = pid must be in the buffer,
//            and is not pinned. pid cannot be INVALID_PAGE.
// PostCond : The page with page id = pid is written to disk if it's dirty. 
//            The frame where the page resides is empty.
// Return   : OK if operation is successful.  FAIL otherwise.
//--------------------------------------------------------------------


Status BufMgr::FlushPage(PageID pid)
{
	if(!map.count(pid)) return FAIL; //The page does not exist in the buffer

	Frame * flushMe = map[pid];
	if(flushMe->Pinned()) return FAIL; //The page cannot be pinned

	if(flushMe->IsDirty()){
		Status status = flushMe->Write();

		if(status != OK) return status;
	}

	/*We remove all records of this page from main memory */
	map.erase(pid); flushMe->ResetFrame();
	policyMgr->ClearFrame(flushMe);

    return OK;
} 

//--------------------------------------------------------------------
// BufMgr::FlushAllPages
//
// Input    : None
// Output   : None
// Purpose  : Flush all pages in this buffer pool to disk.
// Condition: All pages in the buffer pool must not be pinned.
// PostCond : All dirty pages in the buffer pool are written to 
//            disk (even if some pages are pinned). All frames are empty.
// Return   : OK if operation is successful.  FAIL otherwise.
//--------------------------------------------------------------------

Status BufMgr::FlushAllPages()
{
	Frame* frame; Status status;
	if(policyMgr->getUnpinnedPages() != numFrames) return FAIL; //This means there are some pages that are pinned

	for(int f = 0; f < numFrames; ++f){
		frame = frames[f];
		if(frame->hasValidPID() && frame->IsDirty()){
			if(frame->Write() != OK) return FAIL;
		} frame->ResetFrame();
	} map.clear();
	return OK;
}


//--------------------------------------------------------------------
// BufMgr::GetNumOfUnpinnedFrames
//
// Input    : None
// Output   : None
// Purpose  : Find out how many unpinned locations are in the buffer
//            pool.
// Condition: None
// PostCond : None
// Return   : The number of unpinned buffers in the buffer pool.
//--------------------------------------------------------------------

unsigned int BufMgr::GetNumOfUnpinnedFrames()
{
	return policyMgr->getUnpinnedPages();
}

void  BufMgr::PrintStat() {
	cout<<"**Buffer Manager Statistics**"<<endl;
	cout<<"Number of Dirty Pages Written to Disk: "<<numDirtyPageWrites<<endl;
	cout<<"Number of Pin Page Requests: "<<totalCall<<endl;
	cout<<"Number of Pin Page Request Misses "<<totalCall-totalHit<<endl;
}

//--------------------------------------------------------------------
// BufMgr::FindFrame
//
// Input    : pid - a page id 
// Output   : None
// Purpose  : Look for the page in the buffer pool, return the frame
//            number if found.
// PreCond  : None
// PostCond : None
// Return   : the frame number if found. INVALID_FRAME otherwise.
//--------------------------------------------------------------------

int BufMgr::FindFrame( PageID pid )
{
	for(int f = 0; f < numFrames; ++f)
		if(frames[f]->GetPageID() == pid) return f;
	return -1;
}
