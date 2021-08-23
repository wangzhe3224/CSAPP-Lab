#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

typedef unsigned long int uint64_t;

typedef struct {
    int valid;
    int lru;
    uint64_t tag;
} cacheLine;

typedef cacheLine* cacheSet;
typedef cacheSet* Cache;

const char* usage = "Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n";

static int verbose = 0;
static int s;  // bits per set index
static int b;  // bits per block
static int E;  // lines per set
static Cache cache;

FILE* fp = NULL;

static int hits = 0;
static int misses = 0;
static int evictions = 0;

void parseArguments(int argc, char* argv[]);
int visitCache(uint64_t address);
int simulate();

int main(int argc, char* argv[])
{
    parseArguments(argc, argv);
    simulate();
    printSummary(hits, misses, evictions);
    return 0;
}

void parseArguments(int argc, char* argv[])
{
    int opt;
    while ((opt=getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch(opt)
        {
            case 'h':
                fprintf(stdout, usage, argv[0]);
                exit(1);
            case 'v':
                verbose = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                fp = fopen(optarg, "r");
                break;
            default:
                fprintf(stdout, usage, argv[0]);
                exit(1);
        }
    }
}

int simulate()
{
   // allocation cache in heap with S * sizeof(cacheSet) bytes.
   int S = pow(2, s);
   cache = (Cache)malloc(sizeof(cacheSet) * S); // cache is a pointer to cacheSet
//    printf("sizeof(cacheSet): %li", sizeof(cacheSet));
   if (cache == NULL) return -1;

   // for each set, allocate E lines memory in heap
   for (int i = 0; i < S; i++) 
   {
       cache[i] = (cacheSet)calloc(E, sizeof(cacheLine));  
       if (cache[i] == NULL) return -1;
   }

   char buf[20];
   char operation;
   uint64_t address;
   int size;

   while (fgets(buf, sizeof(buf), fp) != NULL) 
   {
       int ret;

       if (buf[0] == 'I')
       {
           continue; // ignore instruction load
       }
       else
       {
           sscanf(buf, " %c %lx,%d", &operation, &address, &size);

           switch (operation)
           {
           case 'S':
               ret = visitCache(address);
               break;
           case 'L':
               ret = visitCache(address);
               break;
           case 'M':
               ret = visitCache(address);
               hits++;
               break;
           }
           if (verbose)
           {
               switch (ret)
               {
               case 0:
                   printf("%c %lx,%d hit\n", operation, address, size);
                   break;
               case 1:
                   printf("%c %lx,%d miss\n", operation, address, size);
                   break;
               case 2:
                   printf("%c %lx,%d miss eviction\n", operation, address, size);
                   break;
               }
           }
       }
   }

   // free heap...
   for (int i = 0; i < S; i++)
   {
       free(cache[i]);
   }
   free(cache);
   // close resource
   fclose(fp);

   return 0;
}

/**
 * @brief 
 * 0 cache hit
 * 1 cache miss
 * 2 cache miss, eviction
 */
int visitCache(uint64_t address)
{
    // |-- tag --|-- set index --|-- shift --|
    // |    t    |      s        |     b     |
    uint64_t tag = address >> (s+b);  // s bits per set index, b bits per block
    unsigned int setIndex = address >> b & ((1 << s) - 1);  // ?

    int evict = 0;
    int empty = -1;
    cacheSet cacheset = cache[setIndex];

    // scan lines in the cache set
    for (int i = 0; i < E; i++) 
    {
        if (cacheset[i].valid)
        {
            // cache hit
            if (cacheset[i].tag==tag)
            {
                hits++;
                cacheset[i].lru = 1;
                return 0;
            }

            cacheset[i].lru++;
            if (cacheset[evict].lru < cacheset[i].lru){
                evict = i;
            }
        }
        else
        {
            empty = i;
        }
    }
    
    // miss
    misses++;

    if (empty != -1)
    {
        // miss, but cache is not full
        cacheset[empty].valid = 1;
        cacheset[empty].lru = 1;
        cacheset[empty].tag = tag;
        return 1;
    }
    else
    {
        // miss and cache is full
        cacheset[evict].tag = tag;
        cacheset[evict].lru = 1;
        evictions++;
        return 2;
    }
}


/*return value
      0             cache hit
      1             cache miss
      2             cache miss, eviction
*/
int visitCache2(uint64_t address)
{
    uint64_t tag = address >> (s + b);
    unsigned int setIndex = address >> b & ((1 << s) - 1);

    int evict = 0;
    int empty = -1;
    cacheSet cacheset = cache[setIndex];

    for (int i = 0; i < E; i++)
    {
        if (cacheset[i].valid)
        {
            if (cacheset[i].tag == tag)
            {
                hits++;
                cacheset[i].lru = 1;
                return 0;
            }

            cacheset[i].lru++;
            if (cacheset[evict].lru <= cacheset[i].lru) // =是必须的,why?
            {
                evict = i;
            }
        }
        else
        {
            empty = i;
        }
    }

    //cache miss
    misses++;

    if (empty != -1)
    {
        cacheset[empty].valid = 1;
        cacheset[empty].tag = tag;
        cacheset[empty].lru = 1;
        return 1;
    }
    else
    {
        cacheset[evict].tag = tag;
        cacheset[evict].lru = 1;
        evictions++;
        return 2;
    }
}