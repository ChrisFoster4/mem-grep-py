#include "../mem-grep/lib/filtering/heap-filter.hpp"
#include "../mem-grep/lib/heap-traversing/stack-searcher.hpp"
#include "../mem-grep/lib/misc/map-parser.hpp"
#include "../mem-grep/lib/misc/prerun-checks.hpp"
#include "../mem-grep/lib/misc/remote-memory.hpp"
#include "../mem-grep/lib/misc/structs.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// TODO
// ability to filter results
// ability to inspect and edit matched objects

namespace py = pybind11;

std::vector<RemoteHeapPointer> Search(const pid_t pid,
                                      const bool SearchBss = true,
                                      const bool SearchStack = true) {

  auto parser = MapParser(pid);
  auto entries = parser.ParseMap();
  const MapsEntry stack = parser.getStoredStack();
  const MapsEntry text = parser.getStoredText();
  const MapsEntry heap = parser.getStoredHeap();
  const MapsEntry bss = parser.getStoredBss();

  // TODO pass this via the args
  // Is this a helpful option? as the objects arent returned over the C++/Python
  // border so it doesnt matter if they are large
  const auto max_object_size = 2500000;

  auto stack_to_heap_pointers = std::vector<RemoteHeapPointer>();
  if (SearchStack) {
    auto stackSearch = StackSearcher(stack.start, text, pid, 2550);
    stack_to_heap_pointers =
        stackSearch.findHeapPointers(stack.end, heap, 10000);
  }

  auto bss_to_heap_pointers = std::vector<RemoteHeapPointer>();
  if (SearchBss) {
    auto bssSearcher = BssSearcher(bss, pid, 25000);
    bss_to_heap_pointers = bssSearcher.FindHeapPointers(heap);
  }

  // Merge the pointers found in the bss and the stack
  // TODO do we want a way for the user to be able to tell where a pointer came
  // from?
  stack_to_heap_pointers.reserve(stack_to_heap_pointers.size() +
                                 bss_to_heap_pointers.size());
  stack_to_heap_pointers.insert(stack_to_heap_pointers.end(),
                                bss_to_heap_pointers.begin(),
                                bss_to_heap_pointers.end());
  return stack_to_heap_pointers;
}

// TODO let use specify this via criteria
const size_t DEFAULT_TRAVERSE_MAX_OBJECT_SIZE = 250 * 1024;

// TODO option to flatten. And option to pass in search crietria
std::vector<RemoteHeapPointer>
Traverse(const pid_t pid, const std::vector<RemoteHeapPointer> &base_pointers,
         const size_t max_object_size = DEFAULT_TRAVERSE_MAX_OBJECT_SIZE) {
  auto parser = MapParser(pid);
  auto entries = parser.ParseMap();

  const auto only_flatten = [&](const RemoteHeapPointer &ptr) -> bool {
    return true;
  };

  auto heap_traverser = HeapTraverser(pid, parser.getStoredHeap(),
                                      DEFAULT_TRAVERSE_MAX_OBJECT_SIZE);

  auto traversed = heap_traverser.TraversePointers(base_pointers);
  return HeapFilter::FlattenAndFilter(traversed, only_flatten);
}

enum Allocators : uint32_t { GLIBC = 0 };

PYBIND11_MODULE(mgpybind, m) {

  py::enum_<Allocators>(m, "Allocators")
      .value("GLIBC", Allocators::GLIBC)
      .export_values();

  m.def("_CanRun", &CanRun, R"pbdoc(
	Check if the system is configured to allow mem-grep to access other programs memory\n
	Either run the program as root or run \"echo 0 > /proc/sys/kernel/yama/ptrace_scope\" as root
    )pbdoc");

  py::class_<RemoteHeapPointer>(m, "RemoteHeapPointer")
      .def(py::init<>())
      .def_readonly("size_pointed_to", &RemoteHeapPointer::size_pointed_to)

      // TODO find a way to pass these void* out. They are always NULL in Python
      // The value is still there as when passed back to C++ as the struct
      // contains the non NULL pointers
      .def_readonly("points_to", &RemoteHeapPointer::points_to)
      .def_readonly("actual_address", &RemoteHeapPointer::actual_address)

      .def_readonly("total_sub_pointers",
                    &RemoteHeapPointer::total_sub_pointers)
      .def_readonly("contains_pointers_to",
                    &RemoteHeapPointer::contains_pointers_to);

  m.def("_search", &Search, R"pbdoc(
        Search the stack and/or the bss segment for pointers to the heap
    )pbdoc",
        py::arg("pid"), py::arg("SearchBss") = true,
        py::arg("SearchStack") = true);

  m.def("_traverse", &Traverse, R"pbdoc(
       Traverse pointers to the heap (found via search)
    )pbdoc",
        py::arg("pid"), py::arg("base_pointers"),
        py::arg("max_object_size") = DEFAULT_TRAVERSE_MAX_OBJECT_SIZE);
};
