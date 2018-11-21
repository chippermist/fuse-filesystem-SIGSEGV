#include "test.h"

// Test instructions
// 1. Replace fuse.cpp with int main(return 0;)
// 2. Add -g flag and test binary output to Makefile
// 3. Run make clean; make && cd bin; ./test | less

int main(int argc, char** argv) {

    MemoryStorage *disk = new MemoryStorage(1 + 10 + (1 + 512) + (1 + 512 + 512*512) + (1 + 2 + 512*2 + 512*512*2));    //788496

    MyBlockManager block_manager;

    // Create a test INode
    INode file_inode;
    memset(&file_inode, 0, sizeof(file_inode));
    file_inode.type = FileType::REGULAR;
    file_inode.size = 0;
    file_inode.blocks = 0;


    // Read data from a file and put it char array(data), use it to write data
    FILE *f = fopen("file.txt", "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *data = (char *)malloc(fsize + 1);
    fread(data, fsize, 1, f);
    fclose(f);



    char *buf = data;
    size_t size;
    size_t offset;
    size_t data_written;


    /*******************************************
     *********** NO NULL FILLER USED ************
     *******************************************/

    /**********************************************************************/
    /************************ TEST1 ***************************************/
    /******** Test writing from offset without overwriting data ***********/

    // write 100 bytes of data
    offset = 0;
    size = 100;
    data_written = write(buf, size, offset, disk, block_manager, file_inode);
    buf += size;

    assert (size == data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 1);

    // fill the whole data block
    offset = 100;
    size = 4096 - offset;
    data_written = write(buf, size, offset, disk, block_manager, file_inode);
    buf += size;

    assert (size == data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 1);

    // fill in a bit more
    offset = 4096;
    size = 100;
    data_written = write(buf, size, offset, disk, block_manager, file_inode);
    buf += size;

    assert (size == data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 2);

    /******** check boundary where block transitions from direct blocks to indirect blocks *******/

    // fill the entire direct block but the last 100 bytes
    offset = 4096 + 100;
    size = 4096*10 - (4096 + 200);
    data_written = write(buf, size, offset, disk, block_manager, file_inode);
    buf += size;

    assert (size == data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 10);

    // transition from direct blocks to single indirect
    offset = 4096*10 - 100;
    size = 200;
    data_written = write(buf, size, offset, disk, block_manager, file_inode);
    buf += size;

    assert (size == data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 11);

    // check if writing in single indirect works, fill the entire single indirect block but the last 100 bytes
    offset = 4096*10 + 100;
    size = 4096*512 - 200;
    data_written = write(buf, size, offset, disk, block_manager, file_inode);
    buf += size;

    assert (size == data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 522);

    // transition from single indirect blocks to double indirect
    offset = 4096*522 - 100;
    size = 200;
    data_written = write(buf, size, offset, disk, block_manager, file_inode);
    buf += size;

    assert (size == data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 10 + 512 + 1);

    // fill in the some part of double indirect
    offset = 4096*522 + 100;
    size = 4096*512*2;
    data_written = write(buf, size, offset, disk, block_manager, file_inode);
    buf += size;

    assert (size == data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 10 + 512 + 512*2 + 1);

//    long const OUTPUT_FILESIZE = (offset + size) + 1;
//    char read_out[OUTPUT_FILESIZE];
//    memset(read_out, 0, OUTPUT_FILESIZE);
//    read(read_out, file_inode.size, 0, disk, file_inode);
//
//
//    if(!strncmp(data, read_out, size)) {
//        cout << "data written from file.txt is same as data read." << endl;
//    } else {
//        cout << "Data doesn't match" <<endl;
//    }

    //FILESIZE AND BLOCKSIZE AFTER TEST1
    size_t fileSize = 4096*(522 + 512*2) + 100; //6332516
    size_t fileBlocks = 1547;

    /**********************************************************************/
    /**************************** TEST2 ***********************************/
    /*********************** Test OVER-WRITING data ***********************/
    //offset <= fileSize

    char *overwrite_buf = data;

    // overwrite the data starting from a direct data blocks and some part of single indirect data blocks
    //offset < file_inode.size, offset + size < file_inode.size
    offset = 4096*522 - 100;
    size = 4096*512*2;
    data_written = write(overwrite_buf, size, offset, disk, block_manager, file_inode);

    assert (size == data_written);
    assert (file_inode.size = fileSize);
    assert (file_inode.blocks == fileBlocks);

    //offset < file_inode.size, file_inode.size < offset + size < file_inode.blocks*4096
    offset = 4096*(522 + 512*2) - 100;
    size = 1000;
    data_written = write(overwrite_buf, size, offset, disk, block_manager, file_inode);

    assert (size == data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 10 + 512 + 512*2 + 1);

    //offset < file_inode.size, offset + size < file_inode.blocks*4096
    offset = 4096*(522 + 512*2) - 100;
    size = 4096 + 200;
    data_written = write(overwrite_buf, size, offset, disk, block_manager, file_inode);

    assert (size == data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 10 + 512 + 512*2 + 2);

//    long const OUTPUT_FILESIZE = (offset + size) + 1;
//    char read_out[OUTPUT_FILESIZE];
//    memset(read_out, 0, OUTPUT_FILESIZE);
//    read(read_out, size, offset, disk, file_inode);
//
//
//    if(!strncmp(data, read_out, size)) {
//        cout << "data written from file.txt is same as data read." << endl;
//    } else {
//        cout << "Data doesn't match" <<endl;
//    }

    //FILESIZE AND BLOCKSIZE AFTER TEST1
    fileSize = 4096*(522 + 512*2 + 1) + 100; //6336612
    fileBlocks = 1548;



    /*******************************************
     ************* NULL FILLER USED ************
     *******************************************/

    /**********************************************************************/
    /************************ TEST3 ***************************************/
    /******** Test cases where writing data lead to null filler ***********/

    //offset > fileSize

    //file_inode.size < offset < file_inode.blocks*4096, offset + size < file_inode.blocks*4096
    size_t fileSize_initial = fileSize;
    offset = 4096*(522 + 512*2 + 1) + 100 + 400;
    size = 1000;
    data_written = write(overwrite_buf, size, offset, disk, block_manager, file_inode);


    assert (size + offset - fileSize_initial== data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 522 + 512*2 + 1 + 1);

    //file_inode.size < offset < file_inode.blocks*4096, offset + size > file_inode.blocks*4096
    fileSize_initial = file_inode.size;
    offset = 4096*(522 + 512*2 + 1) + 2000;
    size = 4000;
    data_written = write(overwrite_buf, size, offset, disk, block_manager, file_inode);

    assert (size + offset - fileSize_initial== data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 10 + 512 + 512*2 + 3);

    // offset > file_inode.size, offset > file_inode.blocks*4096
    fileSize_initial = file_inode.size;
    offset = 4096*(522 + 512*2 + 2) + 2000;
    size = 4096*2;
    data_written = write(overwrite_buf, size, offset, disk, block_manager, file_inode);

    assert (size + offset - fileSize_initial == data_written);
    assert (file_inode.size = offset + size);
    assert (file_inode.blocks == 10 + 512 + 512*2 + 5);


    long const OUTPUT_FILESIZE = (offset + size) + 1;
    char read_out[OUTPUT_FILESIZE];
    memset(read_out, 0, OUTPUT_FILESIZE);

    //read if null value from fileSize_initial to offset
    read(read_out, offset - fileSize_initial, file_inode.size, disk, file_inode);
    cout << read_out << endl;

    //read the overwritten data
    read(read_out, size, offset, disk, file_inode);


    if(!strncmp(data, read_out, size)) {
        cout << "data written from file.txt is same as data read." << endl;
    } else {
        cout << "Data doesn't match" <<endl;
    }
}
