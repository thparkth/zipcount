# zipcount
A minimal and fast tool for listing and counting zip file entries.

## Overview

Zipcount can do basically two things. It can tell you how many files are in a ZIP, and it can list them.

That's it.

### Listing Files

```
zipcount -l[sz] <filename>
```

The -l option causes zipcount to list the files in the ZIP. It lists only files; not folders.

Adding -s causes it to also display the file size in bytes, tab separated from the filename.

Adding -z causes it to skip zero-byte files.

### Counting Files

```
zipcount -d
zipcount -c[z]
```

#### -d - show directory count, includes folders

Each ZIP file contains a data summary at the end of the file (the "end of central directory record"). This summary includes the
count of directory entries written to the file. The -d option simply finds that record and displays the value.

This is EXTREMELY fast, typically involving only one low-level read operation. The limitation is that the count includes folders
in the ZIP, which you may not want, and there is no way to exclude zero-byte files, which is required in the original use-case
for this program.

#### -c - actually count the files, excluding folders

The -c option counts the directory entries (excluding folders) in the file. Optionally -z may also be specified, causing the
program to exclude zero-byte files from the count.

### Notes

- -l and -c may be combined.

- ZIP64 format files are not supported

- files with encrypted directories are not supported




