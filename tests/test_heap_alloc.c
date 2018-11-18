
#include <stdlib.h>
#include <stdio.h>

#include "heap.h"
#include "symrepr.h"


int main(int argc, char **argv) {

  int res = 1;

  int heap_size = 1024 * 1024; 
  uint32_t cell;

  res = symrepr_init();
  if (!res) {
    printf("Error initializing symrepr\n");
    return 0;
  }
  printf("Initialized symrepr: OK\n"); 
  
  res = heap_init(heap_size);
  if (!res) {
    printf("Error initializing heap\n"); 
    return 0;
  }

  printf("Initialized heap: OK\n"); 
  
  for (int i = 0; i < heap_size; i ++) {
    cell = heap_allocate_cell();
    if (!IS_PTR(cell)) {
      printf("Error allocating cell\n"); 
      return 0;
    }
  }
  printf("Allocated %d heap cells: OK\n", heap_size);

  for (int i = 0; i < 34; i ++) {
    cell = heap_allocate_cell();
    if (IS_PTR(cell)) {
      printf("Error allocation succeeded on empty heap\n"); 
      return 0;
    } else if (VAL_TYPE(cell) != VAL_TYPE_SYMBOL ||
	       DEC_SYM(cell) != symrepr_nil()) {
      printf("Error Incorrect return value at cell allocation on full heap\n");
      return 0; 
    }
  }

  printf("HEAP allocation when full test: OK\n");
  return 1; 
  
}
