"""
This file is a thin wrapper around the C++ library
For documentation of inner workings goto https://github.com/ChrisFoster4/mem-grep

"""
import mgpybind
from dataclasses import dataclass
import ctypes
import logging
import sys

RemoteHeapPointer = mgpybind.RemoteHeapPointer;


class SystemConfigurationError(Exception):
    pass


class MustContain:
    SUPPORTED_DATA_TYPES = [ctypes.c_uint32, ctypes.c_uint64, ctypes.c_double, ctypes.c_float, str]
    def __init__(self, data_type, value):
        if type(data_type) not in self.SUPPORTED_DATA_TYPES:
            raise ValueError(f"Unsupported data type. Must be one of: {self.SUPPORTED_DATA_TYPES}")
        
        self.value=value
        self.data_type = data_type


class FilterCriteria:
    def __init__(self, minimum_children=None, maximum_children=None, minimum_size=0, maximum_size=100000, maximum_decendants=None, minimum_decendants=None, must_contain:[MustContain]=None):
        pass
        
        
# Check that the system is configured to allow us access to other processes memory (or we are root)
def can_run() -> bool:
    return mgpybind._CanRun()


def search(pid: int, search_bss:bool = True, search_stack: bool = True):
    if can_run() is False:
        raise SystemConfigurationError()
    
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


def filter(pid: int, pointers:[RemoteHeapPointer], filter: FilterCriteria):
    raise NotImplementedError

# Fetch a copy of an object from the process
def fetch_object(pid: int, pointer:RemoteHeapPointer) -> bytes:
    
    if can_run() is False:
        raise SystemConfigurationError()
    
    if pid < 1:
        raise ValueError(f"pid must be a positive integer")
    
    view: memoryview = mgpybind._FetchObject(pid, pointer)
    return view.tobytes()
    

class Substitution:
    def __init__(self, data_type, current_value, new_value):
        raise NotImplementedError

# Change values specified by the substitutions in a provided object
def substitute(pid: int, target_object:RemoteHeapPointer, substitutions=[Substitution]):
    raise NotImplementedError

