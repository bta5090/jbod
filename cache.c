#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

// Pointer array that represents cache structure
static cache_entry_t *cache = NULL;
// Stores the size of the cache (number of cache lines) and cannot be altered once cache created.
static int cache_size = 0;
// Counter variable that increments every time cache_lookup is called
static int num_queries = 0;
// Counter variable that increments every time a cache_lookup results in cache hit
static int num_hits = 0;

// cacheEnabled flag variable that indicates there is an existing cache or not. If cache_size is not greater than 2 then variable will be zero.
int cacheEnabled = 0;
// fifoIndex counter variable used to keep track of which element in cache array is next to be evicted if need be.
int fifoIndex = 0;


// Returns 1 on success and -1 on failure. Dynamically allocates space for num_entries cache lines/entries and stores address of cache in cache global variable, each entry being of type cache_entry_t. Cache_size is set to num_entries with the argument range being 2 to 4096. Calling it again * without first calling cache_destroy should return -1.
int cache_create(int num_entries) {

  // Checks if argument is an invalid input
  if (num_entries < 2 || num_entries > 4096)
  {
    
    return -1;

  }
  // Checks if a cache already has been created
  else if (cacheEnabled == 1)
  {
    
    return -1;

  }
  // Executes statements if cache has not been made
  else if (cacheEnabled == 0)
  {

    // size of cache set to input
    cache_size = num_entries;
    // Flag variable set to 1 to indicate cache created
    cacheEnabled = 1;

    // calloc() used to allocate zeroed out num_entries elements with size cache_entry_t
    cache = calloc(num_entries, sizeof(cache_entry_t));

    // For loop initially sets all valid struct fields of entries to false because it's an empty cache.
    for (int i = 0 ; i < cache_size ; i++)
    {

    cache[i].valid = false;

    return 1;

    }
    
  }

  return -1;

}



// Returns 1 on success and -1 on failure. Frees the space allocated by * cache_create function above, sets cache to NULL, and cache_size to zero. Calling this function twice without an intervening cache_create() should fail.
int cache_destroy(void) {
  
  // Checks if there is a cache to deallocate memory from
  if (cacheEnabled == 1)
  {
    
    // Frees dynamic memory
    free(cache);
    // Cache set to NULL
    cache = NULL;

    // Cache_size, cacheEnabled flag, fifoIndex counter, and hit rate variables all set to zero until next cache is created.
    cache_size = 0;
    cacheEnabled = 0;
    fifoIndex = 0;
    num_hits = 0;
    num_queries = 0;

    return 1;

  }

  return -1;
  
}



// Returns 1 on success and -1 on failure. Looks up the block located at * |disk_num| and |block_num| in cache and if found, copies the corresponding * block to |buf|, where buf cannot be NULL. Increments num_queries everytime function's called and increments num_hits every time function returns 1, which are then both used to calculate the hit ratio. 
int cache_lookup(int disk_num, int block_num, uint8_t *buf) {

  // Counter variable incremented
  num_queries += 1;
  
  // Checks if any arguments are invalid inputs or not and if cache exists.
  if (cache == NULL || buf == NULL || disk_num < 0 || disk_num > 15 || block_num < 0 || block_num > 255 || cacheEnabled == 0)
  {
    
    return -1;

  }
  // If arguments are valid, then function continues, if not function fails.
  else
  {

    // For loop iterates through whole cache array and with each valid cache entry, if the disk_num and block_num are the same as input, it copies the block field of that entry to buf.
    for (int i = 0 ; i < cache_size ; i++)
    {
      // Checks if entry is valid, disk_num and block_num are equal to inputs.
      if ((cache[i].disk_num == disk_num) && (cache[i].block_num == block_num) && cache[i].valid == true )
      {
        // Counter variable incremented
        num_hits += 1;
        // Memory copied from block inside cache entry to buf
        memcpy(buf, cache[i].block, JBOD_BLOCK_SIZE);

        return 1;

      }

    }

    return -1;

  }

  return -1;

}



// Checks if input matches to existing cache entry and updates it with data from buffer.
void cache_update(int disk_num, int block_num, const uint8_t *buf) {

  // For loop to iterate through and check if entry with same disk_num and block_num exists
  for (int i = 0 ; i < cache_size ; i++)
  {
    // If true, and a none NULL buf is provided, then the block of that entry is updated with new data from buf.
    if ((cache[i].disk_num == disk_num) && (cache[i].block_num == block_num))
    {

      memcpy(cache[i].block, buf, JBOD_BLOCK_SIZE);

    }
  }
 
}



// Returns 1 on success and -1 on failure. Inserts an entry for |disk_num| and * |block_num| where buf cannot be NULL. Returns -1 if there is already an existing entry in the cache * with |disk_num| and |block_num|.If there cache is full, evicts oldest entry (top of array) and insert the new entry using FIFO policy.
int cache_insert(int disk_num, int block_num, const uint8_t *buf) {
  
  // Checks if arguments are invalid inputs
  if (cache == NULL || disk_num < 0 || disk_num > 15 || buf == NULL || block_num < 0 || block_num > 255 || cacheEnabled == 0)
  {
    
    return -1;

  }
  else
  {
    
    // For loop iterates through whole cache array to see if any already existing valid entry has same disk_num and block_num, and fails if true.
    for (int i = 0 ; i < cache_size ; i++)
    {
      
      if ((cache[i].disk_num == disk_num) && (cache[i].block_num == block_num) && (cache[i].valid == true))
      {

        return -1;

      }

    }

    // Calculates whether the fifoIndex has incremented too far above the max. array index
    int y = cache_size - 1;

    // If true, index is reset since the first element will now be the evictee based on FIFO because the cache is full.
    if (fifoIndex > y)
    {
      
      // Index tracker set to zero
      fifoIndex = 0;

      // Newest entry added to cache[0] and all its fields correctly entered
      cache[fifoIndex].valid = true;
      cache[fifoIndex].disk_num = disk_num;
      cache[fifoIndex].block_num = block_num;
      memcpy(cache[fifoIndex].block, buf, JBOD_BLOCK_SIZE);

      // Index tracker now incremented after insert process to ensure that the following cache_insert() evicts the corrects entry, which would be cache[1] if fifoIndex = 0 initially.
      fifoIndex += 1;

      return 1;

    }
    // If cache is not full then no need to reset fifoIndex and just add new entries and their data into the respective struct fields.
    else
    {
      
      cache[fifoIndex].valid = true;
      cache[fifoIndex].disk_num = disk_num;
      cache[fifoIndex].block_num = block_num;
      memcpy(cache[fifoIndex].block, buf, JBOD_BLOCK_SIZE);

      // Index tracker incremented 
      fifoIndex += 1;
      
      return 1;

    }

  }

  return -1;

}



// Returns true if cache is enabled (cache_size > 2) and false if not. I use the flag variable cacheEnabled as well in my implementation so for any cache with size greater than 2, I will also set cache as enabled as a visible counter for myself, and the program. 
bool cache_enabled(void) {
  
  // If cache exists and has size greater than 2 the function returns true
  if (cacheEnabled == 1 && cache_size > 2)
  {
    
    return true;

  }

  return false;

}



// Prints the hit rate of the cache.
void cache_print_hit_rate(void) {
  
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);

}