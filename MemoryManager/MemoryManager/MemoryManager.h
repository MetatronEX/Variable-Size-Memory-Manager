/*
The MIT License (MIT)
Copyright (c) 2016 Xavier RX Tan

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in the 
Software without restriction, including without limitation the rights to use, copy, 
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/****************************************************
Author : Xavier Tan
Email  : xavier.rx.tan@gmail.com / tan.ruixiang@digipen.edu

VariableMemoryManager.h

*****************************************************/
#ifndef MEMORY_MANAGER_H_
#define MEMORY_MANAGER_H_

#include <memory>

/**
	\enum MEM_SIZE
	\brief 
		Memory unit size
*/
enum MEM_SIZE
{
	KILO_BYTE = 1024,
	MEGA_BYTE = 1048576,
	GIGA_BYTE = 1073741824
};

/**
	\brief 
		A custom lightweight memory manager to help the user in maximizing 
		memory usage for asset allocation in run time. Variable here refers 
		to non-fixed allocation since game assets are not fixed size.
*/
class VariableMemoryManager
{
private:
	/**
		\struct MetaData MemoryManager.h
		\brief	
			A meta data header to specify attribute of an associated memory region
	*/
	struct MetaData	
	{
		unsigned  Size;				 /**< Size of associated memory */
		MetaData* Next;				 /**< Pointer to next Meta Data Header*/
		MetaData* Prev;				 /**< Pointer to previous Meta Data Header*/
		unsigned short pageIndex;	 /**< Numeric index of associated parent page*/
		bool available;				 /**< Availability boolean*/
		char padding;				 /**< Padding byte for alignment*/
	};

	/**
		\struct Page MemoryManager.h
		\brief  
			A meta data header to specify the attribute of an associated memory page
	*/
	struct Page
	{
		Page*    Next;				/**< Pointer to next memory page*/
		unsigned memLeft;			/**< Amount of memory left in this page*/
		char*	 chunk;				/**< The memory chunk*/
		unsigned short index;		/**< Numeric index of this memory page*/
	};

public:
	/**
		\brief Constructor.
		\param _pageSizeInBytes			Size of a chunk of memory for page management
		\param _fragmentThreshold		A specified value to denote level of tolerance(in bytes) of the amount of fragmentation. Recommends the size of the smallest asset.
		\param _allocateUponNoFreeSpace A switch that tells the manager to allocate a new page of memory of size pageSize
	*/
	VariableMemoryManager( const unsigned& _pageSizeInBytes, const unsigned& _fragmentThreshold, 
						   bool _allocateUponNoFreeSpace = true );
	/**
		\brief Destructor
	*/
	~VariableMemoryManager();

	/**
		\brief Return a pointer location in an arbitrary memory chunk that satisfy the user's request
		\param size	Size of the requested memory
		\return A void pointer
	*/
	void*  Allocate		( const unsigned& size );

	/**
		\brief "Deallocate" a specified memory address by changing the availability flag to true and coalesce with neighboring free space
		\param object A pointer to the memory address that request deletion
	*/
	void   Free			( void* object );

	// TODO: void   ReturnUnusedMemory();

	/**
		\brief A debug function to dump a text file for examination of memory allocated.
		\param fileName Name of the output file
	*/
	void MemoryDump (const char* fileName);

private:

	// Note: C++11 ctor disabling is not supported in MSVC11
	VariableMemoryManager() /*= delete*/;
	VariableMemoryManager(const VariableMemoryManager& ) /*= delete*/;
	VariableMemoryManager(VariableMemoryManager&& ) /*= delete*/;
	VariableMemoryManager& operator= ( const VariableMemoryManager& ) /*= delete*/;

	/**
		\brief Allocates new set of memory of size pageSize for allocation needs
	*/
	void RequestPage();
	
	/**
		\brief Delete all allocated memory pages
	*/
	void FreeAllPages();

	unsigned pageSize;			   /**< size of a memory chunk that constitute a page*/
	unsigned fragmentThreshold;	   /**< size of fragmentation tolerance*/
	unsigned pageCount;			   /**< number of allocated pages*/

	Page*	 pageList;			   /**< a link list of memory pages*/
	Page*	 lastPage;			   /**< a pointer that points to the last allocated memory*/

	bool	 bAllocate;			   /**< a switch to indicate if user wants the manager to request for new page of memory when there is not enough to satisfy request*/
};

#endif