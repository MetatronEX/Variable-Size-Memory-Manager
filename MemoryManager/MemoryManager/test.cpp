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

/**
*	\mainpage VariableMemoryManager
*
*	\author Xavier Tan	
*
*	\section about_sec About
*
*	\subsection subsec_intro Introduction
*	
*	The VariableMemoryManager(VMM) is an attempt to write a light weight memory manager 
*	that can facilitate the allocation and deallocation of memory in a time sensitive 
*	environment with minimum fragmentation.
*	
*	\subsection subsec_motivation Motivation
*	
*	During my 4 years in DigiPen Singapore, I have written and played around with a few 
*	asset format types (.obj, .fbx etc..) for assignments and projects that are read or 
*	written to during the application run-time. Much of these operations revolves around 
*	heap memory management and we know that the default new and delete operators provided 
*	by the C++ language are too slow to meet the ever demanding pace of a real-time 
*	interactive application. That is because the new operator at the kernel thread level 
*	will attempt to run a recursive call to find a fitting chunk of memory that will match 
*	the request and also the fact that there is the need for thread level safety thus the 
*	extra running cycle for lock-aquisition.
*	
*	Thus this piece of code is an attempt to revisit a brief topic that was covered in 
*	various classes and to arrive at a small compromised solution that attempts to alleviate 
*	as much overhead as possible.
*	
*	\subsection subsec_design Design
*	
*	The VariableMemoryManager (VMM) is consist of 2 parts. The first part is the main management 
*	arbiter that keep track of the allocation/deallocation and page request in run-time. 
*	The second portion are the pages themselves, which are large chunk of memory requested 
*	contiguously at anytime during runtime as per requirement.
*	
*	The VMM do not make any assumption on it's end on the application's memory usage. Much of 
*	the work are dependent on the users themselves as they are the ones who knows their 
*	application better. The VMM is meant to make life easier and allocation/ deallocation 
*	routines faster, not dictate how things should be done. Therefore, in order to achieve 
*	this level of flexibility, a certain level of "ease of use" have been compromised. However, 
*	the payoff is that some form of performance uncertainty is amortized in return for the 
*	users' input on their application's design.
*	
*	Some of the work required for VMM to work effectively is to tell the VMM upon construction 
*	how long is a length of memory page that they want, the level of threshold of fragmentation 
*	they are willing to tolerate in terms of bytes and whether to allocate new page memory 
*	requests upon insufficient space adequacy.
*	
*	We can see that with these variable switches in place, the users can customize how they want 
*	this memory manager to behave. With the page size length and the allocation Boolean switch, 
*	the user have the advantage to decide that he/she could allocate a sizable amount at the start 
*	of the application and use this large chunk to do memory placement of different asset types 
*	(meshes, textures, vertices...). Or they can conservatively choose to construct several different 
*	specialized VMMs that each takes care of a specified type of memory requirement by indicating 
*	smaller fixed page size and choosing to request for new set of memory of pre-specified page 
*	length upon inadequacy. This help to reduce the number of calls to new and delete in runtime 
*	and keep it at a minimum.
*	
*	The reason for fragment threshold value is to facilitate the minimization of memory internal 
*	fragmentation. It is commonly known that fragmentation will always be the central problem of memory 
*	management and there are no effective way to completely remove it from any design considerations. 
*	Thus, the best possible solution is to minimized it. We can attempt to pack as much data as possible 
*	in a given region of space, but we still have to consider the alignment problems and performance 
*	consequences for doing so. Thus, the proposed solution in VMM is to have the users run a prototype 
*	of their application, and base on usage pattern in run-time to benchmark and find the best possible 
*	fragmentation tolerance value for maximum efficiency. It is also recommended that the users find the 
*	memory size of their smallest asset and use that as the threshold value for VMM.
*	
*	The value is factored in during allocation to make predictions for potential spatial head room. In
*	the event when the head room is smaller than the threshold plus the meta data header size(16 bytes), 
*	it will be given as extra space to the current requested memory and will be reclaimed upon 
*	deallocation of the said memory region and coalesced together with neighboring free memory space.
*	
*	In order to keep the arbiter memory footprint as small and fast as possible, the meta data header 
*	are aligned contiguously with the requested memory region. This meta data header contain the 
*	following information:
*	- size of the memory of this sub-portion
*	- a pointer to the next meta data header
*	- a pointer to the previous meta data header
*	- an unsigned short index to indicate which page this memory resides in
*	- a Boolean value that indicates if this region is available for writing
*	- a 1 byte padding for easier memory pointer jump.
*	
*	The size variable is the total value of memory given to the user upon memory request. That includes 
*	any form of extra fragmentation head rooms for easier reclamation. The next and previous pointers help to 
*	facilitate fast memory coalescing. The availability Boolean help us skip regions that are not relevant 
*	for the allocation process. The page index help us to locate the right page index to update another 
*	meta header for the amount of memory left in a given chunk. However, this is currently an oversight 
*	in design as the document will process to explain later.
*	
*	By keeping the meta data header within the page, it helps the VMM achieve a few things.
*	
*	1: it keeps the VMM's arbiter as light weight as possible. If the VMM's arbiter were to keep a separate 
*	data structure to keep track of the allocation and free region, the size of the data structure and the 
*	arbiter will grow unnecessarily as the application runs its course. Even STL data structures will bloat 
*	the arbiter's size as there are certain features that the arbiter may or may not use. Thus, rather than 
*	using focusing on a probable usage, the VMM decide to do away with said data structure all together.
*	
*	2:  it further reduces the call to new and delete operators in run time. Having a separate data structure 
*	to keep track of the state of things meant that there will be information that which the application will 
*	not know ahead of time and be allocated to the stack. Instead, this lack of certainty meant that we will 
*	have to update any allocation behavior to the data structure in real time, thus a direct implication of usage 
*	in the heap space. This meant that we will invariably have to call new and delete and generate a potential for
*	bottleneck.
*	
*	Hence, having to go through a few design revisions, the current paradigm is deem to be effective in terms of 
*	spatial and performance payoff. After all the whole idea of a memory manager is to facilitate quick allocation 
*	and deallocation, garbage collection and maximize memory usage for the assets' requirement. However, VMM's 
*	design is still open to suggestions for any form of improvement.
*	
*	\subsection subsec_oversight Oversights
*	
*	\subsubsection subsubsec_unused Returning Unused Memory
*	One of the biggest oversight is the potential of returning unused memory space back to the OS in runtime
*	when all assets in a page (perhaps for coherency purposes) are deallocated simultaneously. This is clearly 
*	an oversight because right now with the given design, the VMM have no way of returning unused pages without 
*	incurring a large overhead. The unsigned short residing within the memory meta header will now need to be 
*	updated across the board when said event occurs. In the worst case this will result in a O(N^2) update.
*	
*	\subsubsection subsub_MT Multithreading allocation. 
*	The VMM is currently written under the consideration that it is used in a single 
*	threaded environment. In the case of a multithreaded environment the payoff of using a custom memory manager 
*	will diminish as there is the consideration for lock acquisition during allocation and deallocation routines. 
*	As shown in chapter 6 of Efficient C++, one of the reasons why the default new and delete is slow is because 
*	of the fact that both operations have to consider for multithreaded environment. One way to alleviate this 
*	issue is to attempt to write a lock free paradigm for the memory manager's accessing behavior. However, at 
*	the time of writing I am still in the midst of understanding the methodologies of concurrency. Thus, 
*	this portion is subjected to further revision when a better level of understanding is achieved.
*	
*	\subsubsection subsubsec_cache Cache Coherency 
*	One more area of concern may be cache cohenrency. If data set is small then the cache may hit the meta data
*	when the cache takes one line worth of data.
*/

