#ifndef DROPZONE_INCLUDED
#define DROPZONE_INCLUDED

//
//	define a drop-zone, which can be used to over-ride
//	drop-behaviour for any card stacks which fall under it
//

class CardStack;

class DropZone
{
	friend class CardWindow;

	DropZone(int Id, RECT *rect, pDropZoneProc proc) : 
	  id(Id), DropZoneCallback(proc) { CopyRect(&zone, rect); }

public:

	void SetZone(RECT *rect) { CopyRect(&zone, rect); }
	void GetZone(RECT *rect) { CopyRect(rect, &zone); }
	void SetCallback(pDropZoneProc callback) { DropZoneCallback = callback; }

	int  DropCards(CardStack &cardstack) 
	{
		if(DropZoneCallback)
			return DropZoneCallback(id, cardstack);
		else
			return -1;
	}

private:

	int  id;
	RECT zone;
	pDropZoneProc DropZoneCallback;
};

#endif
