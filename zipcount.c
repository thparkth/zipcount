#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#define CDIRSIG "\x50\x4b\x01\x02"
#define EOCDSIG "\x50\x4b\x05\x06"
#define EOCDSIZE 10

int listFilesMode = 0;
int countFilesMode = 0;
int showZipCountMode = 1;
int showSizeMode = 0;
int includeZeroByteFiles = 1;

struct EOCD {
  off_t offset;
  int   fileCount;
  off_t firstEntry;
  long  cdirSize;
};

/* Utility function to build multibyte integers from substrings */
unsigned long pack(unsigned char *buff, int count) {
  unsigned long result = 0;
  for (int n=0; n < count; n++) {
    result += (unsigned int) buff[n] << (8 * n);
  }
  return result;
}

/* Find the End Of Central Directory at the end of the ZIP file and
   return populate an EOCD structure */
struct EOCD readEOCD(FILE *fp) {
  struct EOCD returnStruct = { .offset = 0, .fileCount = 0, .firstEntry = 0 , .cdirSize = 0 };
  off_t searchPosition;
  fseek(fp, -EOCDSIZE, SEEK_END);
  searchPosition = ftell(fp) - EOCDSIZE; /* the last possible position for the EOCD signature */
  unsigned char buff[20];
  /* Now search backwards for the EOCD signature */
  for ( ; searchPosition > 0; searchPosition--) {
    fseek(fp, searchPosition, SEEK_SET);
    if (! fread(buff, 20, 1, fp)) {
      printf("Error reading ZIP file\n");
      exit(1);
    }

    if (! strncmp(buff, EOCDSIG, 4)) {
      returnStruct.offset = searchPosition;
      returnStruct.fileCount = pack(buff + 10, 2);
      returnStruct.cdirSize = pack(buff + 12, 4);
      returnStruct.firstEntry = pack(buff + 16, 4);
      return returnStruct;
    }
  }
  return returnStruct;
}

/* List all of the files in the ZIP */
long listFiles (FILE *fp, off_t firstEntryOffset, long dirSize) {
  /* We read the entire directory into RAM */
  unsigned char *buff = (char *) malloc(dirSize);
  fseek(fp, firstEntryOffset, SEEK_SET);
  if (! fread(buff, dirSize, 1, fp)) {
    printf("Error reading ZIP file\n");
    exit(1);
  }
  char filename[256];
  int filenameLength;
  long fileSize;
  long count = 0;
  /* Search for the central directory entry signature */
  for (long pos = 0; pos < dirSize; pos++) {
    if (! strncmp(buff + pos, CDIRSIG, 4)) {
      filenameLength = buff[pos + 28] + (buff[pos + 29] << 8);
      if (filenameLength > 255) {
        filenameLength = 255;
      }
      strncpy(filename, buff + pos + 46, filenameLength);

      /* Skip names ending in / - those are directories */
      if (filename[filenameLength - 1] == '/') {
        continue;
      }
      filename[filenameLength] = 0;
      fileSize = pack (buff + pos + 24, 4);

      /* Maybe skip zero-byte files depending on options */
      if (fileSize == 00 && ! includeZeroByteFiles) {
        continue;
      }
      
      count++;
      if (listFilesMode) {
        printf("%s", filename);
        if (showSizeMode) {
          printf("\t%ld\n", fileSize);
        } else {
          printf("\n");
        }
      }
      pos = pos + 42 + filenameLength;
    }
  }
  return count;
}


int main(int argc, char *argv[]) {
  /* Process command line options */
  int opt;
  while ((opt = getopt(argc, argv, "lschdz")) != -1) {
    switch (opt) {
    case 'l':
      listFilesMode = 1;
      showZipCountMode = 0;
      break;
    case 's':
      showSizeMode = 1;
      break;
    case 'z':
      includeZeroByteFiles = 0;
      break;
    case 'c':
      countFilesMode = 1;
      showZipCountMode = 0;
      break;
    case 'd':
      showZipCountMode = 1;
      break;
    case 'h':
      printf("Usage: %s [-d] [-l] [-c] [-s] [-z] [-h] <filename>\n\n", argv[0]);
      printf(" -d  print directory count from ZIP file (includes folders). VERY fast\n");
      printf(" -l  list files (not folders) in ZIP\n");
      printf(" -s  (include sizes in listing)\n");
      printf(" -c  count files (not folders) in ZIP\n");
      printf(" -z  Exclude zero-byte files from -l and -c\n");
      printf(" -h  show this help\n\n");
      printf("Default is -z (directory count from ZIP file only).\n");
      exit(0);
      break;
    }
  }

  if (optind >= argc) {
    printf("Usage: %s [-d] [-l] [-c] [-s] [-z] <filename>\n", argv[0]);
    exit(1);
  }

  /* Hopefully, open the ZIP file */
  
  FILE *fp;
  fp = fopen(argv[optind], "rb");
  if (fp == 0) {
    printf("ERROR: Could not open file %s\n", argv[1]);
    exit(1);
  }

  /* Read the EOCD record, which includes the count of directory entries in the ZIP */
  struct EOCD thisEOCD = readEOCD(fp);

  if (thisEOCD.offset == 0) {
    /* Was this even a ZIP file? */
    printf("ERROR: Could not find EOCD in ZIP file\n");
    exit(1);
  }

  if (showZipCountMode) {
    /* The EOCD includes a count of the directory entries in the file. If that's all
       that's needed, this is a very fast option. But it does include directories in
       the count. */
    printf("%d\n", thisEOCD.fileCount);
    exit(0);
  }

  /* If we get here, we need to actually enumerate the central directory entries */
  /* listFiles will print the filenames and sizes if the appropriate global variables
     are set */
  long fileCount = listFiles(fp, thisEOCD.firstEntry, thisEOCD.cdirSize);

  if (countFilesMode) {
    printf("%ld\n", fileCount);
  }

  /* Tidy up, rather pointlessly */
  fclose(fp);
}

