#include "frame.h"

/*initialize frame variables */
Frame::Frame(){
	this->pid        = INVALID_PAGE;
	//this->data       = new Page();
	this->dirty      = false;
	this->pinCount   = 0;

}

Frame::~Frame(){

	//delete data;
}

/*Increment the page's pin count */
void Frame::Pin(){
	++pinCount;
}

void Frame::ResetFrame(){
	this->pid        = INVALID_PAGE;
	this->dirty      = false;
	this->pinCount   = 0;
}

void Frame::Unpin()
{
	--pinCount;
}

void Frame::DirtyIt(){
	dirty = true;
}
void Frame::SetPageID(PageID pid){
	this->pid = pid;
}

bool Frame::IsDirty(){return dirty;}

bool Frame::IsValid(){return pid != INVALID_PAGE;}


/*Write the contents of the page the frame contains to disk */
Status Frame::Write(){
	if(pid == INVALID_PAGE) return FAIL;

	return MINIBASE_DB->WritePage(pid,data);

}
/*Read the page from the disk associated with its pid and fill this frame's page buffer with the data */
Status Frame::Read(){

	Status toReturn = MINIBASE_DB->ReadPage(pid, data);

	return toReturn;

}

Status Frame::Free(){
	return MINIBASE_DB->DeallocatePage(pid, 1);
}


Bool Frame::NotPinned(){
	return pinCount == 0;
}

PageID Frame::GetPageID(){
	return pid;
}
Page* Frame::GetPage(){
	return data;
}

int Frame::NumPins(){
	return pinCount;
}
