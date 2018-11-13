# Optimizations

As we work through the project, we discover that there are many areas in the filesystem that could be optimized. Here, we list potential areas for optimization and which ones we actually implement.

## Possible Improvements

### Directories

#### File Deletion

When a file is deleted, it may leave a gap in a directory between two other allocated files. New entries should be able to reuse these old entries that have extra space within, so we ensure that the space doesn't go to waste by including a "record size" length per record in the directory.

#### No Duplicates in Directories

Currently, our filesystem will implement a simple linear scan of filenames in the directory to check if a filename already exists, before allowing the user to create a file with that name. To optimize, we could use a b-tree to perform the lookup much faster.

### File Creation

#### Pre-Allocation Policy
When creating new files that also need data blocks, we can look for a sequence of contiguous free blocks and return them to ensure sequential access to those blocks is optimized.

## Implemented Improvements

TODO!
