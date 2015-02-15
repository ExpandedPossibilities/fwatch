#include "../watchpaths.c"
#include <err.h>
#include <assert.h>

int main(int argc, char **argv){
  char **slashes;
  char *path, *it;
  size_t count, i, plen;
  if(argc < 2 ){
    printf("USAGE: findslashes PATH\n");
    return 1;
  }
  path = argv[1];
  plen = strnlen(argv[1], PATH_MAX);

  slashes = find_slashes(path, 0, &count);
  if(slashes == NULL){
    err(2, "find_slashes returned NULL");
  }
  for(i = 0; i < count; i++){
    /* iterate over slash list and print out the offsets */
    fprintf(stderr, "%zu ", i);
    it = slashes[i];
    if(i == 0){
      assert(it == NULL);
      fprintf(stderr, "---\n");
    } else {
      assert(it != NULL);
      assert(it >= path);
      assert(it <= path + plen);
      fprintf(stderr, "%3ld %.180s\n", it - path, it);
    }
  }
  return 0;
}
