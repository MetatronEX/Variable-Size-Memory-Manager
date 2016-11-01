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

VariableMemoryManager.cpp

*****************************************************/

#include "MemoryManager.h"
#include <fstream>
#include <iostream>

// Description:
// Constructor
VariableMemoryManager::VariableMemoryManager(const unsigned& _pageSizeInBytes, const unsigned& _fragmentThreshold,
											 bool _allocateUponNoFreeSpace)
		: pageSize(_pageSizeInBytes), fragmentThreshold(_fragmentThreshold), bAllocate ( _allocateUponNoFreeSpace )
{
	// immediately allocate memory upon construction
	pageList = new Page;
	pageList->Next = nullptr;
	pageList->index = 0;

	try 
	{
		pageList->chunk = new char[pageSize];
	}
	catch(std::bad_alloc)
	{
		// build simple log file
		std::ofstream logFile("Log_File.txt");
		logFile << "Bad Allocation detected upon VariableMemoryManager's construction. Application Terminated." << std::endl;
		logFile.close();
		// close the program
		abort();
	}

	MetaData* metaData = reinterpret_cast<MetaData*>(pageList->chunk);
	
	metaData->Prev = metaData->Next = nullptr;
	metaData->Size = pageList->memLeft = pageSize - sizeof(MetaData);
	metaData->available = true;

	// lastPage will always points to most recent requested memory
	lastPage = pageList;

	pageCount = 1;
}

// Description:
// Destructor
VariableMemoryManager::~VariableMemoryManager()
{
	FreeAllPages();
}

// Description:
// One of 2 main interactions for memory operations
// that is exposed to the end user.
// Search through the requested memory pages for free 
// worst-fit space region to satisfy the required memory need
void* VariableMemoryManager::Allocate( const unsigned& size )
{
	// static_assert will complain about the 2 vars not being constant
	// TODO : rework this portion to work with static_assert
	if ( size > pageSize ) 
	{
		std::cout << "Requested memory size exceed page size." << std::endl;
		return nullptr;
	}

	Page* p = pageList;

	// This is a worst case N^2 polynomial time search, but breaks 
	// when a candidate is found. If not, we will call RequestPage
	// to get a new set of empty memory
	while ( p )
	{
		// filter off this page of memory if the amount of memory it holds does not suit our purpose
		// to save saerch time.
		if ( size >= p->memLeft )
		{
			p = p->Next;
			continue;
		}

		MetaData* data = reinterpret_cast<MetaData*>(p->chunk);
		MetaData* worstFitCandidate = nullptr;

		// linear time loop to find area with optimal (worst-fit) candidancy
		// which hopefully will minimize fragmentation
		while ( data )
		{
			// filter off this memory set if it does not satify our request
			if ( !data->available || size > data->Size )
			{
				data = data->Next;
				continue;
			}

			if ( nullptr == worstFitCandidate || data->Size > worstFitCandidate->Size )
				worstFitCandidate = data;

			data = data->Next;
		}

		// if by this point worstFitCandidate do not have a pointed
		// address, it implies that we have enough memory in this page
		// but not a single chunk can accommodate our requirement
		// which could be a potential rare case
		if ( nullptr == worstFitCandidate )
		{
			p = p->Next;
			continue;
		}

		// TODO: merge this code into a function
		// check if there are still head room in this large chunk
		// that can be split to allow new allocation between this
		// and the next chunk pointed by worstFitCandidate. 
		// Determined by asset sizes.
		char* mem_addr = reinterpret_cast<char*>(worstFitCandidate);

		unsigned headroom = ( worstFitCandidate->Size - size );

		if ( headroom > fragmentThreshold + sizeof(MetaData) )
		{
			MetaData* newMetaData = reinterpret_cast<MetaData*>( mem_addr + sizeof(MetaData) + size );
			
			// cast the remainder headroom memory into a new memory set
			// available for future allocation
			newMetaData->Next = worstFitCandidate->Next;
			
			if ( worstFitCandidate->Next )
				worstFitCandidate->Next->Prev = newMetaData;

			newMetaData->Prev = worstFitCandidate;
			worstFitCandidate->Next = newMetaData;
			worstFitCandidate->Size = size;
			// new memory available will be headroom minus the meta data 
			// that describe the new free space
			newMetaData->Size = headroom - sizeof(MetaData);
			newMetaData->available = true;
			newMetaData->pageIndex = p->index;
		}

		// if there are no headroom we will just give the full chunk
		// that worstFitCandidate describes. The excess will be 
		// considered as fragmentation but this fragmentation is considered
		// as minimized because the user had prescribed a certain threshold of tolerance 
		// that they are willing to accept base on their assets' size
		worstFitCandidate->available = false;
		p->memLeft -= worstFitCandidate->Size;

		return reinterpret_cast<void*>(mem_addr + sizeof(MetaData));
	}

	// if for some reason user choose not to allocate new memory 
	// ( i.e user had already allocated a sizable proportion 
	// from the main memory avaliable), then we will terminate 
	// the application and consider it a bad allocation.
	if ( !bAllocate )
	{
		// Clear everything
		FreeAllPages();

		// build simple log file
		std::ofstream logFile("Log_File.txt");
		logFile << "Bad Allocation detected. Application Terminated." << std::endl;
		logFile.close();
		// close the program
		abort();
	}

	// We have exhusted our search, so we have no choice but to 
	// request for a new set of empty page
	RequestPage();

	// TODO: merge this with the usual allocation routine into a function since both 
	// of them does the same thing
	MetaData* currMetaData = reinterpret_cast<MetaData*>(lastPage->chunk);
	
	char* returnPtr = reinterpret_cast<char*>(currMetaData) + sizeof(MetaData);

	MetaData* newMetaData = reinterpret_cast<MetaData*>(returnPtr + size);
	
	currMetaData->Next = newMetaData;
	newMetaData->Prev = currMetaData;
	newMetaData->Next = nullptr;

	currMetaData->available = false;
	newMetaData->available = true;

	newMetaData->Size = currMetaData->Size - ( size + sizeof(MetaData) );
	currMetaData->Size = size;

	return reinterpret_cast<void*>(returnPtr);	
}

