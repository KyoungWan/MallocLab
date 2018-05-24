b mm_init
b mm_malloc
b mm_free
b insert_node
run -f ./traces/short1-bal.rep

define se
	print segregated_lists[$arg0]
end

define sea
  print segregated_lists
end

