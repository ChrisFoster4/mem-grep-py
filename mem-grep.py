import mgpybind
import logging

RemoteHeapPointer = mgpybind.RemoteHeapPointer;


class SystemConfigurationError(Exception):
    pass

def can_run() -> bool:
    return mgpybind._CanRun()

def search(pid: int, search_bss:bool = True, search_stack: bool = True):
    if pid < 1:
        raise ValueError(f"pid must be a positive integer")
    
    if search_bss is False and search_stack is False:
        raise ValueError(f"Both search_bss and search_stack cannot be False")
    
    if can_run() is False:
        raise SystemConfigurationError()
    
    return mgpybind._search(pid,search_bss,search_stack)


def traverse(pid: int, base_pointers:[RemoteHeapPointer], filter=None)->[[list]]:
    if can_run() is False:
        raise SystemConfigurationError()
    
    if not base_pointers:
        logging.error(f"{__name__} asked to traverse an empty list")
        return []
    
    return mgpybind._traverse(pid, base_pointers);

class Filter:
    def __init__(self):
        raise NotImplementedError


if __name__ == "__main__":
    sll_pid = 15132
    #print(len(search(14338, search_bss=True, search_stack=False)))
    bases = search(sll_pid, search_bss=True, search_stack=True)
    
    print("shallow",len(bases))
    
    deep = traverse(15132,bases)
    print("deep:",len(deep))
    