// Description:
// Take the address and free the data for future writes.
// Also coalesce with near by free memory.
void VariableMemoryManager::Free( void* object )
{
	// get to our metadata header
	MetaData* metaData = reinterpret_cast<MetaData*>( reinterpret_cast<char*>(object) - sizeof(MetaData) );
	metaData->available = true;

	// iterate to the parent page meta header
	// to update available memory size
	Page* p = pageList;

	for ( unsigned counter = 0; counter < metaData->pageIndex ; ++counter )
	{
		if ( !p->Next)
			break;

		p = p->Next;
	}
	
	p->memLeft += metaData->Size;

	// attempt to coalesce within immediate memory vacinity
	if ( metaData->Next->available )
	{
		metaData->Size += metaData->Next->Size + sizeof(MetaData);
		metaData->Next = metaData->Next->Next;
		p->memLeft += sizeof(MetaData);
	}

	if ( metaData->Prev )
	{
		if ( metaData->Prev->available )
		{
			metaData->Prev->Size += metaData->Size + sizeof(MetaData);
			metaData->Prev->Next = metaData->Next;
			p->memLeft += sizeof(MetaData);
		}
	}	
}

// Description :
// Allocate a pageSize long chunk of memory for present and future allocation.
void VariableMemoryManager::RequestPage()
{
	// create new Page node
	Page* p = new Page;

	// attempt to request for a large chunk of memory of pageSize
	try
	{
		p->Next = nullptr;
		p->chunk = new char[pageSize];
	}
	catch(std::bad_alloc) // bad allocation detected
	{
		FreeAllPages();

		// build simple log file
		std::ofstream logFile("Log_File.txt");
		logFile << "Bad Allocation detected. Application Terminated." << std::endl;
		logFile.close();
		// close the program
		abort();
	}

	MetaData* metaData = reinterpret_cast<MetaData*>(p->chunk);

	metaData->Next = metaData->Prev = nullptr;
	metaData->Size = p->memLeft = pageSize - sizeof(MetaData);
	metaData->available = true;
	
	lastPage->Next = p;
	lastPage = p;
	p->index = pageCount;
	++pageCount;
}

// Description :
// Garbage collection at the end of the program lifecycle when the manager's dtor is called
void VariableMemoryManager::FreeAllPages()
{
	// iterate throught and delete the memory chunks
	for ( Page* p = pageList; p != nullptr; p = pageList )
	{
		pageList = pageList->Next;
		
		if ( p->chunk )
			delete [] p->chunk;

		if ( p )
			delete p;
	}
}

// Description:
// A simple dump file to examine the memory for debugging purpose
void VariableMemoryManager::MemoryDump(const char* fileName)
{
	std::ofstream dumpFile(fileName);

	unsigned pageNumber = 0;

	Page* p = pageList;

	std::cout << "Writing file: " << fileName << std::endl;

	while ( p )
	{
		MetaData* meta = reinterpret_cast<MetaData*>(p->chunk);

		dumpFile << "Page : " << pageNumber << std::endl;

		while ( meta )
		{
			dumpFile << "Meta Data Address: " << std::hex << meta << std::dec << std::endl;
			dumpFile << "Next Node Address: " << std::hex << meta->Next << std::dec << std::endl;
			dumpFile << "Prev Node Address: " << std::hex << meta->Prev << std::dec << std::endl;
			dumpFile << "Memory Size : " << meta->Size << std::endl;
			dumpFile << "Avaliability : " << meta->available << std::endl;
			dumpFile << "Address\t|\tMemory Content" << std::endl;

			char* data = reinterpret_cast<char*>(meta) + sizeof(MetaData);
			
			for ( unsigned offset = 0; offset < meta->Size; ++offset )
			{
				dumpFile << std::hex << reinterpret_cast<unsigned>(data + offset) << std::hex << "\t|\t" << *(data + offset) << std::dec << std::endl;
			}

			dumpFile << std::endl;

			meta = meta->Next;
		}

		dumpFile << std::endl;

		++pageNumber;

		p = p->Next;
	}

	dumpFile.close();
}