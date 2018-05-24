b mm_init
b mm_malloc
b mm_free
b insert_node
run -f ./traces/short1-bal.rep

define se
	print segregated_lists[$arg0]
end

define hh
call mm_checkheap(1)
end

define lo
	call mem_heap_lo()
end

define hi
	call mem_heap_hi()
end
