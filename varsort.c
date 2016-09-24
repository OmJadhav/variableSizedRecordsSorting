#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "sort.h"

void
usage() {
  fprintf(stderr, "Usage: varsort -i inputfile -o outputfile\n");
  exit(1);
}

int
compare(const void * a, const void * b) {
  rec_dataptr_t *recPtrA = (rec_dataptr_t *)a;
  rec_dataptr_t *recPtrB = (rec_dataptr_t *)b;
  return ( recPtrA->key - recPtrB->key );
}

int
main(int argc, char *argv[]) {
  // arguments
  char *inFile = NULL;
  char *outFile = NULL;
  int c, argFlag = 0;
  opterr = 0;
  if (argc != 5) {
    usage();
  }
  while ((c = getopt(argc, argv, "i:o:")) != -1) {
    switch (c) {
      case 'i':
        argFlag++;
        inFile = strdup(optarg);
        break;
      case 'o':
        argFlag++;
        outFile = strdup(optarg);
        break;
      default:
        usage();
    }
  }
  if (inFile == NULL || outFile == NULL || argFlag != 2) {
    usage();
  }

  // open input file
  int fd = open(inFile, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Error: Cannot open file %s\n", inFile);
    exit(1);
  }

  // getting size of file
  struct stat buf;
  fstat(fd, &buf);
  int fileSize = buf.st_size;
  int recordsLeft, totalRecords;
  int rc;

  rc = read(fd, &recordsLeft, sizeof(recordsLeft));
  if (rc != sizeof(recordsLeft)) {
    perror("read");
    exit(1);
  }

  totalRecords = recordsLeft;

  void *voidPtr, *tempVoidPtr;
  rec_nodata_t *recordWithNoData;

  voidPtr = malloc(fileSize-4);
  assert(voidPtr);
  tempVoidPtr = voidPtr;

  rc = read(fd, voidPtr, fileSize - 4);
  if (rc != (fileSize - 4)) {
    perror("read");
    exit(1);
  }
  (void) close(fd);

  /*
  ** create records with dataPtr  and copy the key, data_ints, and pointer to
  ** data for sorting purpose
  */

  int j = 0;
  int movePtr = 0;
  rec_dataptr_t *recordsWithDataPtrs;

  recordsWithDataPtrs = (rec_dataptr_t *) malloc(recordsLeft *
    sizeof(rec_dataptr_t));
  assert(recordsWithDataPtrs);

  while (recordsLeft) {
    recordWithNoData = (rec_nodata_t *)voidPtr;
    recordsWithDataPtrs[j].key = recordWithNoData->key;
    recordsWithDataPtrs[j].data_ints = recordWithNoData->data_ints;
    recordsWithDataPtrs[j].data_ptr =
      (unsigned int *) (voidPtr + (2*(sizeof(unsigned int))));
    movePtr = (sizeof(unsigned int)*2) +
      (recordWithNoData->data_ints*sizeof(unsigned int));
    voidPtr += movePtr;
    j++;
    recordsLeft--;
  }


  /*
  ** sort the data using qsort
  */
  qsort(recordsWithDataPtrs, totalRecords, sizeof(rec_dataptr_t), compare);
  fd = open(outFile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if (fd < 0) {
    fprintf(stderr, "Error: Cannot open file %s\n", outFile);
    exit(1);
  }

  rc = write(fd, &totalRecords, sizeof(totalRecords));
  if (rc != sizeof(totalRecords)) {
    perror("write");
    exit(1);
  }

  rec_data_t tempRecordWithData;
  j = 0;
  int recordSize = 0;
  while (j < totalRecords) {
    tempRecordWithData.key = recordsWithDataPtrs[j].key;
    tempRecordWithData.data_ints = recordsWithDataPtrs[j].data_ints;
    memcpy(tempRecordWithData.data, recordsWithDataPtrs[j].data_ptr,
      (recordsWithDataPtrs[j].data_ints)*sizeof(unsigned int));
    recordSize = (recordsWithDataPtrs[j].data_ints + 2)*sizeof(unsigned int);
    rc = write(fd, &tempRecordWithData, recordSize);
    if (rc != recordSize) {
      perror("write");
      exit(1);
    }
    j++;
  }

  (void) close(fd);
  // free all pointers
  free(recordsWithDataPtrs);
  recordsWithDataPtrs = NULL;
  free(tempVoidPtr);
  tempVoidPtr = NULL;
  free(outFile);
  outFile = NULL;
  free(inFile);
  inFile = NULL;

  return 0;
}


