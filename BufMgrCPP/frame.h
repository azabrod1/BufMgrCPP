#ifndef FRAME_H
#define FRAME_H

#include "page.h"
#include "db.h"


class Frame 
{
	private :
	
		PageID pid;
		Page   *data;
		int    pinCount;
		bool    dirty;


	public :
		
		Frame();
		virtual ~Frame();
		void Pin();
		void Unpin();
		void DirtyIt();
		void SetPageID(PageID pid);
		Bool IsDirty();
		Bool IsValid();
		Status Write();
		Status Read();
		Status Free();
		Bool NotPinned();
		Bool Pinned(){return pinCount;}
		PageID GetPageID();
		Page *GetPage();
		bool hasValidPID(){return pid != INVALID_PAGE;}

		Bool IsReferenced();
		Bool IsVictim();
		int NumPins();
		void ResetFrame();

};

#endif
