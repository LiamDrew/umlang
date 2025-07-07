Here is the original project proposal from February 2025. The project focus since evolved from complete memory system to memory allocator to fit our use case, but I've left the original proposal unmodified below.  

Project Idea:   
A 32-bit virtual memory system managing emulated “physical” memory on a 64-bit system.  

Rationale:  
Memory is one of the most precious hardware resources of a computer that an operating system has to manage. Between memory and the CPU, memory is by far the more difficult hardware resource to manage. When an operating system has to run different processes, it will context switch between running different processes on the CPU. For instance, if one process requires an expensive I/O operation, the OS will context switch from the process that is waiting for I/O to another one that is ready to run directly on the CPU. 

However, doing this is not possible with memory. The equivalent of doing a memory context switch would be storing the entire contents of physical memory to disk every time a CPU context switch occurred. That would be prohibitively slow considering that the only reason a context switch occurred in the first place is because of a disk operation. Thus, physical memory must be shared between processes.

Thus, the OS gives each program the illusion that it has complete access to contiguous virtual memory, even when physical memory may be heavily in use by other processes on the system. In order to do this, the OS uses page tables (one per process) to convert virtual memory addresses to physical memory addresses when the process needs to access data. The OS also has a hardware address translation cache (the address Translation Lookaside Buffer, or TLB) that facilitates this process. The OS also will swap memory pages to the disk when under heavy load. Modern virtual memory systems can get extremely complicated. People who understand how to build this stuff are superstar engineers, and I think we should give ourselves a taste of that, even in a limited way.  

Plan:  
Implement a 32-bit virtual memory system on a 64-bit machine. Our entire team has Arm64 Macbooks that all have the same physical page size (16384, or 214) so it would make life super easy if we could compile and run C programs natively, all on the same architecture.

MacOS is very much like Linux for what we’re trying to do. My plan is to use the mmap() function to allocate 4GB of memory. Doing this will allocate a “huge page” that the OS gives to the application to manage as it sees fit. Importantly, this 4GB of virtual memory will be contiguous, which is super important for what we’re trying to do. We are going to treat this 4GB of contiguous virtual as our emulated physical memory, and basically write our own version of malloc() and free(). Our_malloc() will carve out a chunk of this contiguous mmapped virtual memory, which we are calling our emulated physical memory, and serve it to the application running on top of our VM system as virtual memory. When the application free()s the memory, our VM system will have to update its internal state to recognize that the memory has been freed, and is available for future use.

It’s up to us how we design this system. We could use pages, which is likely to work, I just fear it would be slow, since a real virtual memory system that uses pages has hardware support that we don’t have access to (this is, of course, not a hardware project). I would like to implement a custom virtual memory system that is 1. simple and 2. fast. It will certainly not be as useful for general-purpose computing as Linux, for instance. My preference would be to do this with segments instead of pages. This could get messy, but we can easily find ways around it. There will be a lot of memory that goes unutilized (this is called fragmentation, in this case external fragmentation between segments).

The great part about this project is that we can make it as complicated as it needs to be. We will start with managing memory for a single process, and keeping track of an efficient way to allocate and free memory as it is required. If we breeze through that, we can tackle more complex problems like
Having multiple processes share the same virtual memory
Running two memory-intensive processes that require more than 4GB of memory, and implementing our page-swapping algorithm to and from disk.
I would be shocked if we have time to tackle either of these challenges within our two-day timespan.  

Tech Stack:  
C  

Applications/Use cases:  
The Universal Machine (from CS40) is an outstanding use case for testing this 32-bit virtual memory system. That project has students use an array of pointers to memory segments to handle this problem, which handles the 64-bit to 32-bit memory conversion problem. However, it makes every single UM memory access slow, since every UM memory access requires 2 system memory accesses, which is particularly slow because the double-pointer situation means terrible spatial locality and a high likelihood of cache misses. If we implement a simple working version of this, we should be able to significantly improve the performance of the UM.

Making the UM faster is my main intended application of this project. This is not likely to win us first place, I see this project as more of an interesting engineering challenge. We are all kernel engineers for the weekend.

I could also see this project being very useful for testing memory management on an embedded system. Some emulated embedded development is probably done in a docker container emulating 32-bit Linux 386, but the benefit of implementing this virtual memory system on a 64-bit system is that anyone doing embedded development could use external memory (memory available on the 64-bit system that’s not contained within the bounds of our emulated physical memory) for additional testing/debugging. I could also see this project being an excellent starting point for a bunch of other awesome systems/embedded projects. (Full disclosure: I know very little about embedded so take this with a grain of salt).  

Readings on Virtual Memory:
I recommend the following 4 readings if anything above doesn’t make sense:  
Address spaces:  https://pages.cs.wisc.edu/~remzi/OSTEP/vm-intro.pdf  
Address translation: https://pages.cs.wisc.edu/~remzi/OSTEP/vm-mechanism.pdf   
Segmentation: https://pages.cs.wisc.edu/~remzi/OSTEP/vm-segmentation.pdf  
Free space management: https://pages.cs.wisc.edu/~remzi/OSTEP/vm-freespace.pdf  

The below readings are about paging, which is interesting but not super applicable to the project I have proposed. Unless you’re super bored I’d skip over these.
Paging: https://pages.cs.wisc.edu/~remzi/OSTEP/vm-paging.pdf   
Fast address translation: https://pages.cs.wisc.edu/~remzi/OSTEP/vm-tlbs.pdf
