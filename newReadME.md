FatRec - a simple tool to recover fat32 drive files
[an image will be here]

Need to have openssl installed 

### On Mac 
With brew installed run
```bash
brew install openssl
```


### On linux


to install run 
```bash
a link will be here
````


here are the use : 
void errUse() {
    fprintf(stderr, "Usage: fatrec32 disk <options>\n");
    fprintf(stderr, "  -i                     Print the file system information.\n");
    fprintf(stderr, "  -l                     List the root directory.\n");
    fprintf(stderr, "  -r filename [-s sha1]  Recover a contiguous file.\n");
    fprintf(stderr, "  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
    fprintf(stderr, "  -ra filename           Recover all files with the given name.\n");
    fprintf(stderr, "  -all                   Recover all deleted files.\n");
}


