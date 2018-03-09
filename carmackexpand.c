#include <stdio.h>
#include <stdlib.h>

#include "carmackexpand.h"

u_int8_t* wCarmackExpand(u_int8_t* data, u_int16_t size,
						 u_int16_t deSize)
{
	/* 	cpyOff: offset of data to copy
		cpyCnt: amount of data to copy */
	unsigned short cpyOff;
	unsigned char cpyCnt;
	
	unsigned int i = 0;
	while(i < size)
	{
		// first byte: amount of characters we must copy
		
		cpyCnt = data[i];
		++i;
		
		// second byte: CARMACKTag
		
		switch(data[i])
		{
			default:
				break;
			case NEARTAG:
				++i;
				cpyOff = data[i];
				int j; for(j = 0; j < cpyCnt; ++j) {
					// -- neartag shit -- //
				}
				break;
			case FARTAG:
			// get the low byte
				++i;
				cpyOff = (unsigned short)data[i];
			// get the high byte
				++i;
				cpyOff += (data[i] * 16);
				break;
			case EXCEPTAG:
				break;
		}
	}
	
	return NULL;
}
