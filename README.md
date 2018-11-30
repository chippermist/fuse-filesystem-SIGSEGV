# SIGSEGV

<img src="https://travis-ci.com/chippermist/SIGSEGV.svg?branch=master" />

## Usage
To run a fuse filesystem 
1) Run mkfs on the filesystem device you want to mount the filesystem on
2) 
   * If it is an **empty** filesystem then use `./fuse <mount point path name> -d -s` to mount the filesystem
   * If it is an **non-empty** filesystem then use `./fuse <mount point path name> -d -s -o nonempty` to mount the filesystem
3) Once the filesystem is mounted it you can `cd <mount point path name>` to start using it. 

#### Flags
* `-d`   : Debugging mode
* `-s`   : Single thread
* `-o`   : Optional Arguments
