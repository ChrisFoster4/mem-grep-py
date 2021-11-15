#include <pybind11/stl.h>
#include <pybind11/pybind11.h>
#include "../mem-grep/lib/misc/remote-memory.hpp"
#include "../mem-grep/lib/misc/structs.hpp"
#include "../mem-grep/lib/heap-traversing/stack-searcher.hpp"
#include "../mem-grep/lib/misc/map-parser.hpp"
#include "../mem-grep/lib/misc/prerun-checks.hpp"

// TODO
// ability to search (bss, heap and stack) # wip
// ability to traverse pointers
// ability to filter results
// ability to inspect and edit matched objects
// Expose Allocator enum


// DONE
// expose CanRun()

namespace py = pybind11;

enum Allocator : uint32_t {
  GLIBC
};




std::vector<RemoteHeapPointer> Search(const pid_t pid, const bool SearchBss=true, const bool SearchStack=true){

	auto parser = MapParser(pid);
	auto entries = parser.ParseMap();
	const MapsEntry stack = parser.getStoredStack();
	const MapsEntry text = parser.getStoredText();
	const MapsEntry heap = parser.getStoredHeap();
    const MapsEntry bss = parser.getStoredBss();
    
    
    // TODO pass this via the args
    // Is this a helpful option? as the objects arent returned over the C++/Python border so it doesnt matter if they are large
    const auto max_object_size = 2500000;
    
    auto stack_to_heap_pointers = std::vector<RemoteHeapPointer>();
    if (SearchStack){
        std::cout << stack.start << std::endl;
        auto stackSearch = StackSearcher(stack.start,text,pid,2550);
        stack_to_heap_pointers = stackSearch.findHeapPointers(stack.end, heap,10000);
        std::cout << "found: "<< stack_to_heap_pointers.size() << " stack pointers" << std::endl;
    }
    
    
    auto bss_to_heap_pointers = std::vector<RemoteHeapPointer>();
    if (SearchBss){
        auto bssSearcher = BssSearcher(bss, pid, 25000);
        bss_to_heap_pointers = bssSearcher.FindHeapPointers(heap);
        std::cout << "found " << bss_to_heap_pointers.size() << " bss pointers" << std::endl;
    }

    
    // Merge the two vectors
    stack_to_heap_pointers.reserve( stack_to_heap_pointers.size() + bss_to_heap_pointers.size() );
    stack_to_heap_pointers.insert(stack_to_heap_pointers.end(), bss_to_heap_pointers.begin(), bss_to_heap_pointers.end());

     //auto parser = MapParser(pid);
	//auto entries = parser.ParseMap();
    
    std::cout << "FoUND 1: " << stack_to_heap_pointers.size() << "\n";
    
    auto heap_traverser = HeapTraverser(pid, parser.getStoredHeap(), 1000000);
    
    return heap_traverser.TraversePointers(stack_to_heap_pointers);
    
    
	//return stack_to_heap_pointers;
}


const size_t DEFAULT_TRAVERSE_MAX_OBJECT_SIZE = 250 * 1024;

// TODO see if there a proper Python3 tree data structure (maybe in numpy) we can return
std::vector<RemoteHeapPointer> Traverse(const pid_t pid, const std::vector<RemoteHeapPointer>& base_pointers,
 const size_t max_object_size=DEFAULT_TRAVERSE_MAX_OBJECT_SIZE){
    auto parser = MapParser(pid);
	auto entries = parser.ParseMap();
    
    auto heap_traverser = HeapTraverser(pid, parser.getStoredHeap(), DEFAULT_TRAVERSE_MAX_OBJECT_SIZE);
    
    return heap_traverser.TraversePointers(base_pointers);
}




PYBIND11_MODULE(mgpybind, m) {
    
    m.def("_CanRun", &CanRun, R"pbdoc(
	Check if the system is configured to allow mem-grep to access other programs memory\n
	Either run the program as root or run \"echo 0 > /proc/sys/kernel/yama/ptrace_scope\" as root
    )pbdoc");
    
	py::class_<RemoteHeapPointer>(m, "RemoteHeapPointer")
		.def(py::init<>())
		.def_readonly("points_to",&RemoteHeapPointer::points_to)
		.def_readonly("size_pointed_to",&RemoteHeapPointer::size_pointed_to)
		.def_readonly("actual_address",&RemoteHeapPointer::actual_address);

    
      m.def("_search", &Search, R"pbdoc(
        Search memory for pointers to the heap
    )pbdoc",py::arg("pid"),py::arg("SearchBss")=true, py::arg("SearchStack")=true);

    m.def("_traverse", &Traverse, R"pbdoc(
       Traverse pointers to the heap (found via search)
    )pbdoc",py::arg("pid"), py::arg("base_pointers"),py::arg("max_object_size")=DEFAULT_TRAVERSE_MAX_OBJECT_SIZE);
};
