/*
 * linkedframe.h

 *
 *  Created on: Feb 19, 2017
 *      Author: alex
 */

#ifndef INCLUDE_LINKEDFRAME_H_
#define INCLUDE_LINKEDFRAME_H_

#include "frame.h"



class LinkedFrame : public Frame{

public:
	LinkedFrame* prev, *next;
	LinkedFrame();
	~LinkedFrame();

};

#endif /* INCLUDE_LINKEDFRAME_H_ */