#include "MemoryManager.h"

#include <chrono>
#include <iostream>

static VariableMemoryManager TestManager(5 * MEM_SIZE::KILO_BYTE, 50);

/*
*	\brief 
*	a test structure that is meant to mimic a vertex in a mesh file
*/
class test_struct
{
public:
	test_struct()
	{
		start_boundary = 0x54525453; // STRT 
		x = 0x38373d78;				 // x = 78
		y = 0x39373d79;				 // y = 79
		z = 0x41373d7a;				 // z = 7A
		u = 0x35373d75;				 // u = 75
		v = 0x36373d76;				 // v = 76
		end_boundary = 0x2e444e45;   // END.
	}

	~test_struct()
	{
	
	}

	/// \brief 
	///	overloaded operator new for placement syntax
	void* operator new(std::size_t size)
	{
		return TestManager.Allocate(size);
	}

	/// \brief 
	///	overloaded operator new[] for placement syntax
	void* operator new[] (std::size_t size)
	{
		return TestManager.Allocate(size);
	}

	/// \brief 
	///	overloaded operator delete for placement syntax
	void operator delete(void* addr)
	{
		TestManager.Free(addr);
	}

	/// \brief 
	///	overloaded operator delete[] for placement syntax
	void operator delete[](void* addr)
	{
		TestManager.Free(addr);
	}

	/**
		\var start_boundary an unsigned int to denote start of a new data structure
		\var x emulates vertex position 
		\var y emulates vertex position 
		\var z emulates vertex position	
		\var u emulates vertex texture 
		\var v emulates vertex texture 	
		\var end_boundary  
	*/
	unsigned start_boundary; 
	unsigned x,y,z;			 
	unsigned u,v;
	unsigned end_boundary;   
};

void SequenceCorrectnessTest()
{
	// a simple allocation test
	test_struct* A = new test_struct;

	TestManager.MemoryDump("../1st Write.txt");

	// An array allocation. Apparently the compiler will
	// take care of the extra 4, 8 or any other amount of specified alignment
	// bytes that it will request under the hood
	test_struct* B = new test_struct[10];

	TestManager.MemoryDump("../2nd Write.txt");

	// A deallocation memory test.
	delete A;

	TestManager.MemoryDump("../1st Delete.txt");

	// A test for correctness of memory allocation
	// pattern
	test_struct* C = new test_struct[5];

	TestManager.MemoryDump("../3rd Write.txt");

	// A mass memory deallocation test.
	// alignment byte factored in on compiler's end
	// Also a test on coalescing function correctness
	delete [] B;

	TestManager.MemoryDump("../2nd Delete.txt");

	// A test on overwriting previously freed memory
	A = new test_struct[10];

	TestManager.MemoryDump("../4th Write.txt");

	// same test at a larger scope
	// and reallcate into a section of coalesced
	// memory. At the same time test for the fragment 
	// threshold behavior
	B = new test_struct[150];

	test_struct* D = new test_struct[10];

	TestManager.MemoryDump("../5th Write.txt");

	// A test for new page request
	test_struct* E = new test_struct[3];

	TestManager.MemoryDump("../6th Write.txt");

	// test to find out if the correct memory region will
	// be choosen.
	test_struct* F = new test_struct[2];

	TestManager.MemoryDump("../7th Write.txt");
}

int main ()
{
	SequenceCorrectnessTest();

	system("PAUSE");

	return 0;
}